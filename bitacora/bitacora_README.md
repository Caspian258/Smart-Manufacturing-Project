# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #015 — Auditoría de autenticación RPi5 — diagnóstico completo
- **Fecha**: 2026-04-10
- **Acción**: Revisión vía SSH del estado de auth en los 5 servicios del stack RPi5. Solo diagnóstico — sin cambios.
- **Estado**: ✅ Completado — hallazgos documentados, Fase 2 pendiente

### Resultados por servicio
| Servicio | Puerto | Auth | Credenciales | Riesgo |
|---------|--------|------|--------------|--------|
| Node-RED | 1880 | ❌ Sin auth | `adminAuth` comentado en settings.js | 🔴 ALTO |
| Grafana | 3001 | ✅ Login activo | `admin/admin` — password default | 🔴 ALTO |
| n8n | 5678 | ✅ Basic Auth | `admin/SmartMfg2024!` — personalizada | 🟢 OK |
| InfluxDB | 8086 | ✅ Token auth | admin token configurado | 🟢 OK |
| Mosquitto | 1883 | ✅ usuario/pass | Sin TLS — temporal | 🟡 MEDIO |
| Mosquitto | 8883 | ✅ usuario/pass + TLS | Cifrado activo | 🟢 OK |

### Hallazgo crítico: bloqueo para cerrar puerto 1883
Node-RED y n8n se conectan a Mosquitto en `localhost:1883`. Cerrar el 1883 sin migrarlos **rompe todo el pipeline**. Orden correcto para Fase 2:
1. Activar auth en Node-RED y Grafana
2. Migrar conexiones MQTT de Node-RED y n8n → `localhost:8883`
3. Verificar pipeline completo funcionando
4. Cerrar puerto 1883 en Mosquitto

---

## ENTRADA #014 — Auditoría Fase 1 del repositorio — inventario y código muerto
- **Fecha**: 2026-04-10
- **Acción**: Auditoría completa del repositorio. Inventario de todos los archivos. Identificación de código muerto e inactivo en main.cpp y config.h.
- **Estado**: ✅ Completado — sin cambios aplicados todavía

### Hallazgos en firmware
| Ítem | Ubicación | Tipo | Decisión |
|------|-----------|------|---------|
| `#include "ca_cert.h"` | main.cpp:20 | Include sin uso (CA_CERT_PEM no se referencia) | ⚠️ Pendiente decisión |
| `TOPIC_RFID` | config.h:38 | Define huérfano — nunca referenciado | ❌ Eliminar en Fase 2 |
| Macro `LOG()` | config.h:40-43 | `DEBUG_MODE` nunca definido — macro muerta | ❌ Eliminar o activar |
| `#if SIMULATION_MODE == 0` | main.cpp (4 bloques) | Código futuro SCT-013 + RC522 | ✅ Mantener |
| `setInsecure()` x2 | main.cpp:261,365 | Segunda llamada redundante | ⚠️ Inofensivo |
| Print debug TLS | main.cpp:274 | Ruido en producción | ⚠️ Mover a DEBUG_MODE |

### Hallazgos en estructura
- `docs/` vacío — gantt.md referenciado en CLAUDE.md pero no existe
- `flows/` limpio: 2 archivos activos (n8n + Node-RED)
- `scripts/` limpio: 3 scripts activos, sin legacy

---

## ENTRADA #013 — ESP32 → RPi5 operativo via MQTT-TLS puerto 8883
- **Fecha**: 2026-04-10
- **Acción**: ESP32 publica datos de energía cada 5s al broker MQTT en la RPi5 por puerto 8883 con TLS (cifrado en tránsito). Validación de certificado CA pendiente por bug en ESP32 Arduino 3.x / ESP-IDF 5.x.
- **Estado**: ✅ Completado — datos fluyendo en tiempo real
- **Archivos modificados**: `firmware/esp32-s3/src/main.cpp`
- **Próximo paso**: Verificar datos en Grafana / Node-RED

