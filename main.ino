// =============================================================
//  IR Remote v2.0  —  ESP32 Access Point + Learn Mode
// =============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include "globals.h"
#include "ir_codes.h"
#include "wifi_manager.h"
#include "smarthome_client.h"
#include "vendor_api.h"
#include "scenes.h"
#include "web_ui.h"

// ════════════════════════════════════════════════════════════
//  Hardware config
// ════════════════════════════════════════════════════════════
const uint16_t IR_SEND_PIN = 4;
const uint16_t IR_RECV_PIN = 14;
const uint16_t CAPTURE_BUF = 1024;

// ════════════════════════════════════════════════════════════
//  Globals
// ════════════════════════════════════════════════════════════
IRsend   irsend(IR_SEND_PIN);
IRrecv   irrecv(IR_RECV_PIN, CAPTURE_BUF, 50, true);
WebServer server(80);
Preferences prefs;

bool     learnMode   = false;
String   learnDevice = "";
String   learnBtn    = "";
uint32_t learnStart  = 0;
const uint32_t LEARN_TIMEOUT = 15000;
bool     learnDone   = false;

// ── Scene matching ────────────────────────────────────────
String lastRxDevice = "";
String lastRxBtn    = "";

// ── Client instances ──────────────────────────────────────
SmartHomeClient shClient;
VendorAPIClient vendorAPI;

// ── CSRF token ────────────────────────────────────────────
char csrfToken[17];

// ════════════════════════════════════════════════════════════
//  IR code matching helpers
// ════════════════════════════════════════════════════════════
String findIRInLibrary(uint64_t code, decode_type_t proto, String& outBtn) {
  // Check learned codes first
  for (int i = 0; i < DEVICE_COUNT; i++) {
    for (int j = 0; j < devices[i].btnCount; j++) {
      uint64_t lCode; uint8_t lBits; decode_type_t lProto;
      if (loadLearnedCode(devices[i].id, devices[i].buttons[j].name, lCode, lBits, lProto)) {
        if (lCode == code && lProto == proto) {
          outBtn = devices[i].buttons[j].name;
          return devices[i].id;
        }
      }
    }
  }
  // Check static library
  for (int i = 0; i < DEVICE_COUNT; i++) {
    for (int j = 0; j < devices[i].btnCount; j++) {
      if (devices[i].buttons[j].code == code && devices[i].protocol == proto) {
        outBtn = devices[i].buttons[j].name;
        return devices[i].id;
      }
    }
  }
  return "";
}

// ════════════════════════════════════════════════════════════
//  loopReceive — IR receive (always active)
// ════════════════════════════════════════════════════════════
void loopReceive() {
  decode_results results;
  if (!irrecv.decode(&results)) return;

  if (results.decode_type != decode_type_t::UNKNOWN &&
      results.decode_type != decode_type_t::GLOBALCACHE &&
      results.value != 0xFFFFFFFF) {

    uint64_t    code  = results.value;
    uint8_t     bits  = results.bits;
    decode_type_t proto = results.decode_type;

    if (learnMode) {
      // Save learned code
      saveLearnedCode(learnDevice, learnBtn, code, bits, proto);
      Serial.printf("[LEARN] OK: %s/%s = 0x%llX (%d bits, proto=%d)\n",
        learnDevice.c_str(), learnBtn.c_str(), code, bits, proto);
      learnMode = false;
      learnDone = true;
    } else {
      // Check for scene triggers
      String btn;
      String dev = findIRInLibrary(code, proto, btn);
      if (dev.length() > 0) {
        lastRxDevice = dev;
        lastRxBtn    = btn;
        Serial.printf("[IR-RX] Matched: %s / %s = 0x%llX\n", dev.c_str(), btn.c_str(), code);
      }
    }
  }
  irrecv.resume();
}

void loopLearn() {
  // Legacy — handled by loopReceive now
}

// ════════════════════════════════════════════════════════════
//  Setup
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== IR Remote v2.0 ===");

  for (int i = 0; i < 16; i++) csrfToken[i] = "0123456789abcdef"[esp_random() % 16];
  csrfToken[16] = '\0';
  Serial.printf("CSRF token: %s\n", csrfToken);

  irsend.begin();
  irrecv.enableIRIn();
  Serial.printf("IR Send  → GPIO %d\n", IR_SEND_PIN);
  Serial.printf("IR Recv  → GPIO %d\n", IR_RECV_PIN);

  setupWiFi();

  // Routes
  server.on("/",              HTTP_GET,  handleRoot);
  server.on("/devices",       HTTP_GET,  handleDeviceList);
  server.on("/csrf",          HTTP_GET,  handleCSRFToken);
  server.on("/ir",            HTTP_GET,  handleIRSend);
  server.on("/learn/start",   HTTP_POST, handleLearnStart);
  server.on("/learn/status",  HTTP_GET,  handleLearnStatus);
  server.on("/learn/cancel",  HTTP_POST, handleLearnCancel);
  server.on("/learn/delete",  HTTP_POST, handleLearnDelete);
  server.on("/wifi/status",   HTTP_GET,  handleWiFiStatus);
  server.on("/wifi/config",   HTTP_GET,  handleWiFiConfig);
  server.on("/irdb/brands",   HTTP_GET,  handleIRDBBrands);
  server.on("/irdb/codes",    HTTP_GET,  handleIRDBCodes);
  server.on("/irdb",          HTTP_GET,  handleIRDBPage);
  server.on("/scenes",        HTTP_GET,  handleScenesList);
  server.on("/scenes",        HTTP_POST, handleScenesCreate);
  server.on("/scenes",        HTTP_DELETE, handleScenesDelete);
  server.on("/smarthome",       HTTP_GET,  handleSmartHomePage);
  server.on("/smarthome/config", HTTP_ANY, handleSmartHomeConfig);
  server.on("/smarthome/sensors", HTTP_GET,  handleSmartHomeSensors);
  server.on("/smarthome/device",  HTTP_POST, handleSmartHomeDevice);
  server.on("/smarthome/alarm",   HTTP_POST, handleSmartHomeAlarm);
  server.on("/smarthome/smart",   HTTP_POST, handleSmartHomeSmart);
  server.on("/vendor",          HTTP_GET,  handleVendorPage);
  server.on("/vendor/config",   HTTP_ANY,  handleVendorConfig);

  server.onNotFound([]{ server.send(404,"text/plain","Not Found"); });

  server.begin();
  Serial.println("Server started.");
  Serial.println("WiFi: IR-Remote  |  Pass: 12345678");
  Serial.println("http://192.168.4.1");
}

// ════════════════════════════════════════════════════════════
//  Main loop
// ════════════════════════════════════════════════════════════
void loop() {
  server.handleClient();
  loopWiFi();
  loopReceive();
  loopScenes();
}
