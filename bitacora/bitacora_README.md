# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #002 — Stack RPi5 completamente operativo
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado
- **Resumen**: Después de resolver problemas de compatibilidad con
  Debian 13 Trixie, todos los servicios están activos.

### Servicios instalados y corriendo
| Servicio | Puerto | Método | Estado |
|---------|--------|--------|--------|
| Mosquitto (MQTT Broker) | 1883 | apt | ✅ active |
| InfluxDB 2.7 | 8086 | binario directo | ✅ active |
| Node-RED | 1880 | npm global | ✅ active |
| n8n | 5678 | Docker | ✅ active |

### Problemas encontrados y soluciones
- **Mosquitto**: faltaba `/etc/mosquitto/mosquitto.conf` y usuario
  del sistema — reinstalación limpia con `apt purge` lo resolvió
- **InfluxDB**: repo oficial no soporta Debian Trixie (sqv/GPG issue)
  — instalado desde binario `.tar.gz` con servicio systemd manual
- **n8n via npm**: Python 3.13 eliminó `distutils`, rompía
  `node-gyp` — resuelto instalando vía Docker
- **influx CLI**: binario separado del servidor, descargado
  `influxdb2-client` por separado

### Usuarios MQTT creados
- `smfg_user` / `SmartMfg2024!` — usuario principal del sistema
- `esp32_cima` / `Esp32Cima!` — gateway IoT Planta 1

### InfluxDB configurado
- Org: `smart-manufacturing`
- Bucket: `sensor-data` (retención 30 días)
- Admin: `admin` / `SmartMfg2024!`

---

## ENTRADA #001 — Estructura del repositorio
- **Fecha**: 2026-03-28
- **Estado**: ✅ Completado
- **Archivos creados**: README.md, CLAUDE.md, platformio.ini,
  main.cpp, config.h, inject_env.py, mqtt_to_influxdb.json,
  setup_rpi5.sh, auto_push.yml

---

## PENDIENTES — Fase 1 (semanas 1-6)
- [ ] Flashear ESP32-S3 y verificar mensajes MQTT llegando al RPi5
- [ ] Importar flujo `flows/n8n/mqtt_to_influxdb.json` en n8n
- [ ] Construir dashboard Node-RED con KPIs (energía, OEE, trazabilidad)
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
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
