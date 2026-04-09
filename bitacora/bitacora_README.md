# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #007 — Plan de Ciberseguridad e Innovación definido
- **Fecha**: 2026-03-29
- **Estado**: 📋 Planificado — pendiente implementación
- **Acción**: Definida hoja de ruta de ciberseguridad e innovación para Fase 1 mejorada y Fase 3.

### Roadmap de Ciberseguridad (por prioridad)
| # | Componente | Descripción | Fase |
|---|-----------|-------------|------|
| 1 | **MQTT con TLS** | Certificados autofirmados, cifrado ESP32 ↔ RPi5 | 1+ |
| 2 | **Vault (HashiCorp)** | Gestión centralizada de tokens y secretos | 1+ |
| 3 | **IDS MQTT** | Agente Python detecta topics/clientes anómalos | 1+ |
| 4 | **Auth reforzada** | Passwords seguros, auth en Node-RED, roles Grafana | 1+ |
| 5 | **Anomalías con ML** | TFLite detecta patrones inusuales en consumo | 3 |
| 6 | **Pasaporte Digital** | QR/RFID con historial completo por pieza | 2 |

### Roadmap de Innovación
| Componente | Descripción | Fase |
|-----------|-------------|------|
| Digital Twin en vivo | Modelo de planta en Node-RED con estado real por máquina | 3 |
| Detección de anomalías ML | TFLite entrenado con SCT-013, detecta fallas de motor | 3 |
| Pasaporte Digital de Pieza | Historial RFID: fabricante, energía, Quality Gate | 2 |
| OEE Predictivo | Predice caída de OEE basado en tendencias energéticas | 3 |

### Próxima sesión — arrancar con
1. MQTT TLS: generar CA + certificados en RPi5
2. Actualizar `main.cpp` para usar TLS en PubSubClient
3. Actualizar Mosquitto con `cafile`, `certfile`, `keyfile`

---

## ENTRADA #006 — Grafana operativo con KPIs en tiempo real
- **Fecha**: 2026-03-29
- **Estado**: ✅ Completado — HITO FASE 1

### Problemas encontrados y soluciones
| Problema | Solución |
|---------|---------|
| `musl` faltaba como dependencia | `sudo apt-get install -y musl && sudo dpkg --configure grafana` |
| Puerto 3000 redirigido por Docker | Grafana movido al puerto 3001 en `grafana.ini` |

### Configuración Grafana
- Puerto: **3001** | URL: `http://192.168.100.168:3001`
- Datasource: InfluxDB Flux → `smart-manufacturing` / `sensor-data`
- Auto-refresh: 10s

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
| Mosquitto (MQTT Broker) | 1883 | ✅ active |
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
- [ ] MQTT con TLS (certificados autofirmados)
- [ ] HashiCorp Vault para gestión de secretos
- [ ] IDS MQTT — agente de detección de intrusos
- [ ] Autenticación reforzada en todos los servicios

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
| 0.7.0 | 2026-03-29 | Plan ciberseguridad e innovación definido |
| 0.6.0 | 2026-03-29 | Grafana operativo con KPIs en tiempo real |
| 0.5.0 | 2026-03-29 | Dashboard Node-RED operativo con KPIs en vivo |
| 0.4.0 | 2026-03-28 | Pipeline completo ESP32 → MQTT → n8n → InfluxDB |
| 0.3.0 | 2026-03-28 | Primer dato MQTT recibido — ESP32 → RPi5 operativo |
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
