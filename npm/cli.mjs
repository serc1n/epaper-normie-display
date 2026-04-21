#!/usr/bin/env node

import { execSync, spawnSync } from "child_process";
import { existsSync, readdirSync } from "fs";
import { homedir, platform, arch } from "os";
import { join } from "path";

const IS_WIN = platform() === "win32";
const IS_MAC = platform() === "darwin";
const IS_LINUX = platform() === "linux";

const REPO = "https://github.com/serc1n/epaper-normie-display.git";
const FQBN = "esp32:esp32:esp32";
const BAUD = 115200;
const DIR = join(homedir(), "epaper-normie-display");

const WHITE = "\x1b[1;37m";
const GREEN = "\x1b[1;32m";
const RED = "\x1b[1;31m";
const DIM = "\x1b[2m";
const RESET = "\x1b[0m";

let stepN = 0;
const info = (m) => console.log(`${WHITE}>> ${m}${RESET}`);
const ok = (m) => console.log(`${GREEN}   ${m}${RESET}`);
const err = (m) => console.log(`${RED}!! ${m}${RESET}`);
const step = (m) => {
  stepN++;
  console.log(`\n${WHITE}[${stepN}] ${m}${RESET}`);
};

function run(cmd, opts = {}) {
  try {
    return execSync(cmd, {
      encoding: "utf-8",
      stdio: opts.silent ? "pipe" : "inherit",
      shell: true,
      ...opts,
    });
  } catch {
    return null;
  }
}

function has(cmd) {
  const flag = IS_WIN ? "where" : "which";
  return run(`${flag} ${cmd}`, { silent: true }) !== null;
}

function findSerial() {
  if (IS_WIN) {
    const out = run("chcp 65001 >nul && mode", { silent: true }) || "";
    const matches = out.match(/COM\d+/g);
    return matches ? matches[0] : null;
  }
  const patterns = [
    "/dev/cu.usbmodem*",
    "/dev/cu.usbserial*",
    "/dev/cu.wchusbserial*",
    "/dev/ttyUSB*",
    "/dev/ttyACM*",
  ];
  for (const pat of patterns) {
    const out = run(`ls ${pat} 2>/dev/null`, { silent: true });
    if (out && out.trim()) return out.trim().split("\n")[0];
  }
  return null;
}

function findEsptool() {
  if (has("esptool")) return "esptool";
  if (has("esptool.py")) return "esptool.py";

  const bases = IS_WIN
    ? [
        join(process.env.LOCALAPPDATA || "", "Arduino15", "packages", "esp32", "tools", "esptool_py"),
        join(process.env.APPDATA || "", "Arduino15", "packages", "esp32", "tools", "esptool_py"),
      ]
    : [
        join(homedir(), "Library", "Arduino15", "packages", "esp32", "tools", "esptool_py"),
        join(homedir(), ".arduino15", "packages", "esp32", "tools", "esptool_py"),
      ];

  for (const base of bases) {
    if (!existsSync(base)) continue;
    for (const ver of readdirSync(base)) {
      const names = IS_WIN ? ["esptool.exe", "esptool"] : ["esptool"];
      for (const name of names) {
        const p = join(base, ver, name);
        if (existsSync(p)) return p;
      }
    }
  }
  return null;
}

// ── Banner ──
console.log(`
${WHITE}┌─────────────────────────────────────────┐
│         ${GREEN}NORMIE DISPLAY${WHITE}                   │
│    ESP32 E-Paper Firmware Flasher       │
└─────────────────────────────────────────┘${RESET}
`);

if (!IS_WIN && !IS_MAC && !IS_LINUX) {
  err(`Unsupported platform: ${platform()}`);
  process.exit(1);
}

// ── 1. arduino-cli ──
step("Checking arduino-cli");
if (has("arduino-cli")) {
  const ver = run("arduino-cli version", { silent: true })?.trim();
  ok(`Found: ${ver}`);
} else if (IS_MAC && has("brew")) {
  info("Installing via Homebrew...");
  run("brew install arduino-cli");
} else if (IS_WIN && has("winget")) {
  info("Installing via winget...");
  run("winget install --id Arduino.ArduinoCLI -e --accept-source-agreements --accept-package-agreements");
} else if (IS_WIN && has("choco")) {
  info("Installing via Chocolatey...");
  run("choco install arduino-cli -y");
} else if (IS_LINUX) {
  info("Installing via official script...");
  run("curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh");
  process.env.PATH = join(process.cwd(), "bin") + ":" + process.env.PATH;
} else {
  err("Please install arduino-cli: https://arduino.github.io/arduino-cli/installation/");
  process.exit(1);
}