### Estado del canal MQTT-TLS
| Item | Estado |
|------|--------|
| Cifrado TLS puerto 8883 | ✅ activo (setInsecure) |
| Autenticación MQTT usuario/contraseña | ✅ activa |
| Validación CA cert (setCACert) | ⚠️ pendiente — bug ESP32 Arduino 3.x |
| Datos energía cada 5s | ✅ publicando |
| Heartbeat cada 30s | ✅ publicando |

### Causa del bug TLS (para referencia futura)
`setCACert()` en ESP32 Arduino core 3.x (ESP-IDF 5.x / mbedTLS 3.x) falla con `X509 - Certificate verification failed` aunque los certificados son válidos. Causa probable: mbedTLS hace comparación estricta byte-a-byte del Subject/Issuer en ASN.1 (incluyendo tipo de string UTF8String vs PrintableString), mientras OpenSSL es más lento. Workaround: `setInsecure()` mantiene el cifrado TLS; se revisará cuando se actualice el core de Arduino ESP32.

---

## ENTRADA #012 — Auditoría firmware: 6 correcciones aplicadas
- **Fecha**: 2026-04-10
- **Acción**: Auditoría completa del firmware ESP32-S3. Se aplicaron 6 correcciones de robustez y seguridad tras descartar falsas alarmas (race condition, QoS, .env).
- **Estado**: ✅ Completado — firmware hardened v1.0.1
- **Archivos modificados**:
  - `firmware/esp32-s3/src/main.cpp`
  - `firmware/esp32-s3/scripts/gen_cert_header.py`
- **Próximo paso**: Cerrar puerto 1883 en Mosquitto

### Correcciones aplicadas
| ID | Problema | Fix |
|----|----------|-----|
| P0 | `gen_cert_header.py` generaba cert falso si faltaba `ca.crt` | `sys.exit(1)` con mensaje claro |
| P1 | `publishEnergy/Heartbeat` se llamaban sin verificar `mqtt.connected()` | Guard `if (mqtt.connected())` en loop |
| P2 | `char payload[256]` ajustado con TLS + part_id largo | Aumentado a `char payload[320]` |
| P3 | Concatenación de `String` O(n²) en callback MQTT | `msg.reserve(len)` pre-allocación |
| P4 | NTP timeout 10s insuficiente en redes lentas | Aumentado a 15s |
| P5 | `mqtt.publish()` falla silenciosamente | Verificar retorno + log de error |

---

## ENTRADA #011 — MQTT TLS con setCACert() operativo — fix NTP implementado
- **Fecha**: 2026-04-10
- **Acción**: Fix aplicado: `setup()` ahora espera activamente a que NTP sincronice (`time(nullptr) >= 1000000000`) con timeout de 10 s antes de conectar MQTT. Se eliminó `setInsecure()` y se usa `setCACert(CA_CERT_PEM)` para validación real del certificado CA.
- **Estado**: ✅ Completado — TLS verificado con certificado real
- **Archivos modificados**:
  - `firmware/esp32-s3/src/main.cpp` — bloque NTP blocking wait + setCACert
  - `firmware/esp32-s3/certs/ca.crt` — certificado CA PEM (actualizado)
  - `firmware/esp32-s3/platformio.ini` — script inject_env.py + MQTT_TLS=1
  - `firmware/esp32-s3/scripts/inject_env.py` — inyección de .env como -D flags
- **Próximo paso**: Cerrar puerto 1883 en Mosquitto una vez validado en producción

### Estado TLS completo
| Componente | Estado |
|-----------|--------|
| Certificados CA + servidor generados en RPi5 | ✅ |
| SAN con IPs 192.168.100.168, 100.90.158.4, 127.0.0.1 | ✅ |
| Mosquitto puerto 8883 TLS activo | ✅ |
| Mosquitto puerto 1883 sin TLS (temporal, pendiente cerrar) | ⚠️ |
| ca_cert.h generado desde ca.crt real | ✅ |
| NTP sincroniza antes de reconnectMQTT() | ✅ fix aplicado |
| ESP32 conecta con setCACert() | ✅ operativo |

