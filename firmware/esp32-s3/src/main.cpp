/*
 * firmware_cima_iot.ino
 * Agente IoT — Planta 1: CIMA
 * Hardware: ESP32-S3 + SCT-013 (sensor corriente AC) + RC522 (RFID)
 *
 * Función:
 *   - Mide consumo eléctrico por ciclo de máquina (taladro/torno/fresa)
 *   - Lee RFID de piezas para trazabilidad (Pasaporte Digital)
 *   - Publica datos vía MQTT al broker en Raspberry Pi 5
 *
 * Topics MQTT publicados:
 *   cima/machines/{machine_id}/energy  → {"kWh": 0.12, "W": 1200, "ts": "..."}
 *   cima/machines/{machine_id}/cycle   → {"start": "...", "end": "...", "part_id": "..."}
 *   cima/rfid/scan                     → {"part_id": "ABC123", "machine": "fresadora", "ts": "..."}
 *   cima/system/heartbeat              → {"ip": "...", "rssi": -65, "uptime": 3600}
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>

// ─── Configuración WiFi y MQTT ─────────────────────────────────────────────
const char* WIFI_SSID     = "TU_WIFI_SSID";
const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD";
const char* MQTT_BROKER   = "192.168.1.100";  // IP de tu Raspberry Pi 5
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "esp32_cima";
const char* MQTT_PASSWORD = "Esp32Cima!";
const char* CLIENT_ID     = "esp32-cima-planta1";

// ─── Identificación de máquina ─────────────────────────────────────────────
// Cambiar según cuál máquina esté monitoreando
const char* MACHINE_ID   = "fresadora";  // opciones: taladro, torno, fresadora
const char* MACHINE_NAME = "Fresadora CIMA-01";

// ─── Pines Hardware ────────────────────────────────────────────────────────
// SCT-013 (sensor de corriente AC) — conectado a ADC a través de burden resistor
const int SCT_PIN         = 36;   // ADC1_CH0 — GPIO36 (solo lectura)
const int BURDEN_RESISTOR = 33;   // Ohms — ajustar según tu SCT-013
const int SCT_TURNS       = 100;  // Relación de transformación del SCT-013-100A

// RFID RC522
const int RFID_SS_PIN     = 5;
const int RFID_RST_PIN    = 27;

// LED de estado (built-in en ESP32-S3)
const int LED_PIN         = 48;

// Botón inicio/fin de ciclo (opcional, conectar a GND con pull-up)
const int CYCLE_BTN_PIN   = 0;

// ─── Variables globales ────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient mqtt(espClient);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// Estado del ciclo
bool     cycleActive    = false;
unsigned long cycleStart = 0;
String   currentPartId  = "";
float    cycleEnergyWh  = 0.0;

// Muestreo de corriente
const int   SAMPLES         = 1480;   // ~1 ciclo AC a 60Hz con 24kHz ADC
const float VCC             = 3.3;
const float ADC_RESOLUTION  = 4096.0;
const float ICAL            = 90.9;   // Factor de calibración — ajustar con medidor real

unsigned long lastMqttPublish  = 0;
unsigned long lastHeartbeat    = 0;
const long    PUBLISH_INTERVAL = 5000;   // ms entre publicaciones de energía
const long    HEARTBEAT_INTERVAL = 30000; // ms entre heartbeats

// ─── Funciones auxiliares ──────────────────────────────────────────────────

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "1970-01-01T00:00:00Z";
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

void ledBlink(int times, int ms = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(ms);
    digitalWrite(LED_PIN, LOW);
    delay(ms);
  }
}

// ─── Medición de corriente RMS con SCT-013 ────────────────────────────────
struct PowerReading {
  float irms;   // Corriente RMS (A)
  float watt;   // Potencia activa estimada (W)
  float kwh;    // Energía acumulada (kWh) en este intervalo
};

PowerReading measurePower() {
  long sumSq = 0;
  int offsetI = 2048;  // Offset ADC para señal AC centrada

  // Acumular muestras
  for (int n = 0; n < SAMPLES; n++) {
    int raw = analogRead(SCT_PIN);
    int shifted = raw - offsetI;
    sumSq += (long)shifted * shifted;
    delayMicroseconds(10);
  }

  // Calcular IRMS
  float irms = (sqrt((float)sumSq / SAMPLES) / ADC_RESOLUTION * VCC) / BURDEN_RESISTOR * SCT_TURNS * ICAL / 100.0;
  if (irms < 0.05) irms = 0.0;  // Filtrar ruido

  // Potencia estimada (asumiendo FP=0.85 para motores industriales)
  float watt = irms * 220.0 * 0.85;

  // Energía en kWh para este intervalo (5 segundos)
  float kwh = watt * (PUBLISH_INTERVAL / 1000.0) / 3600.0 / 1000.0;

  return {irms, watt, kwh};
}

// ─── Publicar datos de energía vía MQTT ──────────────────────────────────
void publishEnergy(PowerReading& p) {
  StaticJsonDocument<256> doc;
  doc["machine_id"] = MACHINE_ID;
  doc["machine_name"] = MACHINE_NAME;
  doc["irms_a"] = round(p.irms * 100) / 100.0;
  doc["power_w"] = round(p.watt);
  doc["energy_kwh"] = round(p.kwh * 10000) / 10000.0;
  doc["cycle_active"] = cycleActive;
  if (cycleActive) doc["part_id"] = currentPartId;
  doc["ts"] = getTimestamp();
  doc["rssi"] = WiFi.RSSI();

  char topic[64];
  snprintf(topic, sizeof(topic), "cima/machines/%s/energy", MACHINE_ID);

  char payload[256];
  serializeJson(doc, payload);
  mqtt.publish(topic, payload, true);  // retain=true

  if (cycleActive) {
    cycleEnergyWh += p.kwh * 1000.0;  // Acumular en Wh
  }
}

// ─── Publicar fin de ciclo con KPI de energía ────────────────────────────
void publishCycleEnd() {
  unsigned long duration = (millis() - cycleStart) / 1000;

  StaticJsonDocument<256> doc;
  doc["machine_id"]    = MACHINE_ID;
  doc["part_id"]       = currentPartId;
  doc["duration_sec"]  = duration;
  doc["energy_wh"]     = round(cycleEnergyWh * 100) / 100.0;
  doc["ts_end"]        = getTimestamp();
  doc["status"]        = "completed";

  char topic[64];
  snprintf(topic, sizeof(topic), "cima/machines/%s/cycle", MACHINE_ID);

  char payload[256];
  serializeJson(doc, payload);
  mqtt.publish(topic, payload, true);

  Serial.printf("[CYCLE END] Pieza: %s | Duración: %lus | Energía: %.2f Wh\n",
    currentPartId.c_str(), duration, cycleEnergyWh);
}

// ─── Leer RFID (Pasaporte Digital) ───────────────────────────────────────
String readRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return "";
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return uid;
}

void publishRFID(String uid) {
  StaticJsonDocument<200> doc;
  doc["part_id"]  = uid;
  doc["machine"]  = MACHINE_ID;
  doc["location"] = "cima_entrada";
  doc["ts"]       = getTimestamp();
  doc["action"]   = cycleActive ? "cycle_start" : "scan";

  char payload[200];
  serializeJson(doc, payload);
  mqtt.publish("cima/rfid/scan", payload, true);

  Serial.printf("[RFID] Pieza escaneada: %s\n", uid.c_str());
  ledBlink(3, 50);
}

// ─── Heartbeat del sistema ────────────────────────────────────────────────
void publishHeartbeat() {
  StaticJsonDocument<200> doc;
  doc["device"]  = CLIENT_ID;
  doc["machine"] = MACHINE_ID;
  doc["ip"]      = WiFi.localIP().toString();
  doc["rssi"]    = WiFi.RSSI();
  doc["uptime"]  = millis() / 1000;
  doc["ts"]      = getTimestamp();

  char payload[200];
  serializeJson(doc, payload);
  mqtt.publish("cima/system/heartbeat", payload);
}

// ─── Callback MQTT (recibe comandos) ─────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.printf("[MQTT IN] %s: %s\n", topic, msg.c_str());

  String topicStr = String(topic);

  // Comando para iniciar/detener ciclo manualmente
  if (topicStr == "cima/commands/cycle") {
    StaticJsonDocument<100> cmd;
    deserializeJson(cmd, msg);
    if (cmd["action"] == "start") {
      cycleActive = true;
      cycleStart = millis();
      cycleEnergyWh = 0.0;
      currentPartId = cmd["part_id"] | "UNKNOWN";
      Serial.printf("[CMD] Ciclo iniciado para pieza: %s\n", currentPartId.c_str());
      ledBlink(2, 200);
    } else if (cmd["action"] == "stop") {
      publishCycleEnd();
      cycleActive = false;
      currentPartId = "";
    }
  }

  // Comando OTA (para actualización remota futura)
  if (topicStr == "cima/commands/ota") {
    Serial.println("[OTA] Actualización remota recibida — implementar con ArduinoOTA");
  }
}

// ─── Reconexión MQTT ──────────────────────────────────────────────────────
void reconnectMQTT() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print("[MQTT] Conectando... ");
    if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("OK");
      mqtt.subscribe("cima/commands/cycle");
      mqtt.subscribe("cima/commands/ota");
      ledBlink(4, 100);
    } else {
      Serial.printf("Fallo (rc=%d) — reintento en 5s\n", mqtt.state());
      delay(5000);
      attempts++;
    }
  }
}

// ─── Setup ────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[CIMA IoT Gateway] Iniciando...");

  pinMode(LED_PIN, OUTPUT);
  pinMode(CYCLE_BTN_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // Para señales hasta 3.3V

  // Inicializar RFID
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();

  // Conectar WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Conectando");
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Conectado: %s\n", WiFi.localIP().toString().c_str());
    ledBlink(5, 100);
  } else {
    Serial.println("\n[WiFi] Fallo de conexión — modo offline");
  }

  // Configurar tiempo NTP
  configTime(-21600, 0, "pool.ntp.org");  // UTC-6 para Querétaro
  Serial.println("[NTP] Tiempo sincronizado");

  // Configurar MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);

  Serial.printf("[READY] Máquina: %s | IP: %s\n", MACHINE_NAME, WiFi.localIP().toString().c_str());
}

// ─── Loop principal ───────────────────────────────────────────────────────
void loop() {
  // Reconectar si se pierde conexión
  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  unsigned long now = millis();

  // Publicar energía cada PUBLISH_INTERVAL ms
  if (now - lastMqttPublish >= PUBLISH_INTERVAL) {
    lastMqttPublish = now;
    PowerReading p = measurePower();
    publishEnergy(p);
    Serial.printf("[ENERGY] %.1fW | IRMS: %.2fA | Ciclo: %s\n",
      p.watt, p.irms, cycleActive ? currentPartId.c_str() : "inactivo");
  }

  // Heartbeat
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    publishHeartbeat();
  }

  // Leer RFID
  String uid = readRFID();
  if (uid.length() > 0) {
    publishRFID(uid);
    // Si hay lectura RFID, auto-iniciar ciclo
    if (!cycleActive) {
      cycleActive = true;
      cycleStart = millis();
      cycleEnergyWh = 0.0;
      currentPartId = uid;
      Serial.printf("[AUTO-CYCLE] Ciclo iniciado con RFID: %s\n", uid.c_str());
    }
  }

  // Botón físico para terminar ciclo
  if (digitalRead(CYCLE_BTN_PIN) == LOW && cycleActive) {
    delay(50);  // debounce
    if (digitalRead(CYCLE_BTN_PIN) == LOW) {
      publishCycleEnd();
      cycleActive = false;
      currentPartId = "";
      delay(500);
    }
  }

  delay(10);
}
