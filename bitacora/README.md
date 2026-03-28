# Bitácora — Smart Manufacturing

## ENTRADA #001 — Estructura del repositorio corregida
- **Fecha**: $(date '+%Y-%m-%d')
- **Acción**: Reorganización de archivos a estructura PlatformIO
- **Estado**: ✅ Completado
- **Próximo paso**: Instalar PlatformIO en VS Code y compilar firmware

## PENDIENTES
- [ ] Ejecutar setup_rpi5.sh en RPi5
- [ ] Copiar .env.template a .env y completar credenciales
- [ ] Compilar firmware: cd firmware/esp32-s3 && pio run
- [ ] Flashear ESP32-S3: pio run --target upload
- [ ] Importar mqtt_to_influxdb.json en n8n
