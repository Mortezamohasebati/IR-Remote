// =============================================================
//  IR Remote v3.0  —  WiFi Station + MQTT
//  کد ESP32 برای کنترل آنلاین از هر جای دنیا
// =============================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ir_codes.h"

// ════════════════════════════════════════════════════════════
//  ⚙️  تنظیمات — اینجا رو ویرایش کن
// ════════════════════════════════════════════════════════════
const char* WIFI_SSID  = "WIFI_SSID";      // نام وای‌فای خونه
const char* WIFI_PASS  = "WIFI_PASS";  // پسورد وای‌فای خونه

const char* MQTT_HOST  = "IP_Host";       // IP سرور
const int   MQTT_PORT  = 1883;
const char* MQTT_CLIENT_ID = "esp32-irremote";
const char* TOPIC_CMD  = "irremote/cmd";        // دستورات از سایت
const char* TOPIC_RESP = "irremote/resp";       // جواب به سایت

// ════════════════════════════════════════════════════════════
//  سخت‌افزار
// ════════════════════════════════════════════════════════════
const uint16_t IR_SEND_PIN = 4;
const uint16_t IR_RECV_PIN = 14;
const uint16_t CAPTURE_BUF = 1024;

IRsend   irsend(IR_SEND_PIN);
IRrecv   irrecv(IR_RECV_PIN, CAPTURE_BUF, 50, true);
Preferences prefs;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);

// ── وضعیت یادگیری ─────────────────────────────────────────
bool     learnMode  = false;
String   learnDevice = "";
String   learnBtn   = "";
uint32_t learnStart = 0;
bool     learnDone  = false;
const uint32_t LEARN_TIMEOUT = 15000;


// ════════════════════════════════════════════════════════════
//  EEPROM helpers — دست‌نخورده از v2.0
// ════════════════════════════════════════════════════════════
String learnKey(String devId, String btnName) {
  return "L_" + devId + "_" + btnName;
}

void saveLearnedCode(String devId, String btnName, uint64_t code, uint8_t bits, uint8_t proto) {
  prefs.begin("irlearn", false);
  String k = learnKey(devId, btnName);
  String val = String((uint32_t)(code >> 32), HEX) + ":" +
               String((uint32_t)(code & 0xFFFFFFFF), HEX) + ":" +
               String(bits) + ":" + String(proto);
  prefs.putString(k.c_str(), val);
  prefs.end();
}

bool loadLearnedCode(String devId, String btnName, uint64_t &code, uint8_t &bits, uint8_t &proto) {
  prefs.begin("irlearn", true);
  String k = learnKey(devId, btnName);
  if (!prefs.isKey(k.c_str())) { prefs.end(); return false; }
  String val = prefs.getString(k.c_str(), "");
  prefs.end();
  if (val.isEmpty()) return false;
  int c1 = val.indexOf(':'), c2 = val.indexOf(':', c1+1), c3 = val.indexOf(':', c2+1);
  uint32_t hi = strtoul(val.substring(0,c1).c_str(), nullptr, 16);
  uint32_t lo = strtoul(val.substring(c1+1,c2).c_str(), nullptr, 16);
  code  = ((uint64_t)hi << 32) | lo;
  bits  = val.substring(c2+1, c3).toInt();
  proto = val.substring(c3+1).toInt();
  return true;
}

void deleteLearnedCode(String devId, String btnName) {
  prefs.begin("irlearn", false);
  prefs.remove(learnKey(devId, btnName).c_str());
  prefs.end();
}


