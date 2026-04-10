# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

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
- [ ] Cerrar puerto 1883 una vez validado TLS
- [ ] HashiCorp Vault para gestión de secretos
- [ ] IDS MQTT — agente de detección de intrusos
- [ ] Autenticación reforzada en todos los servicios

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
