# Bitácora — Smart Manufacturing CIMA + Celda 3105
# Orquestador: Claude Code
# ═══════════════════════════════════════════════════════════════════════

## ENTRADA #009 — Fase A iniciada: Modbus TCP en TIA Portal
- **Fecha**: 2026-04-09
- **Estado**: 🔄 En progreso
- **Acción**: Inicio de implementación de integración PLC S7-1200 → RPi5 por Modbus TCP. Primera tarea: configurar MB_SERVER en TIA Portal y validar conectividad.

### Objetivo de la sesión
Completar Fase A del plan de integración Celda 3105:
- Habilitar Modbus TCP en S7-1200 (bloque MB_SERVER, puerto 502)
- Definir mapa de registros KPIs en `config/plc_registers.md`
- Instalar `node-red-contrib-modbus` en RPi5
- Primer dato PLC → InfluxDB

### Próximo paso
Seguir instrucciones de Fase A1 → A2 → A3 en orden.

---

## ENTRADA #008 — Plan de integración PLC S7-1200 + Control de acceso Celda 3105
- **Fecha**: 2026-04-09
- **Estado**: 📋 Planificado — en ejecución
- **Acción**: Definido plan de trabajo completo para integrar el PLC Siemens S7-1200 de Celda 3105 al stack RPi5, incluyendo dashboard KPIs y control de acceso innovador con ESP32-CAM.

### Decisiones de diseño
| Decisión | Elección | Razón |
|---------|---------|-------|
| Protocolo PLC | Modbus TCP (→ OPC-UA en Fase 3) | Nativo S7-1200 sin licencia extra; rápido de implementar |
| Control de acceso | ESP32-CAM + reconocimiento facial | Sin cables al RPi5; WiFi; misma toolchain que ESP32-S3 |
| Almacenamiento accesos | InfluxDB tag `plant=celda3105` | Reutiliza infraestructura existente; visible en Grafana |

### Plan de fases aprobado

#### Fase A — Conexión PLC → RPi5 (prioridad inmediata)
| Tarea | Descripción | Entregable |
|------|-------------|-----------|
| A1 | Habilitar MB_SERVER en TIA Portal, puerto 502 | PLC respondiendo Modbus TCP |
| A2 | Instalar `node-red-contrib-modbus`, flujo polling 2 s | Datos PLC en Node-RED |
| A3 | Definir mapa de 6 KPIs como Holding Registers | `config/plc_registers.md` |

#### Fase B — Dashboard Grafana Celda 3105
| Tarea | Descripción | Entregable |
|------|-------------|-----------|
| B1 | Flujo Node-RED: PLC → InfluxDB (tag celda3105) | `flows/nodered/celda3105_plc.json` |
| B2 | Dashboard Grafana "Celda 3105" (6 paneles KPI) | `dashboard/celda3105_dashboard.json` |
| B3 | Dashboard maestro ambas plantas (OEE global) | `dashboard/maestro_dashboard.json` |

#### Fase C — Control de acceso ESP32-CAM (innovación)
| Tarea | Descripción | Entregable |
|------|-------------|-----------|
| C1 | Firmware ESP32-CAM: captura + publica MQTT | `firmware/esp32-cam/src/main.cpp` |
| C2 | Agente Python: face_recognition → habilita PLC | `agentes/face_access_agent.py` |
| C3 | Panel Grafana: log de accesos + alertas n8n | Panel en dashboard Celda 3105 |

#### Fase D — Cierre de sesión
| Tarea | Descripción |
|------|-------------|
| D1 | Actualizar bitácora, bump versión, commit + push |

### KPIs definidos para Celda 3105 (Holding Registers)
| HR | KPI | Unidad | Rango esperado |
|----|-----|--------|---------------|
| HR0 | Ciclos completados (sesión) | count | 0–9999 |
| HR1 | Tiempo de ciclo último | décimas de s | 50–500 |
| HR2 | Piezas OK acumuladas | count | 0–9999 |
| HR3 | Piezas NOK acumuladas | count | 0–999 |
| HR4 | Estado cobot (0=idle, 1=running, 2=fault) | enum | 0/1/2 |
| HR5 | Número de paros en sesión | count | 0–99 |

### Hardware adicional requerido
| Componente | Uso | Costo aprox. |
|-----------|-----|-------------|
| ESP32-CAM (módulo AI-Thinker) | Control de acceso inalámbrico | ~$8 USD |
| Alimentación 5V para ESP32-CAM | Panel PLC o adaptador USB | Ya disponible |

### Archivos nuevos a crear
```
config/plc_registers.md          ← Mapa Modbus del S7-1200
flows/nodered/celda3105_plc.json ← Flujo PLC → InfluxDB
firmware/esp32-cam/              ← Proyecto PlatformIO ESP32-CAM
  ├── platformio.ini
  ├── src/main.cpp
  └── .env.template
agentes/face_access_agent.py     ← Agente reconocimiento facial
dashboard/celda3105_dashboard.json
dashboard/maestro_dashboard.json
```

---

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
| 0.8.0 | 2026-04-09 | Plan integración PLC S7-1200 Celda 3105 + inicio Fase A |
| 0.7.0 | 2026-03-29 | Plan ciberseguridad e innovación definido |
| 0.6.0 | 2026-03-29 | Grafana operativo con KPIs en tiempo real |
| 0.5.0 | 2026-03-29 | Dashboard Node-RED operativo con KPIs en vivo |
| 0.4.0 | 2026-03-28 | Pipeline completo ESP32 → MQTT → n8n → InfluxDB |
| 0.3.0 | 2026-03-28 | Primer dato MQTT recibido — ESP32 → RPi5 operativo |
| 0.2.0 | 2026-03-28 | Stack RPi5 completamente operativo |
| 0.1.0 | 2026-03-28 | Estructura inicial del repositorio |