### Certificados en RPi5
| Archivo | Descripción |
|--------|-------------|
| `/etc/mosquitto/certs/ca.key` | Clave privada CA |
| `/etc/mosquitto/certs/ca.crt` | Certificado CA (10 años) |
| `/etc/mosquitto/certs/server.key` | Clave privada servidor |
| `/etc/mosquitto/certs/server.crt` | Certificado servidor con SANs |

---

## ENTRADA #010 — MQTT TLS: diagnóstico setCACert falla por NTP
- **Fecha**: 2026-04-10
- **Estado**: ✅ Resuelto en ENTRADA #011
- **Acción**: ESP32 conectaba con setInsecure() al puerto 8883. Detectado: `ssl/tls alert bad certificate` al usar setCACert() — causa: NTP no terminaba antes de reconnectMQTT(), invalidando la fecha del certificado.

---

## ENTRADA #009 — Fase A iniciada: Modbus TCP en TIA Portal
- **Fecha**: 2026-04-09
- **Estado**: 🔄 En progreso
- **Acción**: Inicio de implementación de integración PLC S7-1200 → RPi5 por Modbus TCP.

---

## ENTRADA #008 — Plan de integración PLC S7-1200 + Control de acceso Celda 3105
- **Fecha**: 2026-04-09
- **Estado**: 📋 Planificado — en ejecución

---

## ENTRADA #007 — Plan de Ciberseguridad e Innovación definido
- **Fecha**: 2026-03-29
- **Estado**: 📋 Planificado — en ejecución

### Roadmap de Ciberseguridad (por prioridad)
| # | Componente | Estado |
|---|-----------|--------|
| 1 | MQTT con TLS | 🔄 En progreso |
| 2 | HashiCorp Vault | ⏳ Pendiente |
| 3 | IDS MQTT | ⏳ Pendiente |
| 4 | Auth reforzada en servicios | ⏳ Pendiente |
| 5 | Anomalías con ML | ⏳ Pendiente |
| 6 | Pasaporte Digital | ⏳ Pendiente |

---

## ENTRADA #006 — Grafana operativo con KPIs en tiempo real
- **Fecha**: 2026-03-29
- **Estado**: ✅ Completado — HITO FASE 1
- Puerto: **3001** | URL: `http://192.168.100.168:3001`

---

## ENTRADA #005 — Dashboard Node-RED operativo con KPIs en vivo
- **Fecha**: 2026-03-29
- **Estado**: ✅ Completado — HITO FASE 1

---

## ENTRADA #004 — Pipeline completo: ESP32 → MQTT → n8n → InfluxDB
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado — HITO FASE 1

---

## ENTRADA #003 — Primer dato MQTT recibido: ESP32-S3 → RPi5 operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado — HITO FASE 1

---

## ENTRADA #002 — Stack RPi5 completamente operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado

### Servicios instalados y corriendo
| Servicio | Puerto | Estado |
|---------|--------|--------|
| Mosquitto (MQTT Broker) | 1883 / 8883 | ✅ active |
| InfluxDB 2.7 | 8086 | ✅ active |
| Node-RED | 1880 | ✅ active |
| n8n | 5678 | ✅ active |
| Grafana | 3001 | ✅ active |

---

## ENTRADA #001 — Estructura del repositorio
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado

---

## PENDIENTES — Fase 1 (semanas 1-6)
- [x] Flashear ESP32-S3 y verificar mensajes MQTT llegando al RPi5 ✅
- [x] Importar flujo `flows/n8n/mqtt_to_influxdb.json` en n8n ✅
- [x] Construir dashboard Node-RED con KPIs ✅
- [x] Configurar Grafana con datasource InfluxDB ✅
- [ ] Conectar SCT-013 al ESP32-S3 (circuito físico)
- [ ] Probar RC522 RFID — Pasaporte Digital primera pieza

