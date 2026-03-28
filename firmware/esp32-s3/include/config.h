#pragma once
#ifndef WIFI_SSID
  #define WIFI_SSID     "FALLBACK_SSID"
#endif
#ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD "FALLBACK_PASS"
#endif
#ifndef MQTT_BROKER
  #define MQTT_BROKER   "192.168.1.100"
#endif
#ifndef MQTT_PORT
  #define MQTT_PORT     1883
#endif
#ifndef MQTT_USER
  #define MQTT_USER     "esp32_cima"
#endif
#ifndef MQTT_PASSWORD
  #define MQTT_PASSWORD "changeme"
#endif
#ifndef MACHINE_ID
  #define MACHINE_ID    "fresadora"
#endif
#define CLIENT_ID       "esp32-cima-" MACHINE_ID
#define PIN_SCT         36
#define PIN_RFID_SS     5
#define PIN_RFID_RST    27
#define PIN_LED         48
#define PIN_CYCLE_BTN   0
#define SCT_SAMPLES     1480
#define PUBLISH_INTERVAL_MS   5000
#define HEARTBEAT_INTERVAL_MS 30000
#define NTP_SERVER            "pool.ntp.org"
#define NTP_UTC_OFFSET_SEC    (-6 * 3600)
#define TOPIC_HEARTBEAT       "cima/system/heartbeat"
#define TOPIC_RFID            "cima/rfid/scan"
#define TOPIC_CMD_CYCLE       "cima/commands/cycle"
#ifdef DEBUG_MODE
  #define LOG(fmt, ...) Serial.printf("[%lu] " fmt "\n", millis(), ##__VA_ARGS__)
#else
  #define LOG(fmt, ...)
#endif
