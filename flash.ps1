# Normie Display — one-command build & flash for ESP32 e-paper (Windows)
# Usage: irm https://raw.githubusercontent.com/serc1n/epaper-normie-display/main/flash.ps1 | iex

$ErrorActionPreference = "Stop"
$REPO = "https://github.com/serc1n/epaper-normie-display.git"
$FQBN = "esp32:esp32:esp32"
$BAUD = 115200
$DIR  = "$env:USERPROFILE\epaper-normie-display"
$step = 0

function Info($msg)  { Write-Host ">> $msg" -ForegroundColor White }
function Ok($msg)    { Write-Host "   $msg" -ForegroundColor Green }
function Err($msg)   { Write-Host "!! $msg" -ForegroundColor Red }
function Step($msg)  { $script:step++; Write-Host "`n[$script:step] $msg" -ForegroundColor White }

# ── 1. arduino-cli ──
Step "Checking arduino-cli"
$acli = Get-Command arduino-cli -ErrorAction SilentlyContinue
if ($acli) {
    Ok "Found: $(arduino-cli version | Select-Object -First 1)"
} elseif (Get-Command winget -ErrorAction SilentlyContinue) {
    Info "Installing via winget..."
    winget install --id Arduino.ArduinoCLI -e --accept-source-agreements --accept-package-agreements
    $env:PATH = "$env:LOCALAPPDATA\Programs\arduino-cli;$env:PATH"
    if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
        $env:PATH = "$env:LOCALAPPDATA\Microsoft\WinGet\Links;$env:PATH"
    }
} elseif (Get-Command choco -ErrorAction SilentlyContinue) {
    Info "Installing via Chocolatey..."
    choco install arduino-cli -y
} else {
    Err "Please install arduino-cli: https://arduino.github.io/arduino-cli/installation/"
    Err "Or install winget/chocolatey first."
    exit 1
}

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    Err "arduino-cli installed but not in PATH. Restart terminal and try again."
    exit 1
}

# ── 2. ESP32 board package ──
Step "Installing ESP32 board package"
arduino-cli config init --overwrite 2>$null
arduino-cli config add board_manager.additional_urls `
    "https://espressif.github.io/arduino-esp32/package_esp32_index.json" 2>$null
arduino-cli core update-index
$cores = arduino-cli core list
if ($cores -notmatch "esp32:esp32") {
    arduino-cli core install esp32:esp32
    Ok "ESP32 core installed"
} else {
    Ok "ESP32 core already installed"
}

# ── 3. GxEPD2 library ──
Step "Installing GxEPD2 library"
$libs = arduino-cli lib list
if ($libs -notmatch "GxEPD2") {
    arduino-cli lib install GxEPD2
    Ok "GxEPD2 installed"
} else {
    Ok "GxEPD2 already installed"
}

# ── 4. Clone repo ──
Step "Getting source code"
if (Test-Path "$DIR\.git") {
    Info "Updating existing clone..."
    Set-Location $DIR
    git pull
} else {
    git clone $REPO $DIR
    Set-Location $DIR
}
Ok "Source ready at $DIR"

# ── 5. Compile ──
Step "Compiling firmware"
arduino-cli compile --fqbn $FQBN `
    --build-property "build.partitions=partitions" `
    --build-property "upload.maximum_size=1572864" `
    --build-path "$DIR\build" $DIR
Ok "Compiled successfully"

# ── 6. Detect port ──
Step "Detecting ESP32"
$PORT = ""
$ports = [System.IO.Ports.SerialPort]::GetPortNames()
foreach ($p in $ports) {
    if ($p -match "COM\d+") {
        $PORT = $p
        break
    }
}
if (-not $PORT) {
    Err "No ESP32 found. Connect the board via USB and try again."
    exit 1
}
Ok "Found device at $PORT"

# ── 7. Find esptool ──
Step "Locating esptool"
$ESPTOOL = ""
if (Get-Command esptool -ErrorAction SilentlyContinue) {
    $ESPTOOL = "esptool"
} elseif (Get-Command esptool.py -ErrorAction SilentlyContinue) {
    $ESPTOOL = "esptool.py"
} else {
    $searchPaths = @(
        "$env:LOCALAPPDATA\Arduino15\packages\esp32\tools\esptool_py",
        "$env:APPDATA\Arduino15\packages\esp32\tools\esptool_py"
    )
    foreach ($base in $searchPaths) {
        if (Test-Path $base) {
            $found = Get-ChildItem -Path $base -Recurse -Filter "esptool.exe" | Select-Object -First 1
            if ($found) { $ESPTOOL = $found.FullName; break }
            $found = Get-ChildItem -Path $base -Recurse -Filter "esptool" | Select-Object -First 1
            if ($found) { $ESPTOOL = $found.FullName; break }
        }
    }
}
if (-not $ESPTOOL) {
    Info "Installing esptool via pip..."
    pip install esptool 2>$null
    if (Get-Command esptool -ErrorAction SilentlyContinue) {
        $ESPTOOL = "esptool"
    } else {
        Err "esptool not found. Install with: pip install esptool"
        exit 1
    }
}
Ok "Using $ESPTOOL"

# ── 8. Flash ──
Step "Flashing firmware to ESP32"
Info "Port: $PORT | Baud: $BAUD"
Info "Hold BOOT button on ESP32 if flash fails on first attempt"
Write-Host ""

& $ESPTOOL --chip esp32 --port $PORT --baud $BAUD `
    --before default-reset --after hard-reset `
    write-flash -z --flash-mode dio --flash-freq 80m --flash-size 4MB `
    0x1000 "$DIR\build\epaper_receiver.ino.bootloader.bin" `
    0x8000 "$DIR\build\epaper_receiver.ino.partitions.bin" `
    0x10000 "$DIR\build\epaper_receiver.ino.bin"

Write-Host ""
Ok "Flash complete!"

# ── Done ──
Write-Host ""
Write-Host ([char]0x2501) * 43
Write-Host "  Normie Display is ready!" -ForegroundColor White
Write-Host ""
Write-Host "  1. Connect to Wi-Fi: EPaper"
Write-Host "  2. Open http://192.168.4.1"
Write-Host "  3. Enter a Normie ID or upload an image"
Write-Host ""
Write-Host ([char]0x2501) * 43