// ════════════════════════════════════════════════════════════
//  ساخت JSON دستگاه‌ها — دست‌نخورده از v2.0
// ════════════════════════════════════════════════════════════
String buildDevicesJSON() {
  String json = "[";
  for (int i = 0; i < DEVICE_COUNT; i++) {
    DeviceIR& d = devices[i];
    if (i > 0) json += ",";
    json += "{\"id\":\"" + String(d.id) + "\",";
    json += "\"name\":\"" + String(d.name) + "\",";
    json += "\"cat\":\"" + String(d.category) + "\",";
    json += "\"catLabel\":\"" + String(d.catLabel) + "\",";
    json += "\"brand\":\"" + String(d.brand) + "\",";
    json += "\"btns\":[";
    for (int j = 0; j < d.btnCount; j++) {
      uint64_t lCode; uint8_t lBits, lProto;
      bool hasLearned = loadLearnedCode(d.id, d.buttons[j].name, lCode, lBits, lProto);
      if (j > 0) json += ",";
      json += "{\"name\":\"" + String(d.buttons[j].name) + "\",\"learned\":" + (hasLearned ? "true" : "false") + "}";
    }
    json += "]}";
  }
  json += "]";
  return json;
}
// ════════════════════════════════════════════════════════════
//  ارسال IR — دست‌نخورده از v2.0
// ════════════════════════════════════════════════════════════
void sendCode(IRProtocol proto, uint64_t code, uint8_t bits, uint8_t repeat) {
  for (int r = 0; r < repeat; r++) {
    switch (proto) {
      case PROTO_NEC:        irsend.sendNEC(code, bits);        break;
      case PROTO_SAMSUNG:    irsend.sendSAMSUNG(code, bits);    break;
      case PROTO_SONY:       irsend.sendSony(code, bits);       break;
      case PROTO_LG:         irsend.sendLG(code, bits);         break;
      case PROTO_RC5:        irsend.sendRC5(code, bits);        break;
      case PROTO_SHARP:      irsend.sendSharpRaw(code, bits);   break;
      case PROTO_PANASONIC:  irsend.sendPanasonic(bits, code);  break;
      default:               irsend.sendNEC(code, bits);        break;
    }
    if (repeat > 1) delay(45);
  }
}


// ════════════════════════════════════════════════════════════
//  MQTT — پاسخ‌دهی
// ════════════════════════════════════════════════════════════
void publishResponse(const String& id, const String& payloadJson) {
  String msg = "{\"id\":\"" + id + "\",\"payload\":" + payloadJson + "}";
  mqttClient.publish(TOPIC_RESP, msg.c_str());
  Serial.println("[MQTT] >> " + msg);
}


// ════════════════════════════════════════════════════════════
//  MQTT — پردازش دستور دریافتی
// ════════════════════════════════════════════════════════════
void handleCommand(const String& id, const String& action, JsonObject params) {

  // ── دریافت لیست دستگاه‌ها ─────────────────────────────
  if (action == "devices") {
    publishResponse(id, buildDevicesJSON());
    return;
  }

  // ── ارسال IR ──────────────────────────────────────────
  if (action == "ir") {
    String devId   = params["device"] | "";
    String btnName = params["btn"]    | "";

    uint64_t lCode; uint8_t lBits, lProto;
    if (loadLearnedCode(devId, btnName, lCode, lBits, lProto)) {
      sendCode((IRProtocol)lProto, lCode, lBits, 1);
      Serial.printf("[IR-L] %s > %s\n", devId.c_str(), btnName.c_str());
      publishResponse(id, "{\"ok\":true,\"src\":\"learned\"}");
      return;
    }

    for (int i = 0; i < DEVICE_COUNT; i++) {
      if (devId != devices[i].id) continue;
      for (int j = 0; j < devices[i].btnCount; j++) {
        if (btnName != devices[i].buttons[j].name) continue;
        IRButton& b = devices[i].buttons[j];
        sendCode(devices[i].protocol, b.code, b.bits, b.repeat);
        Serial.printf("[IR-S] %s > %s\n", devId.c_str(), btnName.c_str());
        publishResponse(id, "{\"ok\":true,\"src\":\"library\"}");
        return;
      }
    }
    publishResponse(id, "{\"error\":\"not found\"}");
    return;
  }

  // ── شروع یادگیری ─────────────────────────────────────
  if (action == "learn/start") {
    learnDevice = params["device"] | "";
    learnBtn    = params["btn"]    | "";
    learnMode   = true;
    learnDone   = false;
    learnStart  = millis();
    irrecv.enableIRIn();
    Serial.printf("[LEARN] Start: %s / %s\n", learnDevice.c_str(), learnBtn.c_str());
    publishResponse(id, "{\"ok\":true,\"timeout\":15}");
    return;
  }

  // ── وضعیت یادگیری ────────────────────────────────────
  if (action == "learn/status") {
    if (learnDone) {
      learnDone = false;
      publishResponse(id, "{\"status\":\"done\"}");
      return;
    }
    if (!learnMode) {
      publishResponse(id, "{\"status\":\"idle\"}");
      return;
    }
    if (millis() - learnStart > LEARN_TIMEOUT) {
      learnMode = false;
      irrecv.disableIRIn();
      publishResponse(id, "{\"status\":\"timeout\"}");
      return;
    }
    uint32_t rem = (LEARN_TIMEOUT - (millis() - learnStart)) / 1000;
    publishResponse(id, "{\"status\":\"waiting\",\"remaining\":" + String(rem) + "}");
    return;
  }

  // ── لغو یادگیری ──────────────────────────────────────
  if (action == "learn/cancel") {
    learnMode = false;
    irrecv.disableIRIn();
    publishResponse(id, "{\"ok\":true}");
    return;
  }

  // ── حذف کد یاد گرفته‌شده ────────────────────────────
  if (action == "learn/delete") {
    String devId   = params["device"] | "";
    String btnName = params["btn"]    | "";
    deleteLearnedCode(devId, btnName);
    publishResponse(id, "{\"ok\":true}");
    return;
  }

  publishResponse(id, "{\"error\":\"unknown action\"}");
}


