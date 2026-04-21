#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <esp_partition.h>
#include "webpage.h"

// Waveshare ESP32 e-Paper Driver Board pin mapping
#define EPD_BUSY 25
#define EPD_RST  26
#define EPD_DC   27
#define EPD_CS   15
#define EPD_CLK  13
#define EPD_MOSI 14

#define WIDTH  200
#define HEIGHT 200
#define IMAGE_BYTES (WIDTH * HEIGHT / 8)

#define NORMIE_SIZE   200   // 40x40 / 8
#define NORMIE_W      40
#define NORMIE_H      40
#define NORMIE_SCALE  5     // 40 * 5 = 200
#define NORMIE_COUNT  10000

const char* AP_SSID = "EPaper";
const char* AP_PASS = "";

GxEPD2_BW<GxEPD2_154_D67, 200>
    display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

WebServer server(80);

const esp_partition_t* normies_part = NULL;

// Serial protocol bytes
const uint8_t SYNC_BYTE   = 0xAA;
const uint8_t ACK_BYTE    = 0xBB;
const uint8_t DONE_BYTE   = 0xCC;
const uint8_t ERR_BYTE    = 0xEE;
const uint8_t CLEAR_CMD   = 0xDD;
const uint8_t TEST_CMD    = 0xFF;
const uint8_t NORMIE_CMD  = 0xAB;

uint8_t imageBuffer[IMAGE_BYTES];

void clearDisplay() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());
}

void showImage(const uint8_t* buf) {
    display.writeImage(buf, 0, 0, WIDTH, HEIGHT, true, false, false);
    display.refresh(false); // full refresh every time — no ghosting
}

// Scale 40x40 normie bitmap (200 bytes) → 200x200 display bitmap (5000 bytes)
void scaleNormie(const uint8_t* src, uint8_t* dst) {
    memset(dst, 0, IMAGE_BYTES);
    for (int sy = 0; sy < NORMIE_H; sy++) {
        for (int sx = 0; sx < NORMIE_W; sx++) {
            int srcIdx = sy * NORMIE_W + sx;
            int srcBit = (src[srcIdx >> 3] >> (7 - (srcIdx & 7))) & 1;
            if (!srcBit) continue;

            for (int dy = 0; dy < NORMIE_SCALE; dy++) {
                for (int dx = 0; dx < NORMIE_SCALE; dx++) {
                    int px = sx * NORMIE_SCALE + dx;
                    int py = sy * NORMIE_SCALE + dy;
                    int dstIdx = py * WIDTH + px;
                    dst[dstIdx >> 3] |= 1 << (7 - (dstIdx & 7));
                }
            }
        }
    }
}

bool displayNormie(uint16_t id) {
    if (!normies_part || id >= NORMIE_COUNT) return false;

    uint8_t normie_data[NORMIE_SIZE];
    esp_err_t err = esp_partition_read(normies_part, (uint32_t)id * NORMIE_SIZE, normie_data, NORMIE_SIZE);
    if (err != ESP_OK) return false;

    // Check if data is all zeros (not downloaded yet)
    bool empty = true;
    for (int i = 0; i < NORMIE_SIZE; i++) {
        if (normie_data[i] != 0) { empty = false; break; }
    }
    if (empty) return false;

    scaleNormie(normie_data, imageBuffer);
    return true;
}

// ── WiFi handlers ──────────────────────────────────────────────

static size_t uploadReceived = 0;
static bool pendingShow = false;
static bool pendingClear = false;

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleUploadDone() {
    if (uploadReceived != IMAGE_BYTES) {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "{\"ok\":false,\"error\":\"Got %u bytes, need %d\"}", uploadReceived, IMAGE_BYTES);
        server.send(400, "application/json", msg);
        return;
    }
    pendingShow = true;
    server.send(200, "application/json", "{\"ok\":true,\"message\":\"Image displayed!\"}");
}

void handleUploadData() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        uploadReceived = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t space = IMAGE_BYTES - uploadReceived;
        size_t toWrite = upload.currentSize < space ? upload.currentSize : space;
        memcpy(imageBuffer + uploadReceived, upload.buf, toWrite);
        uploadReceived += toWrite;
    }
}

