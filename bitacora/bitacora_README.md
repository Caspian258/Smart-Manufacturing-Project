# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #006 — Grafana operativo con KPIs en tiempo real
- **Fecha**: 2026-03-29
- **Estado**: ✅ Completado — HITO FASE 1
- **Acción**: Grafana instalado en RPi5, datasource InfluxDB conectado y dashboard de KPIs importado. Datos del ESP32 visibles en tiempo real.

### Problemas encontrados y soluciones
| Problema | Solución |
|---------|---------|
| `musl` faltaba como dependencia | `sudo apt-get install -y musl && sudo dpkg --configure grafana` |
| Puerto 3000 redirigido por regla Docker a `172.17.0.2:8080` | Grafana movido al puerto 3001 en `grafana.ini` |
| Grafana no accesible desde red local | Puerto 3001 abierto, acceso vía `192.168.100.168:3001` |

### Estado del dashboard
| Panel | Estado |
|-------|--------|
| Gauge — Potencia actual (W) | ✅ datos en vivo |
| Gauge — Corriente Irms (A) | ✅ datos en vivo |
| Historial Potencia (última hora) | ✅ datos en vivo |
| Energía Acumulada (kWh) | ✅ datos en vivo |
| Modo Operación (Sim / Real) | ✅ datos en vivo |
| Trazabilidad — Ciclos Productivos | ✅ datos en vivo |
| Alertas — Anomalías de Consumo (>3000W) | ✅ datos en vivo |

### Archivos agregados al repo
- `dashboard/grafana/grafana_datasource.yaml`
- `dashboard/grafana/grafana_dashboard.json`

### Configuración Grafana
- Puerto: **3001** (3000 ocupado por Docker)
- URL: `http://192.168.100.168:3001`
- Datasource: InfluxDB Flux → `smart-manufacturing` / `sensor-data`
- Auto-refresh: 10s

### Próximo paso
- Conectar SCT-013 al ESP32-S3 (pasar de simulación a datos reales)
- Probar RC522 RFID — Pasaporte Digital primera pieza

---

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

---

## ENTRADA #003 — Primer dato MQTT recibido: ESP32-S3 → RPi5 operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado — HITO FASE 1
- **Acción**: Firmware flasheado en ESP32-S3 (torno). Primer dato MQTT publicado y recibido correctamente por el broker en RPi5 (192.168.100.168:1883). Sistema IoT extremo a extremo funcional.

### Cambios técnicos en esta sesión
- `main.cpp` refactorizado: `SIMULATION_MODE` toggle, `JsonDocument` (ArduinoJson v7)
- Credenciales movidas a guards `#ifndef` — inyección limpia desde `.env` vía `inject_env.py`
- `.env` actualizado: broker → `192.168.100.168`, machine_id → `torno`

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
- [x] Construir dashboard Node-RED con KPIs (energía, OEE, trazabilidad) ✅
- [x] Configurar Grafana con datasource InfluxDB ✅
- [ ] Conectar SCT-013 al ESP32-S3 (diagrama en docs/)
- [ ] Probar RC522 RFID — Pasaporte Digital primera pieza

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
| 0.6.0 | 2026-03-29 | Grafana operativo con KPIs en tiempo real |
| 0.5.0 | 2026-03-29 | Dashboard Node-RED operativo con KPIs en vivo |
| 0.4.0 | 2026-03-28 | Pipeline completo ESP32 → MQTT → n8n → InfluxDB |
| 0.3.0 | 2026-03-28 | Primer dato MQTT recibido — ESP32 → RPi5 operativo |
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