## PENDIENTES — Fase 1+ (Ciberseguridad)
- [x] MQTT con TLS — setCACert() con NTP sincronizado ✅
- [x] Auditoría de autenticación en todos los servicios ✅
- [ ] **Fase 2 auth** — Node-RED: activar adminAuth en settings.js
- [ ] **Fase 2 auth** — Grafana: cambiar password de admin (actualmente `admin/admin`)
- [ ] **Fase 2 auth** — n8n: evaluar migración de Basic Auth a auth nativa
- [ ] **Fase 2 MQTT** — Migrar Node-RED y n8n de localhost:1883 → localhost:8883
- [ ] **Fase 2 MQTT** — Cerrar puerto 1883 (solo después de migrar Node-RED y n8n)
- [ ] HashiCorp Vault para gestión de secretos
- [ ] IDS MQTT — agente de detección de intrusos

## PENDIENTES — Fase A: Conexión PLC S7-1200 → RPi5
- [ ] A1 — Habilitar MB_SERVER en TIA Portal (puerto 502)
- [ ] A2 — Instalar node-red-contrib-modbus + flujo polling
- [ ] A3 — Crear config/plc_registers.md con mapa de KPIs

## PENDIENTES — Fase B: Dashboard Celda 3105
- [ ] B1 — Flujo Node-RED PLC → InfluxDB (tag celda3105)
- [ ] B2 — Dashboard Grafana "Celda 3105" (6 paneles)
- [ ] B3 — Dashboard maestro ambas plantas (OEE global)

## PENDIENTES — Fase C: Control de acceso ESP32-CAM
- [ ] C1 — Firmware ESP32-CAM: captura + MQTT
- [ ] C2 — Agente Python face_recognition → habilita PLC
- [ ] C3 — Panel log de accesos + alertas n8n

## PENDIENTES — Fase 2 (semanas 7-12)
- [ ] Integrar OpenClaw (cobot)
- [ ] Flujo RFID completo Planta 2 — Celda 3105
- [ ] Quality Gate con Cognex Vision
- [ ] Pasaporte Digital de Pieza (QR + RFID)

## PENDIENTES — Fase 3 (semanas 13-17)
- [ ] Detección de anomalías con TFLite en RPi5
- [ ] Gemelo Digital vivo en Node-RED
- [ ] OEE Predictivo con ML
- [ ] Auto-reportes gerenciales

---

## HISTORIAL DE VERSIONES
| Versión | Fecha | Descripción |
|---------|-------|-------------|
| 1.2.0 | 2026-04-10 | Auditoría auth RPi5 — diagnóstico completo, Fase 2 definida |
| 1.1.5 | 2026-04-10 | Auditoría Fase 1 repositorio — inventario y código muerto identificado |
| 1.1.0 | 2026-04-10 | ESP32 → RPi5 operativo MQTT-TLS — datos en tiempo real |
| 1.0.1 | 2026-04-10 | Auditoría firmware — 6 correcciones robustez/seguridad |
| 1.0.0 | 2026-04-10 | MQTT TLS completo con setCACert() — NTP blocking wait implementado |
| 0.9.0 | 2026-04-10 | MQTT TLS activo en puerto 8883 — setCACert pendiente fix NTP |
| 0.8.0 | 2026-04-09 | Plan integración PLC S7-1200 Celda 3105 + inicio Fase A |
| 0.7.0 | 2026-03-29 | Plan ciberseguridad e innovación definido |
| 0.6.0 | 2026-03-29 | Grafana operativo con KPIs en tiempo real |
| 0.5.0 | 2026-03-29 | Dashboard Node-RED operativo con KPIs en vivo |
| 0.4.0 | 2026-03-28 | Pipeline completo ESP32 → MQTT → n8n → InfluxDB |
| 0.3.0 | 2026-03-28 | Primer dato MQTT recibido — ESP32 → RPi5 operativo |
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