void handleNormie() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing id\"}");
        return;
    }

    int id = server.arg("id").toInt();
    if (id < 0 || id >= NORMIE_COUNT) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"ID must be 0-9999\"}");
        return;
    }

    if (!normies_part) {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"Normies partition not found\"}");
        return;
    }

    if (!displayNormie((uint16_t)id)) {
        server.send(404, "application/json", "{\"ok\":false,\"error\":\"Normie not found or empty\"}");
        return;
    }

    pendingShow = true;
    char msg[60];
    snprintf(msg, sizeof(msg), "{\"ok\":true,\"message\":\"Normie #%d displayed!\"}", id);
    server.send(200, "application/json", msg);
}

void handleClear() {
    pendingClear = true;
    server.send(200, "application/json", "{\"ok\":true,\"message\":\"Display cleared.\"}");
}

void handleNotFound() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
}

// ── Serial protocol ───────────────────────────────────────────

void handleSerial() {
    if (Serial.available() < 1) return;

    uint8_t cmd = Serial.read();

    if (cmd == CLEAR_CMD) {
        clearDisplay();
        Serial.write(DONE_BYTE);
        return;
    }

    if (cmd == NORMIE_CMD) {
        uint32_t lastActivity = millis();
        while (Serial.available() < 2) {
            if (millis() - lastActivity > 2000) { Serial.write(ERR_BYTE); return; }
        }
        uint8_t id_high = Serial.read();
        uint8_t id_low  = Serial.read();
        uint16_t id = ((uint16_t)id_high << 8) | id_low;
        if (displayNormie(id)) {
            showImage(imageBuffer);
            Serial.write(DONE_BYTE);
        } else {
            Serial.write(ERR_BYTE);
        }
        return;
    }

    if (cmd == TEST_CMD) {
        for (int i = 0; i < IMAGE_BYTES; i++)
            imageBuffer[i] = (i / 25 % 2 == 0) ? 0xFF : 0x00;
        showImage(imageBuffer);
        Serial.write(DONE_BYTE);
        return;
    }

    if (cmd != SYNC_BYTE) return;
    Serial.write(ACK_BYTE);

    uint32_t received = 0;
    uint32_t lastActivity = millis();

    while (received < IMAGE_BYTES) {
        if (Serial.available()) {
            imageBuffer[received++] = Serial.read();
            lastActivity = millis();
        }
        if (millis() - lastActivity > 5000) {
            Serial.write(ERR_BYTE);
            return;
        }
    }

    uint8_t checksum = 0;
    for (uint32_t i = 0; i < IMAGE_BYTES; i++)
        checksum ^= imageBuffer[i];
    Serial.write(checksum);

    lastActivity = millis();
    while (millis() - lastActivity < 5000) {
        if (Serial.available()) {
            uint8_t confirm = Serial.read();
            if (confirm == 0x01) {
                showImage(imageBuffer);
                Serial.write(DONE_BYTE);
            } else {
                Serial.write(ERR_BYTE);
            }
            return;
        }
    }
    Serial.write(ERR_BYTE);
}

// ── Setup & Loop ──────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    SPI.end();
    SPI.begin(EPD_CLK, -1, EPD_MOSI, EPD_CS);

    display.init(0);
    display.setRotation(0);
    clearDisplay();

    normies_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x80, "normies");
    if (normies_part) {
        Serial.printf("Normies partition: %dKB at 0x%X\n",
                       normies_part->size / 1024, normies_part->address);
    } else {
        Serial.println("WARNING: Normies partition not found");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("WiFi AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/upload", HTTP_POST, handleUploadDone, handleUploadData);
    server.on("/normie", HTTP_GET, handleNormie);
    server.on("/clear", HTTP_POST, handleClear);
    server.onNotFound(handleNotFound);
    server.begin();

    Serial.println("EPAPER_READY");
}

void loop() {
    server.handleClient();

    if (pendingShow) {
        pendingShow = false;
        delay(50);
        showImage(imageBuffer);
    }

    if (pendingClear) {
        pendingClear = false;
        delay(50);
        clearDisplay();
    }

    handleSerial();
}
