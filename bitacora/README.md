# Bitácora — Smart Manufacturing

## ENTRADA #001 — Estructura del repositorio corregida
- **Fecha**: $(date '+%Y-%m-%d')
- **Acción**: Reorganización de archivos a estructura PlatformIO
- **Estado**: ✅ Completado
- **Próximo paso**: Instalar PlatformIO en VS Code y compilar firmware

## ENTRADA #002 — Firmware ESP32-S3 compilado exitosamente
- **Fecha**: 2026-03-28
- **Acción**: Corrección de conflicto macros vs. variables en `main.cpp` (WIFI_SSID, MQTT_BROKER, etc.). Se migraron declaraciones a guards `#ifndef` para compatibilidad con `inject_env.py`. Se actualizó `.env` con credenciales reales (WiFi: Totalplay-3CAF, Broker: 10.90.159.4).
- **Estado**: ✅ Compilado — SUCCESS en 10.3s
- **Archivos modificados**: `firmware/esp32-s3/src/main.cpp`, `firmware/esp32-s3/.env`
- **Métricas**: RAM 13.8% (45 KB), Flash 23.8% (796 KB)
- **Próximo paso**: Flashear al ESP32-S3 con `pio run --target upload`

## PENDIENTES
- [ ] Ejecutar setup_rpi5.sh en RPi5
- [x] Copiar .env.template a .env y completar credenciales ✅
- [x] Compilar firmware: cd firmware/esp32-s3 && pio run ✅
- [ ] Flashear ESP32-S3: pio run --target upload
- [ ] Importar mqtt_to_influxdb.json en n8n