if (!has("arduino-cli")) {
  err("arduino-cli installed but not in PATH. Restart your terminal and try again.");
  process.exit(1);
}

// ── 2. ESP32 board package ──
step("Installing ESP32 board package");
run("arduino-cli config init --overwrite", { silent: true });
run('arduino-cli config add board_manager.additional_urls "https://espressif.github.io/arduino-esp32/package_esp32_index.json"', { silent: true });
run("arduino-cli core update-index");
const cores = run("arduino-cli core list", { silent: true }) || "";
if (!cores.includes("esp32:esp32")) {
  run("arduino-cli core install esp32:esp32");
  ok("ESP32 core installed");
} else {
  ok("ESP32 core already installed");
}

// ── 3. GxEPD2 library ──
step("Installing GxEPD2 library");
const libs = run("arduino-cli lib list", { silent: true }) || "";
if (!libs.includes("GxEPD2")) {
  run("arduino-cli lib install GxEPD2");
  ok("GxEPD2 installed");
} else {
  ok("GxEPD2 already installed");
}

// ── 4. Clone repo ──
step("Getting source code");
if (existsSync(join(DIR, ".git"))) {
  info("Updating existing clone...");
  run("git pull", { cwd: DIR });
} else {
  run(`git clone ${REPO} "${DIR}"`);
}
ok(`Source ready at ${DIR}`);

// ── 5. Compile ──
step("Compiling firmware");
const buildPath = join(DIR, "build");
const compileCmd = [
  "arduino-cli compile",
  `--fqbn ${FQBN}`,
  `--build-property "build.partitions=partitions"`,
  `--build-property "upload.maximum_size=1572864"`,
  `--build-path "${buildPath}"`,
  `"${DIR}"`,
].join(" ");
run(compileCmd);
ok("Compiled successfully");

// ── 6. Detect port ──
step("Detecting ESP32");
const port = findSerial();
if (!port) {
  err("No ESP32 found. Connect the board via USB and try again.");
  process.exit(1);
}
ok(`Found device at ${port}`);

// ── 7. Find esptool ──
step("Locating esptool");
let esptool = findEsptool();
if (!esptool) {
  info("Installing esptool via pip...");
  run("pip install esptool", { silent: true }) || run("pip3 install esptool", { silent: true });
  esptool = findEsptool();
}
if (!esptool) {
  err("esptool not found. Install with: pip install esptool");
  process.exit(1);
}
ok(`Using ${esptool}`);

// ── 8. Flash ──
step("Flashing firmware to ESP32");
info(`Port: ${port} | Baud: ${BAUD}`);
info("Hold BOOT button on ESP32 if flash fails on first attempt");
console.log();

const sep = IS_WIN ? "\\" : "/";
const flashCmd = [
  `"${esptool}" --chip esp32 --port ${port} --baud ${BAUD}`,
  "--before default-reset --after hard-reset",
  "write-flash -z --flash-mode dio --flash-freq 80m --flash-size 4MB",
  `0x1000 "${buildPath}${sep}epaper_receiver.ino.bootloader.bin"`,
  `0x8000 "${buildPath}${sep}epaper_receiver.ino.partitions.bin"`,
  `0x10000 "${buildPath}${sep}epaper_receiver.ino.bin"`,
].join(" ");

const result = spawnSync(flashCmd, { shell: true, stdio: "inherit" });
if (result.status !== 0) {
  console.log();
  err("Flash failed. Try holding the BOOT button and running again.");
  process.exit(1);
}

// ── Done ──
console.log(`
${WHITE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}
${WHITE}  Normie Display is ready!${RESET}

  1. Connect to Wi-Fi: ${GREEN}EPaper${RESET}
  2. Open ${GREEN}http://192.168.4.1${RESET}
  3. Enter a Normie ID or upload an image

${WHITE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}
`);
