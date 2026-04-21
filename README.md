# Normie Display

ESP32 firmware that drives a 1.54" monochrome e-paper display (200x200) with a built-in Wi-Fi web interface. Display any of the 10,000 on-chain [Normies](https://normies.art) NFTs by ID, or upload any photo with real-time dithering preview.

## Hardware

- **MCU:** ESP32-D0WDQ6 (Waveshare ESP32 e-Paper Driver Board)
- **Display:** 1.54" 200x200 monochrome e-paper (GxEPD2_154_D67)
- **Flash:** 4MB SPI flash with custom partition table

### Pin Mapping

| Signal | GPIO |
|--------|------|
| EPD_BUSY | 25 |
| EPD_RST | 26 |
| EPD_DC | 27 |
| EPD_CS | 15 |
| EPD_CLK | 13 |
| EPD_MOSI | 14 |

## Features

### Normie Tab
Enter a Normie ID (0–9999) to instantly display its 40x40 on-chain pixel art, scaled to 200x200 via nearest-neighbor.

All 10,000 Normie bitmaps are stored in a dedicated flash partition (`normies`, 2.5MB) and include the latest composited Canvas customizations from the Ethereum smart contracts.

### Upload Tab
Upload any image from your phone or computer. The browser handles all image processing client-side before sending a 5000-byte 1-bit packed bitmap to the ESP32.

**Dithering modes:**
- **Threshold** — hard black/white cutoff at the slider value
- **Floyd-Steinberg** — error diffusion dithering that preserves tonal gradients as dot patterns, best for photographs
- **Ordered (Bayer)** — 4x4 Bayer matrix dithering that produces a structured halftone pattern

The threshold slider (1–255) adjusts brightness across all modes. Preview updates in real-time on a 200x200 canvas.

### Serial Protocol
The device also accepts commands over UART (115200 baud) for integration with other systems:

| Command | Byte(s) | Description |
|---------|---------|-------------|
| Clear | `0xDD` | Clears the display to white |
| Test | `0xFF` | Shows a checkerboard test pattern |
| Normie | `0xAB` + 2-byte big-endian ID | Displays a normie by ID |
| Image | `0xAA` (sync) → 5000 bytes → checksum → confirm | Streams a raw 200x200 1-bit image |

## Web Interface

On boot, the ESP32 creates an open Wi-Fi access point named **EPaper**. Connect and navigate to `http://192.168.4.1` to access the UI.

The entire web interface is a single HTML document embedded in firmware (`webpage.h`), with inline CSS and vanilla JavaScript. No external dependencies, no build step.

## Flash Layout

| Partition | Offset | Size | Contents |
|-----------|--------|------|----------|
| nvs | 0x9000 | 20KB | Non-volatile storage |
| otadata | 0xE000 | 8KB | OTA metadata |
| app0 | 0x10000 | 1.5MB | Application firmware |
| normies | 0x190000 | 2.5MB | 10,000 Normie bitmaps (200 bytes each) |

## Quick Start

Connect your ESP32 e-paper board via USB and run:

```bash
npx normie-display
```

That's it. This single command installs all dependencies, compiles the firmware, detects your board, and flashes it. Works on **macOS**, **Linux**, and **Windows**.

> Requires [Node.js](https://nodejs.org) 16+. If you don't have Node, use the shell scripts below instead.

<details>
<summary><b>Alternative: shell scripts (no Node required)</b></summary>

**macOS / Linux:**
```bash
curl -sL https://raw.githubusercontent.com/serc1n/epaper-normie-display/main/flash.sh | bash
```

**Windows (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/serc1n/epaper-normie-display/main/flash.ps1 | iex
```

</details>

These will install all dependencies (arduino-cli, ESP32 board package, GxEPD2), compile the firmware, detect your board, and flash it automatically.

Once flashed:
1. Connect to Wi-Fi network **EPaper**
2. Open **http://192.168.4.1**
3. Enter a Normie ID or upload an image

## Manual Build & Flash

### Requirements
- [arduino-cli](https://arduino.github.io/arduino-cli/installation/) or Arduino IDE
- ESP32 Arduino core (v3.3.7+)
- [GxEPD2](https://github.com/ZinggJM/GxEPD2) library
- [esptool](https://github.com/espressif/esptool)

### Clone & compile
```bash
git clone https://github.com/serc1n/epaper-normie-display.git
cd epaper-normie-display

arduino-cli compile --fqbn esp32:esp32:esp32 \
  --build-property "build.partitions=partitions" \
  --build-property "upload.maximum_size=1572864" .
```

### Flash firmware
```bash
esptool --chip esp32 --port /dev/cu.usbmodem* --baud 115200 \
  --before default-reset --after hard-reset \
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 4MB \
  0x1000 build/epaper_receiver.ino.bootloader.bin \
  0x8000 build/epaper_receiver.ino.partitions.bin \
  0x10000 build/epaper_receiver.ino.bin
```

### Flash normies data (optional)
The normie bitmaps are flashed separately to the `normies` partition. Without this step, the Upload tab still works but the Normie ID tab will not display anything.

```bash
esptool --chip esp32 --port /dev/cu.usbmodem* --baud 115200 \
  write-flash 0x190000 normies.bin
```

Each normie is fetched from the on-chain API as a 1600-character binary string (40x40 pixels), then packed into 200 bytes MSB-first. 10,000 normies = 2,000,000 bytes total.

```bash
curl https://api.normies.art/normie/{id}/pixels
```

## Project Structure

```
epaper-normie-display/
├── epaper_receiver.ino   # Main firmware (Wi-Fi AP, HTTP server, display driver, serial protocol)
├── webpage.h             # Embedded HTML/CSS/JS web interface
├── partitions.csv        # Custom flash partition table
├── flash.sh              # One-command install, build & flash (macOS/Linux)
├── flash.ps1             # One-command install, build & flash (Windows)
└── build/                # Compiled binaries (gitignored)
```

## HTTP API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Serves the web UI |
| GET | `/normie?id=N` | Displays normie N on the e-paper |
| POST | `/upload` | Accepts a 5000-byte raw 1-bit bitmap (multipart form) |
| POST | `/clear` | Clears the display |

All endpoints return JSON responses: `{"ok": true/false, "message": "...", "error": "..."}`.

## License

MIT License. See [LICENSE](LICENSE) for details.

Built by Normies, for Normies.
