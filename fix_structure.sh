#!/bin/bash
# =============================================================================
# fix_structure.sh — Reorganiza el repositorio a la estructura correcta
# Ejecutar desde la raíz del repositorio (donde está CLAUDE.md)
# Uso: bash fix_structure.sh
# =============================================================================

set -e
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
log()     { echo -e "${GREEN}[✓]${NC} $1"; }
warn()    { echo -e "${YELLOW}[→]${NC} $1"; }
error()   { echo -e "${RED}[✗]${NC} $1"; }

echo ""
echo "━━━ Reorganizando estructura del repositorio ━━━"
echo ""

# ─── Verificar que estamos en la raíz correcta ────────────────────────────
if [ ! -f "CLAUDE.md" ]; then
  error "No encontré CLAUDE.md — ejecuta este script desde la raíz del repo"
  exit 1
fi

# ─── Crear carpetas ───────────────────────────────────────────────────────
warn "Creando estructura de carpetas..."
mkdir -p firmware/esp32-s3/src
mkdir -p firmware/esp32-s3/include
mkdir -p firmware/esp32-s3/lib
mkdir -p firmware/esp32-s3/test
mkdir -p firmware/esp32-s3/scripts
mkdir -p flows/n8n
mkdir -p flows/nodered
mkdir -p agentes
mkdir -p bitacora
mkdir -p dashboard
mkdir -p docs
mkdir -p scripts
mkdir -p config
mkdir -p .github/workflows
log "Carpetas creadas"

# ─── Mover archivos existentes al lugar correcto ─────────────────────────
warn "Moviendo archivos..."

# Firmware .ino → PlatformIO src/main.cpp
if [ -f "firmware_cima_iot.ino" ]; then
  mv firmware_cima_iot.ino firmware/esp32-s3/src/main.cpp
  log "firmware_cima_iot.ino → firmware/esp32-s3/src/main.cpp"
fi

# Flujo n8n
if [ -f "mqtt_to_influxdb.json" ]; then
  mv mqtt_to_influxdb.json flows/n8n/mqtt_to_influxdb.json
  log "mqtt_to_influxdb.json → flows/n8n/"
fi

# Script de instalación RPi5
if [ -f "setup_rpi5.sh" ]; then
  mv setup_rpi5.sh scripts/setup_rpi5.sh
  log "setup_rpi5.sh → scripts/"
fi

# ─── Crear archivos que faltan ────────────────────────────────────────────
warn "Creando archivos faltantes..."

# .gitignore
if [ ! -f ".gitignore" ]; then
cat > .gitignore << 'EOF'
.env
.env.local
secrets/
*.log
*.db
*.sqlite
.pio/
node_modules/
__pycache__/
*.pyc
.DS_Store
Thumbs.db
influxdb_data/
EOF
  log ".gitignore creado"
fi

# platformio.ini
if [ ! -f "firmware/esp32-s3/platformio.ini" ]; then
cat > firmware/esp32-s3/platformio.ini << 'EOF'
[platformio]
default_envs = debug

[env]
platform     = espressif32
board        = esp32-s3-devkitc-1
framework    = arduino
lib_deps =
    knolleary/PubSubClient @ ^2.8.0
    bblanchon/ArduinoJson  @ ^7.0.0
    miguelbalboa/MFRC522   @ ^1.4.10
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, colorize
build_flags =
    -DCORE_DEBUG_LEVEL=0
    !python3 scripts/inject_env.py

[env:debug]
build_type  = debug
build_flags = ${env.build_flags} -DDEBUG_MODE=1 -DCORE_DEBUG_LEVEL=3
upload_speed = 921600

[env:release]
build_type  = release
build_flags = ${env.build_flags} -DDEBUG_MODE=0 -Os
upload_speed = 921600

[env:ota]
extends         = env:release
upload_protocol = espota
upload_port     = ${sysenv.ESP32_IP}
EOF
  log "platformio.ini creado"
fi

# .env.template para el firmware
if [ ! -f "firmware/esp32-s3/.env.template" ]; then
cat > firmware/esp32-s3/.env.template << 'EOF'
WIFI_SSID=TU_RED_WIFI
WIFI_PASSWORD=TU_PASSWORD_WIFI
MQTT_BROKER=192.168.1.100
MQTT_PORT=1883
MQTT_USER=esp32_cima
MQTT_PASSWORD=Esp32Cima!
MACHINE_ID=fresadora
OTA_PASSWORD=SmartOTA2024!
EOF
  log ".env.template creado"
fi

# inject_env.py
if [ ! -f "firmware/esp32-s3/scripts/inject_env.py" ]; then
cat > firmware/esp32-s3/scripts/inject_env.py << 'PYEOF'
#!/usr/bin/env python3
import os, sys
from pathlib import Path

def find_env():
    p = Path(__file__).resolve().parent
    for _ in range(5):
        c = p / ".env"
        if c.exists(): return c
        p = p.parent
    return None

VARS = ["WIFI_SSID","WIFI_PASSWORD","MQTT_BROKER","MQTT_PORT",
        "MQTT_USER","MQTT_PASSWORD","MACHINE_ID","OTA_PASSWORD"]

env_file = find_env()
if not env_file:
    print("-DENV_NOT_FOUND"); sys.exit(0)

env = {}
for line in env_file.read_text().splitlines():
    line = line.strip()
    if not line or line.startswith("#") or "=" not in line: continue
    k, _, v = line.partition("=")
    env[k.strip()] = v.strip().strip('"').strip("'")

for var in VARS:
    val = env.get(var, "")
    if not val: continue
    if var == "MQTT_PORT": print(f"-D{var}={val}")
    else: print(f'-D{var}=\\"{val.replace(chr(34), chr(92)+chr(34))}\\"')
PYEOF
  log "inject_env.py creado"
fi

# config.h
if [ ! -f "firmware/esp32-s3/include/config.h" ]; then
cat > firmware/esp32-s3/include/config.h << 'EOF'
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
EOF
  log "config.h creado"
fi

# Bitácora inicial
if [ ! -f "bitacora/README.md" ]; then
cat > bitacora/README.md << 'EOF'
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
EOF
  log "bitacora/README.md creado"
fi

# GitHub Actions
if [ ! -f ".github/workflows/auto_push.yml" ]; then
cat > .github/workflows/auto_push.yml << 'EOF'
name: Validate & Sync
on:
  push:
    branches: [main]
  schedule:
    - cron: '0 12 * * *'
jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Check structure
        run: |
          for f in README.md CLAUDE.md scripts/setup_rpi5.sh \
                   firmware/esp32-s3/platformio.ini \
                   firmware/esp32-s3/src/main.cpp \
                   flows/n8n/mqtt_to_influxdb.json; do
            [ -f "$f" ] && echo "✓ $f" || (echo "✗ FALTA: $f" && exit 1)
          done
EOF
  log ".github/workflows/auto_push.yml creado"
fi

# ─── Resultado final ──────────────────────────────────────────────────────
echo ""
echo "━━━ Estructura final ━━━"
find . -not -path './.git/*' -not -path './.pio/*' \
       -not -path './node_modules/*' \
       | sort | grep -v "^.$"

echo ""
log "¡Listo! Ahora haz:"
echo ""
echo "  git add -A"
echo "  git commit -m 'fix: estructura correcta del repositorio'"
echo "  git push"
echo ""
