/*
 * main.cpp — IoT Gateway Planta 1: CIMA
 * Modo: SIMULACIÓN — funciona sin circuito físico conectado
 *
 * Publica datos simulados de energía cada 5 segundos via MQTT.
 * Cuando tengas el circuito (SCT-013 + RC522), cambiar:
 *   #define SIMULATION_MODE 0
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <time.h>

#include "config.h"
#include "ca_cert.h"

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
#if MQTT_TLS
WiFiClientSecure wifiClient;
#else
WiFiClient       wifiClient;
#endif
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
  char payload[320];
  serializeJson(doc, payload);
  if (!mqtt.publish(topic, payload, true)) {
    Serial.printf("[MQTT] publish falló: %s\n", topic);
  }

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
  if (!mqtt.publish(TOPIC_HEARTBEAT, payload)) {
    Serial.println("[MQTT] heartbeat publish falló");
  } else {
    Serial.println("[HEARTBEAT] enviado");
  }
}

// ─── MQTT callback ────────────────────────────────────────────────────────────
void onMqttMessage(char* topic, uint8_t* payload, unsigned int len) {
  String msg;
  msg.reserve(len);
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

// ─── Helpers WiFi ────────────────────────────────────────────────────────────
#ifdef WIFI_BSSID
static uint8_t s_bssid[6];
static bool    s_bssid_parsed = false;

static bool parseBSSID(const char* str, uint8_t out[6]) {
  return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
    &out[0],&out[1],&out[2],&out[3],&out[4],&out[5]) == 6;
}
#endif

static void wifiBegin() {
#ifdef WIFI_BSSID
  if (!s_bssid_parsed)
    s_bssid_parsed = parseBSSID(WIFI_BSSID, s_bssid);
  if (s_bssid_parsed) {
#ifdef WIFI_CHANNEL
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, s_bssid);
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 0, s_bssid);
#endif
    return;
  }
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ─── Reconexión WiFi ─────────────────────────────────────────────────────────
void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("[WiFi] Reconectando");
  WiFi.disconnect();
  wifiBegin();
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 3) {
    delay(500);
    Serial.print('.');
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Reconectado → IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Sin conexión — continuando sin red");
  }
}

// ─── Reconexión MQTT ──────────────────────────────────────────────────────────
void reconnectMQTT() {
#if MQTT_TLS
  // Re-sincronizar NTP si el tiempo no es válido — necesario para validar certs TLS
  if (time(nullptr) < 1000000000UL) {
    configTime(NTP_UTC_OFFSET_SEC, 0, NTP_SERVER);
    Serial.print("[NTP] Re-sincronizando");
    uint32_t t0 = millis();
    while (time(nullptr) < 1000000000UL && millis() - t0 < 10000) {
      delay(200); Serial.print('.'); esp_task_wdt_reset();
    }
    Serial.println(time(nullptr) >= 1000000000UL ? " OK" : " TIMEOUT");
  }
#endif
  uint8_t attempts = 0;
  while (!mqtt.connected() && attempts < 3) {
#if MQTT_TLS
    wifiClient.setInsecure();
#endif
    Serial.printf("[MQTT] Conectando a %s...\n", MQTT_BROKER);
    if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      mqtt.subscribe(TOPIC_CMD_CYCLE);
      Serial.println("[MQTT] Conectado OK");
      ledBlink(4, 80);
    } else {
      Serial.printf("[MQTT] Fallo rc=%d — reintento en 5s\n", mqtt.state());
#if MQTT_TLS
      char sslErr[128];
      wifiClient.lastError(sslErr, sizeof(sslErr));
      Serial.printf("[TLS] mbedTLS: %s\n", sslErr);
      Serial.printf("[TLS] time()=%lu (esperado ~1775834000+)\n", (unsigned long)time(nullptr));
#endif
      for (uint8_t i = 0; i < 10; i++) { delay(500); esp_task_wdt_reset(); }
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
  wifiBegin();
  Serial.print("[WiFi] Conectando");
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500); Serial.print('.'); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] OK → IP: %s\n", WiFi.localIP().toString().c_str());
    ledBlink(5, 80);
  } else {
    Serial.println("\n[WiFi] Sin conexión — verificar credenciales en .env");
  }

  // Si WiFi aún no conectó, esperar hasta 15s más antes de NTP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("[WiFi] Esperando");
    uint32_t wStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wStart < 15000) {
      delay(500); Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED)
      Serial.printf(" OK → IP: %s\n", WiFi.localIP().toString().c_str());
    else
      Serial.println(" TIMEOUT");
  }

  // NTP — esperar sincronización antes de validar certificados TLS
  configTime(NTP_UTC_OFFSET_SEC, 0, NTP_SERVER);
  Serial.print("[NTP] Sincronizando");
  {
    uint32_t ntpStart = millis();
    time_t now = 0;
    while (now < 1000000000UL && millis() - ntpStart < 15000) {
      delay(200);
      Serial.print('.');
      now = time(nullptr);
    }
    if (now >= 1000000000UL) {
      struct tm t;
      gmtime_r(&now, &t);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
      Serial.printf("\n[NTP] OK → %s\n", buf);
    } else {
      Serial.println("\n[NTP] TIMEOUT — se reintentará antes de conectar MQTT");
    }
  }

  // OTA
  ArduinoOTA.setHostname(CLIENT_ID);
  ArduinoOTA.begin();

  // MQTT — TLS
#if MQTT_TLS
  wifiClient.setHandshakeTimeout(30);
  // TODO: setCACert() falla en ESP32 Arduino 3.x / ESP-IDF 5.x con CA custom
  // (bug conocido de mbedTLS: comparación estricta ASN.1 de nombres)
  // La conexión TLS sigue cifrada; auth via usuario/contraseña MQTT
  wifiClient.setInsecure();
  Serial.println("[TLS] Modo: cifrado TLS sin verificación CA (ver TODO)");
#endif
  // Usar IPAddress en lugar de string para evitar bug de IP SAN en mbedTLS
  // (ESP32 Arduino 3.x / ESP-IDF 5.x no reconoce IP strings para SAN matching)
  IPAddress mqttIp;
  if (!mqttIp.fromString(MQTT_BROKER)) {
    WiFi.hostByName(MQTT_BROKER, mqttIp);
  }
  mqtt.setServer(mqttIp, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  Serial.println("[READY] Sistema listo\n");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) reconnectWiFi();
  if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  uint32_t now = millis();

  if (mqtt.connected()) {
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