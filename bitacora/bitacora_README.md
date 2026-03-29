# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #005 — Dashboard Node-RED operativo con KPIs en vivo
- **Fecha**: 2026-03-29
- **Estado**: ✅ Completado — HITO FASE 1
- **Acción**: Importado `flows/nodered/nodered_dashboard.json` en Node-RED. Dashboard funcionando correctamente con visualización en tiempo real de KPIs de energía, OEE y estado del sistema.

### Estado del dashboard
| Panel | Estado |
|-------|--------|
| Gauge — Potencia actual (W) | ✅ en vivo |
| Gráfica histórica — Consumo kW | ✅ en vivo |
| Estado ciclo productivo | ✅ en vivo |
| Modo operación (sim / real) | ✅ en vivo |
| Estado Gateway IoT (heartbeat) | ✅ en vivo |
| OEE Global (%) | ✅ calculado cada 60s |
| Disponibilidad / Rendimiento / Calidad | ✅ en vivo |

### Archivos involucrados
- `flows/nodered/nodered_dashboard.json`

### Próximo paso
- Configurar Grafana con datasource InfluxDB para persistencia histórica de KPIs

---

## ENTRADA #004 — Pipeline completo: ESP32 → MQTT → n8n → InfluxDB
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado — HITO FASE 1
- **Acción**: Flujo n8n importado y activo. Datos del ESP32-S3 ahora persisten en InfluxDB automáticamente.

### Estado del pipeline extremo a extremo
| Componente | Estado |
|-----------|--------|
| ESP32-S3 publicando datos | ✅ cada 5s |
| MQTT Broker | ✅ recibiendo |
| n8n workflow | ✅ procesando |
| InfluxDB guardando | ✅ |

### Próximo paso
- Configurar dashboard Grafana con datasource InfluxDB para visualizar KPIs en tiempo real

---

## ENTRADA #003 — Primer dato MQTT recibido: ESP32-S3 → RPi5 operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado — HITO FASE 1
- **Acción**: Firmware flasheado en ESP32-S3 (torno). Primer dato MQTT publicado y recibido correctamente por el broker en RPi5 (192.168.100.168:1883). Sistema IoT extremo a extremo funcional.

### Estado del stack al cierre de sesión
| Componente | Estado |
|-----------|--------|
| GitHub repo | ✅ sincronizado |
| Firmware ESP32-S3 | ✅ publicando datos de energía cada 5s |
| MQTT Broker (Mosquitto) | ✅ recibiendo datos desde ESP32 |
| InfluxDB 2.7 | ✅ listo para guardar (pendiente flujo n8n) |
| Node-RED | ✅ listo para dashboard |
| n8n | ✅ listo para flujos |

### Cambios técnicos en esta sesión
- `main.cpp` refactorizado: `SIMULATION_MODE` toggle, `JsonDocument` (ArduinoJson v7), lectura SCT-013 real + modo simulación realista de fresadora 1200W
- Credenciales movidas a guards `#ifndef` — inyección limpia desde `.env` vía `inject_env.py`
- `.env` actualizado: broker → `192.168.100.168`, machine_id → `torno`

### Archivos modificados
- `firmware/esp32-s3/src/main.cpp`
- `firmware/esp32-s3/.env`
- `bitacora/bitacora_README.md`

---

## ENTRADA #002 — Stack RPi5 completamente operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado
- **Resumen**: Después de resolver problemas de compatibilidad con Debian 13 Trixie, todos los servicios están activos.

### Servicios instalados y corriendo
| Servicio | Puerto | Método | Estado |
|---------|--------|--------|--------|
| Mosquitto (MQTT Broker) | 1883 | apt | ✅ active |
| InfluxDB 2.7 | 8086 | binario directo | ✅ active |
| Node-RED | 1880 | npm global | ✅ active |
| n8n | 5678 | Docker | ✅ active |

### Problemas encontrados y soluciones
- **Mosquitto**: faltaba `/etc/mosquitto/mosquitto.conf` — reinstalación con `apt purge`
- **InfluxDB**: repo oficial no soporta Debian Trixie — instalado desde binario `.tar.gz`
- **n8n via npm**: Python 3.13 rompía `node-gyp` — resuelto con Docker
- **influx CLI**: binario separado, descargado `influxdb2-client` por separado

### InfluxDB configurado
- Org: `smart-manufacturing` | Bucket: `sensor-data` (30 días)

---

## ENTRADA #001 — Estructura del repositorio
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado

---

## PENDIENTES — Fase 1 (semanas 1-6)
- [x] Flashear ESP32-S3 y verificar mensajes MQTT llegando al RPi5 ✅
- [x] Importar flujo `flows/n8n/mqtt_to_influxdb.json` en n8n ✅
- [x] Construir dashboard Node-RED con KPIs (energía, OEE, trazabilidad) ✅
- [ ] Conectar SCT-013 al ESP32-S3 (diagrama en docs/)
- [ ] Probar RC522 RFID — Pasaporte Digital primera pieza
- [ ] Configurar Grafana con datasource InfluxDB

## PENDIENTES — Fase 2 (semanas 7-12)
- [ ] Integrar OpenClaw (cobot)
- [ ] Flujo RFID completo Planta 2 — Celda 3105
- [ ] Quality Gate con Cognex Vision

## PENDIENTES — Fase 3 (semanas 13-17)
- [ ] Detección de anomalías con TFLite en RPi5
- [ ] Gemelo Digital vivo en Node-RED
- [ ] Auto-reportes gerenciales

---

## HISTORIAL DE VERSIONES
| Versión | Fecha | Descripción |
|---------|-------|-------------|
| 0.5.0 | 2026-03-29 | Dashboard Node-RED operativo con KPIs en vivo |
| 0.4.0 | 2026-03-28 | Pipeline completo ESP32 → MQTT → n8n → InfluxDB |
| 0.3.0 | 2026-03-28 | Primer dato MQTT recibido — ESP32 → RPi5 operativo |
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
