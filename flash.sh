#!/usr/bin/env bash
set -euo pipefail

# Normie Display — one-command build & flash for ESP32 e-paper
# Usage: curl -sL https://raw.githubusercontent.com/serc1n/epaper-normie-display/main/flash.sh | bash

REPO="https://github.com/serc1n/epaper-normie-display.git"
FQBN="esp32:esp32:esp32"
BAUD=115200
DIR="$HOME/epaper-normie-display"

info()  { printf "\033[1;37m>> %s\033[0m\n" "$1"; }
ok()    { printf "\033[1;32m   %s\033[0m\n" "$1"; }
err()   { printf "\033[1;31m!! %s\033[0m\n" "$1"; }
step=0
step()  { step=$((step+1)); printf "\n\033[1;37m[%d] %s\033[0m\n" "$step" "$1"; }

# ── Detect OS ──
OS="$(uname -s)"
ARCH="$(uname -m)"
if [[ "$OS" != "Darwin" && "$OS" != "Linux" ]]; then
  err "Unsupported OS: $OS (macOS and Linux only)"
  exit 1
fi

# ── 1. arduino-cli ──
step "Checking arduino-cli"
if command -v arduino-cli &>/dev/null; then
  ok "Found: $(arduino-cli version | head -1)"
elif [[ "$OS" == "Darwin" ]] && command -v brew &>/dev/null; then
  info "Installing via Homebrew..."
  brew install arduino-cli
elif [[ "$OS" == "Linux" ]]; then
  info "Installing via official script..."
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
  export PATH="$PWD/bin:$PATH"
else
  err "Please install arduino-cli: https://arduino.github.io/arduino-cli/installation/"
  exit 1
fi

# ── 2. ESP32 board package ──
step "Installing ESP32 board package"
arduino-cli config init --overwrite 2>/dev/null || true
arduino-cli config add board_manager.additional_urls \
  "https://espressif.github.io/arduino-esp32/package_esp32_index.json" 2>/dev/null || true
arduino-cli core update-index
if ! arduino-cli core list | grep -q "esp32:esp32"; then
  arduino-cli core install esp32:esp32
  ok "ESP32 core installed"
else
  ok "ESP32 core already installed"
fi

# ── 3. GxEPD2 library ──
step "Installing GxEPD2 library"
if ! arduino-cli lib list | grep -q "GxEPD2"; then
  arduino-cli lib install GxEPD2
  ok "GxEPD2 installed"
else
  ok "GxEPD2 already installed"
fi

# ── 4. Clone repo ──
step "Getting source code"
if [[ -d "$DIR/.git" ]]; then
  info "Updating existing clone..."
  cd "$DIR" && git pull
else
  git clone "$REPO" "$DIR"
  cd "$DIR"
fi
ok "Source ready at $DIR"

# ── 5. Compile ──
step "Compiling firmware"
arduino-cli compile --fqbn "$FQBN" \
  --build-property "build.partitions=partitions" \
  --build-property "upload.maximum_size=1572864" \
  --build-path "$DIR/build" "$DIR"
ok "Compiled successfully"

# ── 6. Detect port ──
step "Detecting ESP32"
PORT=""
for p in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/ttyUSB* /dev/ttyACM*; do
  if [[ -e "$p" ]]; then
    PORT="$p"
    break
  fi
done
if [[ -z "$PORT" ]]; then
  err "No ESP32 found. Connect the board via USB and try again."
  exit 1
fi
ok "Found device at $PORT"

# ── 7. Find esptool ──
step "Locating esptool"
ESPTOOL=""
if command -v esptool &>/dev/null; then
  ESPTOOL="esptool"
elif command -v esptool.py &>/dev/null; then
  ESPTOOL="esptool.py"
else
  # Look in Arduino packages
  for f in "$HOME"/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool \
           "$HOME"/.arduino15/packages/esp32/tools/esptool_py/*/esptool; do
    if [[ -x "$f" ]]; then
      ESPTOOL="$f"
      break
    fi
  done
fi
if [[ -z "$ESPTOOL" ]]; then
  err "esptool not found. Install with: pip install esptool"
  exit 1
fi
ok "Using $ESPTOOL"

# ── 8. Flash ──
step "Flashing firmware to ESP32"
info "Port: $PORT | Baud: $BAUD"
info "Hold BOOT button on ESP32 if flash fails on first attempt"
echo ""

"$ESPTOOL" --chip esp32 --port "$PORT" --baud "$BAUD" \
  --before default-reset --after hard-reset \
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 4MB \
  0x1000 "$DIR/build/epaper_receiver.ino.bootloader.bin" \
  0x8000 "$DIR/build/epaper_receiver.ino.partitions.bin" \
  0x10000 "$DIR/build/epaper_receiver.ino.bin"

echo ""
ok "Flash complete!"

# ── Done ──
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
printf "\033[1;37m  Normie Display is ready!\033[0m\n"
echo ""
echo "  1. Connect to Wi-Fi: EPaper"
echo "  2. Open http://192.168.4.1"
echo "  3. Enter a Normie ID or upload an image"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
