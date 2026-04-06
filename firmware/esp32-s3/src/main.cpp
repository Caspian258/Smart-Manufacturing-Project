/*
 * main.cpp — IoT Gateway Planta 1: CIMA
 * Modo: SIMULACIÓN — funciona sin circuito físico conectado
 *
 * Publica datos simulados de energía cada 5 segundos via MQTT.
 * Cuando tengas el circuito (SCT-013 + RC522), cambiar:
 *   #define SIMULATION_MODE 0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <time.h>

#include "config.h"

// ─── Modo simulación ─────────────────────────────────────────────────────────
// 1 = datos simulados (sin circuito físico)
// 0 = datos reales    (con SCT-013 y RC522)
#define SIMULATION_MODE 1

#if SIMULATION_MODE == 0
#include <SPI.h>
#include <MFRC522.h>
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
#endif

// ─── Clientes globales ────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ─── Estado del ciclo ────────────────────────────────────────────────────────
struct CycleState {
  bool     active    = false;
  uint32_t startMs   = 0;
  float    energyWh  = 0.0f;
  String   partId    = "";
};
CycleState cycle;

uint32_t lastPublish   = 0;
uint32_t lastHeartbeat = 0;

// ─── Utilidades ──────────────────────────────────────────────────────────────
String isoTimestamp() {
  struct tm t;
  if (!getLocalTime(&t)) return "1970-01-01T00:00:00Z";
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(buf);
}

void ledBlink(uint8_t times, uint16_t ms = 100) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH); delay(ms);
    digitalWrite(PIN_LED, LOW);  delay(ms);
  }
}

// ─── Datos simulados ─────────────────────────────────────────────────────────
#if SIMULATION_MODE == 1
struct PowerReading {
  float irmsA;
  float powerW;
  float energyKwh;
};

PowerReading measurePower() {
  // Simular variación realista de una torno industrial
  static float base = 1200.0f;
  float noise = (random(-200, 200)) / 10.0f;
  float power = base + noise;
  float irms  = power / (220.0f * 0.85f);
  float energy = power * (PUBLISH_INTERVAL_MS / 1000.0f) / 3600000.0f;
  return { irms, power, energy };
}

String readRFID() { return ""; }  // Sin RFID en simulación
#endif

// ─── Datos reales (SCT-013) ───────────────────────────────────────────────────
#if SIMULATION_MODE == 0
struct PowerReading {
  float irmsA;
  float powerW;
  float energyKwh;
};

PowerReading measurePower() {
  int64_t sumSq = 0;
  const int offset = 2048;
  for (int n = 0; n < 1480; n++) {
    int raw = analogRead(PIN_SCT) - offset;
    sumSq += (int64_t)raw * raw;
    delayMicroseconds(10);
  }
  float irms = sqrtf((float)sumSq / 1480) / 4096.0f * 3.3f / 33.0f * 100.0f * 0.909f;
  if (irms < 0.05f) irms = 0.0f;
  float power  = irms * 220.0f * 0.85f;
  float energy = power * (PUBLISH_INTERVAL_MS / 1000.0f) / 3600000.0f;
  return { irms, power, energy };
}

String readRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";
  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i) uid += ':';
    if (rfid.uid.uidByte[i] < 0x10) uid += '0';
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return uid;
}
#endif

// ─── Publicación MQTT ────────────────────────────────────────────────────────
void publishEnergy(float irms, float power, float energy) {
  JsonDocument doc;
  doc["machine_id"]   = MACHINE_ID;
  doc["irms_a"]       = roundf(irms * 100) / 100.0f;
  doc["power_w"]      = roundf(power);
  doc["energy_kwh"]   = energy;
  doc["cycle_active"] = cycle.active;
  doc["simulated"]    = (SIMULATION_MODE == 1);
  if (cycle.active) doc["part_id"] = cycle.partId;
  doc["ts"]           = isoTimestamp();
  doc["rssi"]         = WiFi.RSSI();

  char topic[64];
  snprintf(topic, sizeof(topic), "cima/machines/%s/energy", MACHINE_ID);
  char payload[256];
  serializeJson(doc, payload);
  mqtt.publish(topic, payload, true);

  if (cycle.active) cycle.energyWh += energy * 1000.0f;

  Serial.printf("[ENERGY] %.0fW  %.2fA  %s\n",
    power, irms, SIMULATION_MODE ? "(SIM)" : "(REAL)");
}

void publishHeartbeat() {
  JsonDocument doc;
  doc["device"]   = CLIENT_ID;
  doc["machine"]  = MACHINE_ID;
  doc["ip"]       = WiFi.localIP().toString();
  doc["rssi"]     = WiFi.RSSI();
  doc["uptime"]   = millis() / 1000;
  doc["mode"]     = SIMULATION_MODE ? "simulation" : "real";
  doc["ts"]       = isoTimestamp();
  char payload[200];
  serializeJson(doc, payload);
  mqtt.publish(TOPIC_HEARTBEAT, payload);
  Serial.println("[HEARTBEAT] enviado");
}

// ─── MQTT callback ────────────────────────────────────────────────────────────
void onMqttMessage(char* topic, uint8_t* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.printf("[MQTT IN] %s: %s\n", topic, msg.c_str());

  if (String(topic) == TOPIC_CMD_CYCLE) {
    JsonDocument cmd;
    if (deserializeJson(cmd, msg) != DeserializationError::Ok) return;
    const char* action = cmd["action"] | "";
    if (strcmp(action, "start") == 0 && !cycle.active) {
      cycle.active   = true;
      cycle.startMs  = millis();
      cycle.energyWh = 0.0f;
      cycle.partId   = cmd["part_id"] | "SIM-001";
      Serial.printf("[CYCLE] Iniciado: %s\n", cycle.partId.c_str());
      ledBlink(2, 200);
    } else if (strcmp(action, "stop") == 0 && cycle.active) {
      cycle.active = false;
      Serial.printf("[CYCLE] Terminado. Energía: %.2f Wh\n", cycle.energyWh);
    }
  }
}

// ─── Reconexión MQTT ──────────────────────────────────────────────────────────
void reconnectMQTT() {
  uint8_t attempts = 0;
  while (!mqtt.connected() && attempts < 3) {
    Serial.printf("[MQTT] Conectando a %s...\n", MQTT_BROKER);
    if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      mqtt.subscribe(TOPIC_CMD_CYCLE);
      Serial.println("[MQTT] Conectado OK");
      ledBlink(4, 80);
    } else {
      Serial.printf("[MQTT] Fallo rc=%d — reintento en 5s\n", mqtt.state());
      delay(5000);
      attempts++;
    }
  }
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n════════════════════════════════");
  Serial.println(" CIMA IoT Gateway");
  Serial.printf(" Modo: %s\n", SIMULATION_MODE ? "SIMULACION" : "REAL");
  Serial.printf(" Maquina: %s\n", MACHINE_ID);
  Serial.println("════════════════════════════════\n");

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_CYCLE_BTN, INPUT_PULLUP);

#if SIMULATION_MODE == 0
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("[RFID] RC522 inicializado");
#else
  Serial.println("[SIM] Sensores físicos desactivados");
#endif

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Conectando");
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 24) {
    delay(500); Serial.print('.'); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] OK → IP: %s\n", WiFi.localIP().toString().c_str());
    ledBlink(5, 80);
  } else {
    Serial.println("\n[WiFi] Sin conexión — verificar credenciales en .env");
  }

  // NTP
  configTime(NTP_UTC_OFFSET_SEC, 0, NTP_SERVER);
  Serial.println("[NTP] Sincronizando...");

  // OTA
  ArduinoOTA.setHostname(CLIENT_ID);
  ArduinoOTA.begin();

  // MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  Serial.println("[READY] Sistema listo\n");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  ArduinoOTA.handle();

  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  uint32_t now = millis();

  // Publicar energía cada 5 segundos
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    auto p = measurePower();
    publishEnergy(p.irmsA, p.powerW, p.energyKwh);
  }

  // Heartbeat cada 30 segundos
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = now;
    publishHeartbeat();
  }

  // RFID solo en modo real
#if SIMULATION_MODE == 0
  String uid = readRFID();
  if (!uid.isEmpty() && !cycle.active) {
    cycle.active   = true;
    cycle.startMs  = millis();
    cycle.energyWh = 0.0f;
    cycle.partId   = uid;
    Serial.printf("[RFID] Pieza detectada: %s\n", uid.c_str());
  }
#endif

  // Botón BOOT = simular scan RFID (solo en modo simulación)
#if SIMULATION_MODE == 1
  if (digitalRead(PIN_CYCLE_BTN) == LOW) {
    delay(50);
    if (digitalRead(PIN_CYCLE_BTN) == LOW && !cycle.active) {
      cycle.active   = true;
      cycle.startMs  = millis();
      cycle.energyWh = 0.0f;
      cycle.partId   = "SIM-" + String(millis() % 1000);
      Serial.printf("[SIM] Ciclo iniciado: %s\n", cycle.partId.c_str());
      ledBlink(3, 100);
      delay(300);
    } else if (digitalRead(PIN_CYCLE_BTN) == LOW && cycle.active) {
      cycle.active = false;
      Serial.printf("[SIM] Ciclo terminado. Energía: %.2f Wh\n", cycle.energyWh);
      delay(300);
    }
  }
#endif

  delay(10);
}