// ════════════════════════════════════════════════════════════
//  MQTT — callback دریافت پیام
// ════════════════════════════════════════════════════════════
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("[MQTT] << " + msg);

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.println("[MQTT] JSON parse error");
    return;
  }

  String id     = doc["id"]     | "0";
  String action = doc["action"] | "";
  JsonObject params = doc["params"].as<JsonObject>();

  handleCommand(id, action, params);
}


// ════════════════════════════════════════════════════════════
//  اتصال به WiFi
// ════════════════════════════════════════════════════════════
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}


// ════════════════════════════════════════════════════════════
//  اتصال / ری‌کانکت MQTT
// ════════════════════════════════════════════════════════════
void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println(" OK");
      mqttClient.subscribe(TOPIC_CMD);
      Serial.printf("[MQTT] Subscribed to %s\n", TOPIC_CMD);
    } else {
      Serial.printf(" Failed (rc=%d), retry in 5s\n", mqttClient.state());
      delay(5000);
    }
  }
}


// ════════════════════════════════════════════════════════════
//  حلقه یادگیری IR
// ════════════════════════════════════════════════════════════
void loopLearn() {
  if (!learnMode) return;
  if (millis() - learnStart > LEARN_TIMEOUT) {
    learnMode = false;
    irrecv.disableIRIn();
    Serial.println("[LEARN] Timeout");
    return;
  }
  decode_results results;
  if (irrecv.decode(&results)) {
    if (results.decode_type != decode_type_t::UNKNOWN &&
        results.decode_type != decode_type_t::GLOBALCACHE &&
        results.value != 0xFFFFFFFF) {

      uint64_t code  = results.value;
      uint8_t  bits  = results.bits;
      uint8_t  proto = (uint8_t)results.decode_type;

      saveLearnedCode(learnDevice, learnBtn, code, bits, proto);
      Serial.printf("[LEARN] OK: %s/%s = 0x%llX (proto=%d)\n",
        learnDevice.c_str(), learnBtn.c_str(), code, proto);

      learnMode = false;
      learnDone = true;
      irrecv.disableIRIn();
    }
    irrecv.resume();
  }
}


// ════════════════════════════════════════════════════════════
//  Setup
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== IR Remote v3.0 (MQTT) ===");

  irsend.begin();

  connectWiFi();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(16384); // برای JSON بزرگ

  mqttReconnect();

  Serial.println("[READY] Waiting for commands on " + String(TOPIC_CMD));
}


// ════════════════════════════════════════════════════════════
//  Loop
// ════════════════════════════════════════════════════════════
void loop() {
  // بررسی اتصال WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Reconnecting...");
    connectWiFi();
  }

  // بررسی اتصال MQTT
  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttClient.loop();
  loopLearn();
}
