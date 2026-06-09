#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "ir_codes.h"
#include "wifi_manager.h"
#include "irdb_client.h"
#include "scenes.h"

IRDBClient irdbClient;

// ── Input validation ────────────────────────────────────────

bool validateArg(const String& s, size_t maxLen = 64) {
  if (s.length() > maxLen) return false;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!isAlphaNumeric(c) && c != '-' && c != '_' && c != ':' && c != '/') return false;
  }
  return true;
}

bool csrfCheck() {
  String h = server.header("X-CSRF-Token");
  if (h.length() == 16 && h == csrfToken) return true;
  if (server.hasArg("csrf") && server.arg("csrf") == csrfToken) return true;
  return false;
}

// ── Rate limiter: /ir endpoint ──────────────────────────────

const int IR_RATE_MAX = 10;
unsigned long irRateTimes[IR_RATE_MAX];
int irRateIdx = 0;

bool irRateCheck() {
  unsigned long now = millis();
  irRateTimes[irRateIdx] = now;
  irRateIdx = (irRateIdx + 1) % IR_RATE_MAX;
  int count = 0;
  for (int i = 0; i < IR_RATE_MAX; i++) {
    if (now - irRateTimes[i] <= 1000) count++;
  }
  return count <= IR_RATE_MAX;
}

// ── Learning helpers ────────────────────────────────────────

String learnKey(String devId, String btnName) {
  return "L_" + devId + "_" + btnName;
}

void saveLearnedCode(String devId, String btnName, uint64_t code, uint8_t bits, decode_type_t proto) {
  prefs.begin("irlearn", false);
  String k = learnKey(devId, btnName);
  String val = String((uint32_t)(code >> 32), HEX) + ":" +
               String((uint32_t)(code & 0xFFFFFFFF), HEX) + ":" +
               String(bits) + ":" + String(proto);
  prefs.putString(k.c_str(), val);
  prefs.end();
}

bool loadLearnedCode(String devId, String btnName, uint64_t &code, uint8_t &bits, decode_type_t &proto) {
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
  proto = (decode_type_t)val.substring(c3+1).toInt();
  return true;
}

void deleteLearnedCode(String devId, String btnName) {
  prefs.begin("irlearn", false);
  prefs.remove(learnKey(devId, btnName).c_str());
  prefs.end();
}

// ── IR send ─────────────────────────────────────────────────

void sendCode(decode_type_t proto, uint64_t code, uint8_t bits, uint8_t repeat) {
  for (int r = 0; r < repeat; r++) {
    irsend.send(proto, code, bits);
    if (repeat > 1) delayMicroseconds(40000);
  }
}

// ── Builder: devices JSON ───────────────────────────────────

String buildDevicesJSON() {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < DEVICE_COUNT; i++) {
    DeviceIR& d = devices[i];
    JsonObject obj = arr.createNestedObject();
    obj["id"]   = d.id;
    obj["name"] = d.name;
    obj["cat"]  = d.category;
    obj["catLabel"] = d.catLabel;
    obj["brand"]    = d.brand;
    JsonArray btns = obj.createNestedArray("btns");
    for (int j = 0; j < d.btnCount; j++) {
      IRButton& b = d.buttons[j];
  uint64_t lCode; uint8_t lBits; decode_type_t lProto;
      bool hasLearned = loadLearnedCode(d.id, b.name, lCode, lBits, lProto);
      JsonObject btnObj = btns.createNestedObject();
      btnObj["name"]    = b.name;
      btnObj["icon"]    = b.icon;
      btnObj["learned"] = hasLearned;
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}

// ── Handlers ────────────────────────────────────────────────

void handleIRSend() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  String devId   = server.arg("device");
  String btnName = server.arg("btn");
  if (!validateArg(devId) || !validateArg(btnName)) {
    server.send(400, "application/json", "{\"error\":\"invalid args\"}");
    return;
  }
  if (!irRateCheck()) {
    server.send(429, "application/json", "{\"error\":\"rate limit exceeded\"}");
    return;
  }

  uint64_t lCode; uint8_t lBits; decode_type_t lProto;
  if (loadLearnedCode(devId, btnName, lCode, lBits, lProto)) {
    sendCode(lProto, lCode, lBits, 1);
    Serial.printf("[IR-L] %s > %s  code=0x%llX\n", devId.c_str(), btnName.c_str(), lCode);
    server.send(200, "application/json", "{\"ok\":true,\"src\":\"learned\"}");
    return;
  }

  for (int i = 0; i < DEVICE_COUNT; i++) {
    if (devId != devices[i].id) continue;
    for (int j = 0; j < devices[i].btnCount; j++) {
      if (btnName != devices[i].buttons[j].name) continue;
      IRButton& b = devices[i].buttons[j];
      sendCode(devices[i].protocol, b.code, b.bits, b.repeat);
      Serial.printf("[IR-S] %s > %s  code=0x%llX\n", devId.c_str(), btnName.c_str(), b.code);
      server.send(200, "application/json", "{\"ok\":true,\"src\":\"library\"}");
      return;
    }
  }

  // irdb fallback — live lookup from CDN
  if (isWiFiConnected()) {
    String devCategory = "TV";
    String devBrand = "Samsung";
    for (int i = 0; i < DEVICE_COUNT; i++) {
      if (devId == devices[i].id) {
        devCategory = devices[i].category;
        devBrand = devices[i].brand;
        break;
      }
    }
    String irdbType = "TV";
    if (devCategory == "ac") irdbType = "AC";
    if (devCategory == "other") irdbType = "DVD_Player";

    std::vector<IRCode> codes;
    if (irdbClient.fetchCodesForBrand(devBrand.c_str(), irdbType.c_str(), codes)) {
      for (const auto& c : codes) {
        String btnName(c.functionName);
        if (btnName.startsWith("KEY_")) btnName = btnName.substring(4);
        if (btnName == server.arg("btn") || btnName == "KEY_" + server.arg("btn")) {
          decode_type_t dType;
          uint64_t dData;
          uint16_t dBits;
          uint8_t dRepeat = 3;
          if (irdbPackCode(c, dType, dData, dBits, dRepeat)) {
            for (uint8_t r = 0; r < dRepeat; r++) {
              irsend.send(dType, dData, dBits);
              if (dRepeat > 1) delayMicroseconds(40000);
            }
            Serial.printf("[IR-IRDB] %s > %s  proto=%s\n", devId.c_str(), btnName.c_str(), c.protocol.c_str());
            server.send(200, "application/json", "{\"ok\":true,\"src\":\"irdb\"}");
            return;
          }
        }
      }
    }
  }
  server.send(404, "application/json", "{\"error\":\"not found\",\"src\":\"not_found\"}");
}

void handleLearnStart() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  String devId = server.arg("device");
  String btn   = server.arg("btn");
  if (!validateArg(devId) || !validateArg(btn)) {
    server.send(400, "application/json", "{\"error\":\"invalid args\"}");
    return;
  }
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  learnDevice = devId;
  learnBtn    = btn;
  learnMode   = true;
  learnStart  = millis();
  learnDone   = false;
  irrecv.enableIRIn();
  Serial.printf("[LEARN] Start: %s / %s\n", learnDevice.c_str(), learnBtn.c_str());
  server.send(200, "application/json", "{\"ok\":true,\"timeout\":15}");
}

void handleLearnStatus() {
  if (learnDone) {
    learnDone = false;
    server.send(200, "application/json", "{\"status\":\"done\"}");
    return;
  }
  if (!learnMode) {
    server.send(200, "application/json", "{\"status\":\"idle\"}");
    return;
  }
  if (millis() - learnStart > LEARN_TIMEOUT) {
    learnMode = false;
    irrecv.disableIRIn();
    server.send(200, "application/json", "{\"status\":\"timeout\"}");
    return;
  }
  uint32_t rem = (LEARN_TIMEOUT - (millis() - learnStart)) / 1000;
  server.send(200, "application/json", "{\"status\":\"waiting\",\"remaining\":" + String(rem) + "}");
}

void handleLearnCancel() {
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  learnMode = false;
  learnDone = false;
  irrecv.disableIRIn();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleLearnDelete() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  String devId = server.arg("device");
  String btn   = server.arg("btn");
  if (!validateArg(devId) || !validateArg(btn)) {
    server.send(400, "application/json", "{\"error\":\"invalid args\"}");
    return;
  }
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  deleteLearnedCode(devId, btn);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleCSRFToken() {
  String json = "{\"token\":\"" + String(csrfToken) + "\"}";
  server.send(200, "application/json", json);
}

void handleDeviceList() {
  server.send(200, "application/json", buildDevicesJSON());
}

void handleWiFiStatus() {
  DynamicJsonDocument doc(128);
  doc["sta_connected"] = isWiFiConnected();
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleWiFiConfig() {
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  if (!server.hasArg("ssid")) {
    server.send(400, "application/json", "{\"error\":\"missing ssid\"}");
    return;
  }
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  if (!validateArg(ssid, 32) || (pass.length() > 0 && !validateArg(pass, 64))) {
    server.send(400, "application/json", "{\"error\":\"invalid args\"}");
    return;
  }
  saveWiFiConfig(ssid, pass);
  WiFi.begin(ssid.c_str(), pass.c_str());
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── irdb handlers ──────────────────────────────────────────

void handleIRDBBrands() {
  if (!isWiFiConnected()) {
    server.send(400, "application/json", "{\"error\":\"WiFi not connected\"}");
    return;
  }
  String type = server.arg("type");
  if (type.length() == 0) type = "TV";

  String raw;
  if (!irdbClient.fetchBrandList(type.c_str(), raw)) {
    server.send(502, "application/json", "{\"error\":\"failed to fetch brands\"}");
    return;
  }
  String json = irdbClient.brandListJson(raw);
  server.send(200, "application/json", json);
}

void handleIRDBCodes() {
  if (!isWiFiConnected()) {
    server.send(400, "application/json", "{\"error\":\"WiFi not connected\"}");
    return;
  }
  if (!server.hasArg("brand") || !server.hasArg("type")) {
    server.send(400, "application/json", "{\"error\":\"missing brand or type\"}");
    return;
  }
  String brand = server.arg("brand");
  String type  = server.arg("type");

  std::vector<IRCode> codes;
  if (!irdbClient.fetchCodesForBrand(brand.c_str(), type.c_str(), codes)) {
    server.send(502, "application/json", "{\"error\":\"failed to fetch codes\"}");
    return;
  }

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& c : codes) {
    JsonObject obj = arr.createNestedObject();
    String name = c.functionName;
    if (name.startsWith("KEY_")) name = name.substring(4);
    obj["fn"]  = name;
    obj["proto"] = c.protocol;
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleIRDBPage() {
  server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html><html lang="fa" dir="rtl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>IR Remote — IRDB</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:system-ui,-apple-system,sans-serif;background:#0d0d14;color:#e8e6f0;padding:20px;min-height:100vh}
  h1{font-size:24px;font-weight:700;margin-bottom:20px;color:#7c6af5}
  .card{background:#16161f;border:1px solid #2a2a3d;border-radius:14px;padding:20px;margin-bottom:16px}
  label{display:block;font-size:12px;color:#6b6882;margin-bottom:6px}
  select{width:100%;padding:11px 14px;background:#0d0d14;border:1px solid #2a2a3d;border-radius:10px;color:#e8e6f0;font-size:14px;margin-bottom:16px}
  select:focus{outline:none;border-color:#7c6af5}
  button{padding:12px 24px;border-radius:980px;border:none;background:#7c6af5;color:#fff;font-size:14px;font-weight:600;cursor:pointer;font-family:inherit}
  button:active{transform:scale(.95)}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px;margin-top:16px}
  .code-card{background:#1e1e2c;border:1px solid #2a2a3d;border-radius:10px;padding:12px;text-align:center}
  .code-card .fn{font-size:13px;font-weight:600;margin-bottom:6px}
  .code-card .proto{font-size:11px;color:#6b6882}
  .code-card .send-btn{margin-top:8px;padding:6px 16px;border-radius:980px;border:1px solid #7c6af5;background:transparent;color:#7c6af5;font-size:11px;cursor:pointer;font-family:inherit}
  .code-card .send-btn:active{transform:scale(.95)}
  .back{display:inline-block;margin-bottom:16px;color:#6b6882;text-decoration:none;font-size:13px}
  .status{font-size:12px;color:#6b6882;margin-bottom:12px}
</style>
</head><body>
<a href="/" class="back">← بازگشت</a>
<h1>📡 IRDB Browser</h1>
<div class="card">
  <label>نوع دستگاه</label>
  <select id="devType" onchange="loadBrands()">
    <option value="TV">تلویزیون</option>
    <option value="AC">کولر گازی</option>
    <option value="DVD_Player">DVD Player</option>
    <option value="Projector">پروژکتور</option>
    <option value="SetTopBox">ست‌تاپ باکس</option>
    <option value="Audio">صوتی</option>
    <option value="Fan">پنکه</option>
  </select>
  <label>برند</label>
  <select id="brandSelect" disabled onchange="enableFetch()">
    <option>ابتدا نوع را انتخاب کنید</option>
  </select>
  <div style="margin-top:12px">
    <button id="fetchBtn" disabled onclick="fetchCodes()">🔍 دریافت کدها</button>
  </div>
</div>
<div id="status" class="status"></div>
<div class="grid" id="results"></div>
<script>
async function loadBrands(){const t=document.getElementById('devType').value;const s=document.getElementById('brandSelect');s.disabled=true;s.innerHTML='<option>در حال بارگذاری...</option>';try{const r=await fetch('/irdb/brands?type='+encodeURIComponent(t));const brands=await r.json();s.innerHTML='<option value="">انتخاب برند</option>'+brands.map(b=>'<option value="'+b+'">'+b+'</option>').join('');s.disabled=false}catch(e){s.innerHTML='<option>خطا در بارگذاری</option>'}}
function enableFetch(){document.getElementById('fetchBtn').disabled=false}
async function fetchCodes(){const t=document.getElementById('devType').value;const b=document.getElementById('brandSelect').value;if(!b)return;const status=document.getElementById('status');status.textContent='⏳ در حال دریافت...';document.getElementById('results').innerHTML='';try{const r=await fetch('/irdb/codes?brand='+encodeURIComponent(b)+'&type='+encodeURIComponent(t));if(!r.ok){status.textContent='❌ خطا در دریافت';return}const codes=await r.json();const grid=document.getElementById('results');grid.innerHTML='';codes.forEach(c=>{const card=document.createElement('div');card.className='code-card';card.innerHTML='<div class="fn">'+c.fn+'</div><div class="proto">'+c.proto+'</div><button class="send-btn" onclick="sendIRDB(\''+c.fn+'\')">📡 ارسال</button>';grid.appendChild(card)});status.textContent='✅ '+codes.length+' کد یافت شد'}catch(e){status.textContent='❌ خطا'}}
async function sendIRDB(fn){try{await fetch('/ir?device=irdb_temp&btn='+encodeURIComponent(fn))}catch(e){}}
</script>
</body></html>
)rawliteral");
}

// ── Scenes handlers ────────────────────────────────────────

void handleScenesList() {
  server.send(200, "application/json", scenesJSON());
}

void handleScenesCreate() {
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  if (!server.hasArg("body") && server.method() != HTTP_POST) {
    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
    return;
  }
  String body = server.hasArg("body") ? server.arg("body") : "";
  if (body.length() == 0 && server.hasArg("plain")) body = server.arg("plain");
  if (body.length() == 0) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return; }

  Scene s;
  s.id   = "scene_" + String(millis(), HEX);
  s.name = doc["name"].as<String>();
  s.trigger.device = doc["trigger"]["device"].as<String>();
  s.trigger.btn    = doc["trigger"]["btn"].as<String>();
  s.action.type    = doc["action"]["type"].as<String>();
  s.action.device  = doc["action"]["device"].as<String>();
  s.action.state   = doc["action"]["state"].as<bool>();

  std::vector<Scene> scenes = loadScenes();
  scenes.push_back(s);
  saveScenes(scenes);

  String resp = "{\"ok\":true,\"id\":\"" + s.id + "\"}";
  server.send(200, "application/json", resp);
}

void handleScenesDelete() {
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"missing id\"}");
    return;
  }
  String id = server.arg("id");
  if (!validateArg(id)) {
    server.send(400, "application/json", "{\"error\":\"invalid id\"}");
    return;
  }
  std::vector<Scene> scenes = loadScenes();
  std::vector<Scene> filtered;
  for (const auto& s : scenes) {
    if (s.id != id) filtered.push_back(s);
  }
  if (filtered.size() == scenes.size()) {
    server.send(404, "application/json", "{\"error\":\"scene not found\"}");
    return;
  }
  saveScenes(filtered);
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── Smart Home page ───────────────────────────────────────

void handleSmartHomeSensors() {
  SensorData data = shClient.getSensorData();
  DynamicJsonDocument doc(256);
  if (isnan(data.temperature)) {
    doc["ok"] = false;
  } else {
    doc["ok"] = true;
    doc["temperature"] = data.temperature;
    doc["humidity"]    = data.humidity;
    doc["light"]       = data.light;
    doc["gas"]         = data.gas;
    doc["alarm"]       = data.alarm;
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleSmartHomeDevice() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"POST required\"}");
    return;
  }
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.length() == 0) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  DynamicJsonDocument doc(128);
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return; }
  String device = doc["device"].as<String>();
  bool status  = doc["status"].as<bool>();
  if (device.length() == 0) { server.send(400, "application/json", "{\"error\":\"missing device\"}"); return; }
  bool ok = shClient.toggleDevice(device, status);
  server.send(ok ? 200 : 502, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed\"}");
}

void handleSmartHomeAlarm() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"POST required\"}");
    return;
  }
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.length() == 0) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  DynamicJsonDocument doc(128);
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return; }
  bool status = doc["status"].as<bool>();
  bool ok = shClient.toggleAlarm(status);
  server.send(ok ? 200 : 502, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed\"}");
}

void handleSmartHomeSmart() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"POST required\"}");
    return;
  }
  if (!csrfCheck()) {
    server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.length() == 0) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  DynamicJsonDocument doc(128);
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return; }
  uint8_t mode = doc["status"].as<uint8_t>();
  if (mode > 2) { server.send(400, "application/json", "{\"error\":\"invalid mode\"}"); return; }
  bool ok = shClient.setSmartMode(mode);
  server.send(ok ? 200 : 502, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed\"}");
}

void handleSmartHomeConfig() {
  if (server.method() == HTTP_POST) {
    if (!csrfCheck()) {
      server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
      return;
    }
    if (server.hasArg("ip")) {
      String ip = server.arg("ip");
      if (!validateArg(ip, 40)) {
        server.send(400, "application/json", "{\"error\":\"invalid ip\"}");
        return;
      }
      shClient.saveIP(ip);
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
    server.send(400, "application/json", "{\"error\":\"missing ip\"}");
    return;
  }
  String json = "{\"ip\":\"" + shClient.getSavedIP() + "\"}";
  server.send(200, "application/json", json);
}

void handleSmartHomePage() {
  server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html><html lang="fa" dir="rtl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>IR Remote — Smart Home</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:system-ui,-apple-system,sans-serif;background:#0d0d14;color:#e8e6f0;padding:20px;min-height:100vh}
  h1{font-size:24px;font-weight:700;margin-bottom:20px;color:#7c6af5}
  .card{background:#16161f;border:1px solid #2a2a3d;border-radius:14px;padding:20px;margin-bottom:16px}
  label{display:block;font-size:12px;color:#6b6882;margin-bottom:6px}
  input,select{width:100%;padding:11px 14px;background:#0d0d14;border:1px solid #2a2a3d;border-radius:10px;color:#e8e6f0;font-size:14px}
  input:focus,select:focus{outline:none;border-color:#7c6af5}
  button{padding:12px 24px;border-radius:980px;border:none;background:#7c6af5;color:#fff;font-size:14px;font-weight:600;cursor:pointer;font-family:inherit;margin-top:8px}
  button:active{transform:scale(.95)}
  button.secondary{background:transparent;border:1px solid #2a2a3d;color:#6b6882}
  .sensor-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin:12px 0}
  .sensor-card{background:#1e1e2c;border:1px solid #2a2a3d;border-radius:12px;padding:16px;text-align:center}
  .sensor-card .sensor-val{font-size:28px;font-weight:700;color:#7c6af5}
  .sensor-card .sensor-label{font-size:12px;color:#6b6882;margin-top:4px}
  .sensor-card .alarm-dot{width:14px;height:14px;border-radius:50%;margin:8px auto;background:#3ecf8e}
  .sensor-card .alarm-dot.active{background:#e44d7b;box-shadow:0 0 12px #e44d7b;animation:pulse 1s infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
  .device-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin:12px 0}
  .dev-toggle{display:flex;align-items:center;justify-content:space-between;background:#1e1e2c;border:1px solid #2a2a3d;border-radius:12px;padding:14px 16px}
  .dev-toggle .dev-name{font-size:14px;font-weight:600}
  .toggle-sw{width:56px;height:28px;border-radius:14px;border:none;cursor:pointer;font-size:11px;font-weight:600}
  .toggle-sw.on{background:#3ecf8e;color:#fff}
  .toggle-sw.off{background:#2a2a3d;color:#6b6882}
  .scene-card{background:#1e1e2c;border:1px solid #2a2a3d;border-radius:12px;padding:14px;margin-bottom:8px;display:flex;align-items:center;justify-content:space-between}
  .scene-card .sc-name{font-size:13px;font-weight:600}
  .scene-card .sc-trigger{font-size:11px;color:#6b6882}
  .scene-card .sc-del{padding:6px 12px;border-radius:980px;border:1px solid #e44d7b44;background:transparent;color:#e44d7b;font-size:11px;cursor:pointer}
  .back{display:inline-block;margin-bottom:16px;color:#6b6882;text-decoration:none;font-size:13px}
  .conn-status{font-size:12px;color:#6b6882;display:flex;align-items:center;gap:6px;margin-top:8px}
  .conn-dot{width:8px;height:8px;border-radius:50%;flex-shrink:0}
  .conn-dot.ok{background:#3ecf8e}
  .conn-dot.fail{background:#e44d7b}
  .smart-mode{display:flex;gap:8px;margin-top:8px}
  .smart-mode button{flex:1;padding:10px;border-radius:10px;border:1px solid #2a2a3d;background:transparent;color:#6b6882;font-size:12px;cursor:pointer;font-family:inherit;margin:0}
  .smart-mode button:active{transform:scale(.95)}
  .smart-mode button.active{background:#7c6af5;color:#fff;border-color:#7c6af5}
  .sensor-banner{display:none;background:#e44d7b22;border:1px solid #e44d7b44;border-radius:10px;padding:10px;margin-top:8px;font-size:13px;color:#e44d7b;text-align:center}
  .sensor-banner.show{display:block}
</style>
</head><body>
<a href="/" class="back">← Back</a>
<h1>Smart Home</h1>
<div class="card">
  <label>Board IP Address</label>
  <input type="text" id="shIp" value="192.168.4.1" placeholder="192.168.4.1">
  <div style="display:flex;gap:8px">
    <button onclick="saveIP()">Save</button>
    <button class="secondary" onclick="retrySensorPoll()">Refresh</button>
  </div>
  <div class="conn-status" id="connStatus">
    <span class="conn-dot fail" id="connDot"></span>
    <span id="connText">Not connected</span>
  </div>
  <div id="sensorBanner" class="sensor-banner">Board unreachable — polling stopped <button onclick="retrySensorPoll()" style="margin-left:8px;padding:4px 12px;border-radius:980px;border:1px solid #e44d7b;background:transparent;color:#e44d7b;cursor:pointer;font-family:inherit;font-size:12px">Retry</button></div>
</div>
<div class="card">
  <h3 style="font-size:15px;margin-bottom:12px">Sensors</h3>
  <div class="sensor-grid" id="sensorGrid">
    <div class="sensor-card"><div class="sensor-val" id="sTemp">--</div><div class="sensor-label">Temperature</div></div>
    <div class="sensor-card"><div class="sensor-val" id="sHum">--</div><div class="sensor-label">Humidity</div></div>
    <div class="sensor-card"><div class="sensor-val" id="sLight">--</div><div class="sensor-label">Light</div></div>
    <div class="sensor-card"><div class="sensor-val" id="sGas">--</div><div class="sensor-label">Gas</div></div>
    <div class="sensor-card" style="grid-column:span 2"><div class="alarm-dot" id="alarmDot"></div><div class="sensor-label" id="alarmLabel">Alarm: Inactive</div></div>
  </div>
</div>
<div class="card">
  <h3 style="font-size:15px;margin-bottom:12px">Devices</h3>
  <div class="device-grid" id="deviceGrid">
    <div class="dev-toggle"><span class="dev-name">Lamp</span><button class="toggle-sw off" id="devLamp" onclick="toggleDev('lamp')">OFF</button></div>
    <div class="dev-toggle"><span class="dev-name">Cooling</span><button class="toggle-sw off" id="devCooling" onclick="toggleDev('cooling')">OFF</button></div>
  </div>
</div>
<div class="card">
  <h3 style="font-size:15px;margin-bottom:12px">Alarm</h3>
  <div class="dev-toggle"><span class="dev-name">Security Alarm</span><button class="toggle-sw off" id="devAlarm" onclick="toggleAlarm()">OFF</button></div>
</div>
<div class="card">
  <h3 style="font-size:15px;margin-bottom:12px">Smart Mode</h3>
  <p style="font-size:12px;color:#6b6882;margin-bottom:8px">When smart mode is on, the board responds automatically to LDR (light) or PIR (motion) sensors</p>
  <div class="smart-mode">
    <button id="smartOff" class="active" onclick="setSmart(0)">Off</button>
    <button id="smartLdr" onclick="setSmart(1)">LDR</button>
    <button id="smartPir" onclick="setSmart(2)">PIR</button>
  </div>
</div>
<div class="card">
  <h3 style="font-size:15px;margin-bottom:12px">Scenes — IR → Smart Home</h3>
  <div style="display:grid;gap:8px;margin-bottom:12px">
    <select id="sceneDevice"><option value="">Select IR device</option></select>
    <input type="text" id="sceneBtn" placeholder="Button name (e.g. Power)">
    <select id="sceneTarget">
      <option value="lamp">Lamp</option>
      <option value="cooling">Cooling</option>
      <option value="alarm">Alarm</option>
    </select>
    <select id="sceneState"><option value="true">On</option><option value="false">Off</option></select>
    <button onclick="addScene()">Add Scene</button>
  </div>
  <div id="sceneList"></div>
</div>
<script>
let sensorFailCount=0;let sensorTimer=null;let sensorStopped=false;let CSRFToken='';
fetch('/csrf').then(r=>r.json()).then(d=>CSRFToken=d.token);
async function saveIP(){const ip=document.getElementById('shIp').value.trim();if(!ip)return;await fetch('/smarthome/config?csrf='+CSRFToken,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ip='+encodeURIComponent(ip)});retrySensorPoll()}
async function loadSensors(){if(sensorStopped)return;try{const r=await fetch('/smarthome/sensors');const d=await r.json();const ok=d.ok;sensorFailCount=0;document.getElementById('connDot').className='conn-dot '+(ok?'ok':'fail');document.getElementById('connText').textContent=ok?'Connected':'Failed to reach board';document.getElementById('sensorBanner').classList.remove('show');if(!ok){scheduleSensorPoll();return}document.getElementById('sTemp').textContent=d.temperature!==null?d.temperature+'°C':'--';document.getElementById('sHum').textContent=d.humidity!==null?d.humidity+'%':'--';document.getElementById('sLight').textContent=d.light!==null?d.light+'%':'--';document.getElementById('sGas').textContent=d.gas!==null?d.gas+'%':'--';const alarmDot=document.getElementById('alarmDot');const alarmLabel=document.getElementById('alarmLabel');if(d.alarm){alarmDot.className='alarm-dot active';alarmLabel.textContent='Alarm: Triggered!'}else{alarmDot.className='alarm-dot';alarmLabel.textContent='Alarm: Inactive'}}catch(e){sensorFailCount++;document.getElementById('connDot').className='conn-dot fail';document.getElementById('connText').textContent='Connection error';document.getElementById('sTemp').textContent='--';document.getElementById('sHum').textContent='--';document.getElementById('sLight').textContent='--';document.getElementById('sGas').textContent='--';if(sensorFailCount>=3){sensorStopped=true;document.getElementById('sensorBanner').classList.add('show');return}}scheduleSensorPoll()}function scheduleSensorPoll(){if(sensorTimer)clearTimeout(sensorTimer);const delay=Math.min(3000*Math.pow(2,sensorFailCount),30000);sensorTimer=setTimeout(loadSensors,delay)}function retrySensorPoll(){sensorStopped=false;sensorFailCount=0;document.getElementById('sensorBanner').classList.remove('show');loadSensors()}
async function toggleDev(name){const btn=document.getElementById('dev'+(name.charAt(0).toUpperCase()+name.slice(1)));const on=btn.classList.contains('off');btn.className='toggle-sw '+(on?'on':'off');btn.textContent=on?'ON':'OFF';try{await fetch('/smarthome/device?csrf='+CSRFToken,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({device:name,status:on})})}catch(e){}}
async function toggleAlarm(){const btn=document.getElementById('devAlarm');const on=btn.classList.contains('off');btn.className='toggle-sw '+(on?'on':'off');btn.textContent=on?'ON':'OFF';try{await fetch('/smarthome/alarm?csrf='+CSRFToken,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({status:on})})}catch(e){}}
async function setSmart(mode){document.querySelectorAll('.smart-mode button').forEach(b=>b.classList.remove('active'));document.getElementById(['smartOff','smartLdr','smartPir'][mode]).classList.add('active');try{await fetch('/smarthome/smart?csrf='+CSRFToken,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({status:mode})})}catch(e){}}
async function loadDevices(){try{const r=await fetch('/devices');const devs=await r.json();const sel=document.getElementById('sceneDevice');sel.innerHTML='<option value="">Select IR device</option>';devs.forEach(d=>{const opt=document.createElement('option');opt.value=d.id;opt.textContent=d.name+' ('+d.brand+')';sel.appendChild(opt)})}catch(e){}}
async function addScene(){const device=document.getElementById('sceneDevice').value;const btn=document.getElementById('sceneBtn').value.trim();const target=document.getElementById('sceneTarget').value;const state=document.getElementById('sceneState').value==='true';if(!device||!btn)return;await fetch('/scenes?csrf='+CSRFToken,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({name:device+' → '+btn,trigger:{device,btn},action:{type:'device',device:target,state}})});loadScenes();document.getElementById('sceneBtn').value=''}
async function loadScenes(){try{const r=await fetch('/scenes');const scenes=await r.json();const list=document.getElementById('sceneList');list.innerHTML='';scenes.forEach(s=>{const card=document.createElement('div');card.className='scene-card';card.innerHTML='<div><div class="sc-name">'+s.name+'</div><div class="sc-trigger">'+s.trigger.device+' / '+s.trigger.btn+' → '+s.action.device+'</div></div><button class="sc-del" onclick="deleteScene(\''+s.id+'\')">✕</button>';list.appendChild(card)})}catch(e){}}
async function deleteScene(id){await fetch('/scenes?id='+encodeURIComponent(id)+'&csrf='+CSRFToken,{method:'DELETE'});loadScenes()}
loadDevices();loadScenes();loadSensors();
</script>
</body></html>
)rawliteral");
}

// ── Vendor page ────────────────────────────────────────────

void handleVendorConfig() {
  if (server.method() == HTTP_POST) {
    if (!csrfCheck()) {
      server.send(403, "application/json", "{\"error\":\"invalid CSRF token\"}");
      return;
    }
    if (!server.hasArg("base_url") || !server.hasArg("api_key")) {
      server.send(400, "application/json", "{\"error\":\"missing fields\"}");
      return;
    }
    String baseUrl = server.arg("base_url");
    String apiKey  = server.arg("api_key");
    String serial  = server.arg("serial");
    if (baseUrl.length() > 128 || apiKey.length() > 128 || serial.length() > 64) {
      server.send(400, "application/json", "{\"error\":\"arg too long\"}");
      return;
    }
    vendorAPI.saveConfig(
      baseUrl,
      apiKey,
      server.arg("vendor_id").toInt(),
      server.arg("product_id").toInt(),
      serial
    );
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  server.send(200, "application/json", vendorAPI.getConfigJSON());
}

void handleVendorPage() {
  server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html><html lang="fa" dir="rtl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>IR Remote — وندور</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:system-ui,-apple-system,sans-serif;background:#0d0d14;color:#e8e6f0;padding:20px;min-height:100vh}
  h1{font-size:24px;font-weight:700;margin-bottom:20px;color:#7c6af5}
  .card{background:#16161f;border:1px solid #2a2a3d;border-radius:14px;padding:20px;margin-bottom:16px}
  label{display:block;font-size:12px;color:#6b6882;margin-bottom:6px}
  input{width:100%;padding:11px 14px;background:#0d0d14;border:1px solid #2a2a3d;border-radius:10px;color:#e8e6f0;font-size:14px;font-family:monospace}
  input:focus{outline:none;border-color:#7c6af5}
  .pill{padding:12px 24px;border-radius:980px;border:none;background:#7c6af5;color:#fff;font-size:14px;font-weight:600;cursor:pointer;margin-top:8px}
  .pill:active{transform:scale(.95)}
  .status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-left:6px}
  .status-dot.green{background:#3ecf8e;box-shadow:0 0 8px #3ecf8e;animation:pulse 2s infinite}
  .status-dot.amber{background:#f59e0b;box-shadow:0 0 8px #f59e0b}
  .status-dot.red{background:#e44d7b;box-shadow:0 0 8px #e44d7b}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
  .balance{font-size:34px;font-weight:600;text-align:center;padding:16px;color:#7c6af5}
  .back{display:inline-block;margin-bottom:16px;color:#6b6882;text-decoration:none;font-size:13px}
  .status-text{font-size:13px;margin-top:8px}
</style>
</head><body>
<a href="/" class="back">← بازگشت</a>
<h1>🔌 پنل وندور</h1>
<div class="card">
  <label>Base URL</label>
  <input type="text" id="vBaseUrl" value="https://iot.example.com/api/v1">
  <label>API Key</label>
  <input type="text" id="vApiKey" placeholder="your_api_key">
  <label>Vendor ID</label>
  <input type="number" id="vVendorId" value="0">
  <label>Product ID</label>
  <input type="number" id="vProductId" value="0">
  <label>Serial</label>
  <input type="text" id="vSerial" placeholder="005-IR-REMOTE-v2">
  <button class="pill" onclick="saveConfig()">🔗 ذخیره و اتصال</button>
  <div class="status-text" id="vStatus">
    <span class="status-dot red"></span> متصل نیست
  </div>
  <div class="balance" id="vBalance">—</div>
</div>
<script>
let CSRFToken='';fetch('/csrf').then(r=>r.json()).then(d=>CSRFToken=d.token);
async function loadConfig(){try{const r=await fetch('/vendor/config');const c=await r.json();document.getElementById('vBaseUrl').value=c.base_url||'https://iot.example.com/api/v1';document.getElementById('vApiKey').value=c.api_key||'';document.getElementById('vVendorId').value=c.vendor_id||0;document.getElementById('vProductId').value=c.product_id||0;document.getElementById('vSerial').value=c.serial||''}catch(e){}}
async function saveConfig(){const data=new URLSearchParams();data.append('base_url',document.getElementById('vBaseUrl').value.trim());data.append('api_key',document.getElementById('vApiKey').value.trim());data.append('vendor_id',document.getElementById('vVendorId').value);data.append('product_id',document.getElementById('vProductId').value);data.append('serial',document.getElementById('vSerial').value.trim());const r=await fetch('/vendor/config?csrf='+CSRFToken,{method:'POST',body:data});if(r.ok){document.getElementById('vStatus').innerHTML='<span class="status-dot green"></span> پیکربندی ذخیره شد'}else{document.getElementById('vStatus').innerHTML='<span class="status-dot red"></span> خطا در ذخیره'}}
loadConfig();
</script>
</body></html>
)rawliteral");
}


void handleRoot() {
  server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=5,user-scalable=no">
<title>IR Remote</title>
<style>
:root {
  --action-blue: #0066cc;
  --action-blue-dark: #2997ff;
  --ink: #1d1d1f;
  --ink-muted: #6e6e73;
  --canvas: #ffffff;
  --canvas-parchment: #f5f5f7;
  --surface-dark: #1d1d1f;
  --surface-tile-1: #272729;
  --hairline: #d2d2d7;
  --code-surface: #000000;
  --section-pad: 80px;
  --content-max: 980px;
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{font-family:SF Pro Display,SF Pro Text,system-ui,-apple-system,sans-serif;background:var(--canvas);color:var(--ink);min-height:100vh;-webkit-font-smoothing:antialiased}

/* ── Header (true black) ── */
header{
  background:#000;padding:14px 20px;display:flex;align-items:center;justify-content:space-between;
  position:sticky;top:0;z-index:200;color:#fff
}
.logo-text{font-size:17px;font-weight:600;letter-spacing:-0.374px}
.conn-dot{width:8px;height:8px;border-radius:50%;background:#30d158;box-shadow:0 0 6px #30d158;animation:pulse 2s infinite;flex-shrink:0}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

/* ── Navigation ── */
.top-nav{
  background:rgba(255,255,255,.72);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);
  border-bottom:1px solid var(--hairline);display:flex;padding:0 16px;position:sticky;top:52px;z-index:150;
  overflow-x:auto;scrollbar-width:none
}
.top-nav::-webkit-scrollbar{display:none}
.top-nav a{
  flex-shrink:0;padding:12px 16px;font-size:13px;font-weight:600;color:var(--ink-muted);
  text-decoration:none;border-bottom:2px solid transparent;transition:color .15s,border-color .15s
}
.top-nav a.active{color:var(--action-blue);border-bottom-color:var(--action-blue)}
.hamburger{display:none;background:none;border:none;padding:8px;cursor:pointer;color:#fff;font-size:20px}

/* ── Section tiles alternate ── */
.section{width:100%}
.section-white{background:var(--canvas)}
.section-parchment{background:var(--canvas-parchment)}
.section-dark{background:var(--surface-dark);color:#f5f5f7}
.section-inner{max-width:var(--content-max);margin:0 auto;padding:var(--section-pad) 20px}

/* ── Tab strip ── */
.tab-bar{
  display:grid;grid-template-columns:repeat(3,1fr);border-bottom:1px solid var(--hairline);
  background:var(--canvas);position:sticky;top:96px;z-index:100
}
.tab{
  padding:14px 8px;text-align:center;cursor:pointer;font-size:13px;font-weight:600;
  color:var(--ink-muted);border-bottom:2px solid transparent;transition:color .15s,border-color .15s
}
.tab.active{color:var(--action-blue);border-bottom-color:var(--action-blue)}

/* ── Container ── */
.container{max-width:var(--content-max);margin:0 auto;padding:24px 20px 100px}

/* ── Brand pills ── */
.brand-bar{display:flex;gap:8px;overflow-x:auto;padding-bottom:8px;margin-bottom:24px;scrollbar-width:none}
.brand-bar::-webkit-scrollbar{display:none}
.brand-pill{
  flex-shrink:0;padding:8px 18px;border:none;border-radius:980px;
  background:var(--canvas-parchment);color:var(--ink-muted);
  font-family:inherit;font-size:14px;font-weight:400;cursor:pointer;transition:all .15s;
  min-height:44px;min-width:44px
}
.brand-pill:active{transform:scale(.95)}
.brand-pill.active{background:var(--action-blue);color:#fff;font-weight:600}

/* ── Device grid ── */
.device-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:12px;margin-bottom:32px}
.dev-card{
  background:var(--canvas);border:1px solid var(--hairline);border-radius:18px;
  overflow:hidden;cursor:pointer;transition:border-color .15s;padding:20px
}
.dev-card:active{transform:scale(.95)}
.dev-card-head{display:flex;flex-direction:column;align-items:center;gap:8px;text-align:center}
.dev-icon-wrap{
  width:48px;height:48px;border-radius:12px;background:var(--canvas-parchment);
  display:flex;align-items:center;justify-content:center;font-size:22px;
  overflow:hidden;flex-shrink:0
}
.dev-name{font-size:17px;font-weight:600;letter-spacing:-0.374px;line-height:1.3}
.dev-brand{font-size:14px;font-weight:400;color:var(--ink-muted)}
.add-card{
  background:var(--canvas);border:1.5px dashed var(--hairline);border-radius:18px;
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  gap:8px;padding:24px 20px;cursor:pointer;text-align:center;min-height:140px;transition:border-color .15s
}
.add-card:active{transform:scale(.95);border-color:var(--action-blue)}
.add-icon-wrap{
  width:48px;height:48px;border-radius:12px;background:var(--canvas-parchment);
  display:flex;align-items:center;justify-content:center;font-size:22px;color:var(--ink-muted)
}
.add-label{font-size:14px;font-weight:400;color:var(--ink-muted)}

/* ── Remote bottom sheet ── */
.panel-bg{display:none;position:fixed;inset:0;z-index:290;background:rgba(0,0,0,.32)}
.panel-bg.show{display:block}
.remote-panel{
  position:fixed;bottom:0;left:0;right:0;z-index:300;
  background:var(--canvas);border-radius:20px 20px 0 0;
  max-height:75vh;overflow-y:auto;
  transform:translateY(100%);transition:transform .4s cubic-bezier(.22,1,.36,1);
  padding-bottom:env(safe-area-inset-bottom,20px)
}
.remote-panel.open{transform:translateY(0)}
.rp-handle{width:36px;height:4px;background:var(--hairline);border-radius:2px;margin:10px auto 8px}
.rp-header{padding:4px 20px 16px;display:flex;align-items:center;justify-content:space-between}
.rp-title{font-size:34px;font-weight:600;letter-spacing:-0.374px;line-height:1.1}
.rp-actions{display:flex;gap:8px}
.rp-close{
  padding:10px 20px;border:1px solid var(--hairline);border-radius:980px;
  background:transparent;color:var(--ink-muted);font-family:inherit;font-size:15px;
  font-weight:400;cursor:pointer;min-height:44px;transition:background .15s
}
.rp-close:active{transform:scale(.95)}
.rp-learn-btn{
  padding:10px 20px;border:1px solid var(--action-blue);border-radius:980px;
  background:transparent;color:var(--action-blue);font-family:inherit;font-size:15px;
  font-weight:400;cursor:pointer;min-height:44px;transition:all .15s
}
.rp-learn-btn:active{transform:scale(.95)}
.rp-learn-btn.on{background:var(--action-blue);color:#fff}

/* ── Button grid ── */
.btns-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;padding:0 20px 8px}
.ir-btn{
  background:var(--canvas);border:1px solid var(--hairline);border-radius:12px;
  padding:16px 8px;display:flex;flex-direction:column;align-items:center;gap:6px;
  cursor:pointer;font-family:inherit;color:var(--ink);font-size:13px;font-weight:400;
  text-align:center;transition:all .15s;min-height:44px
}
.ir-btn:active{transform:scale(.95);background:var(--canvas-parchment)}
.ir-btn .bi{font-size:22px;line-height:1}
.ir-btn[data-btn="Power"]{
  grid-column:span 3;flex-direction:row;justify-content:center;gap:10px;
  padding:16px;background:var(--action-blue);border-color:var(--action-blue);color:#fff;
  font-size:15px;font-weight:600
}
.ir-btn[data-btn="Power"]:active{background:#004999}
.ir-btn.learned{border-color:var(--action-blue)}
.ir-btn.learned::after{content:'';position:absolute;top:6px;right:6px;width:6px;height:6px;border-radius:50%;background:var(--action-blue)}
.ir-btn.sent{background:rgba(0,102,204,.08);border-color:var(--action-blue)}
.ir-btn.learn-target{
  border-color:var(--action-blue)!important;background:rgba(0,102,204,.1)!important;
  animation:blink .8s infinite
}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.4}}

/* ── Learn overlay ── */
.learn-overlay{
  display:none;position:fixed;inset:0;z-index:500;
  background:rgba(0,0,0,.4);backdrop-filter:blur(8px);
  align-items:center;justify-content:center
}
.learn-overlay.show{display:flex}
.learn-box{
  background:var(--canvas);border-radius:20px;padding:32px 24px;text-align:center;
  max-width:320px;width:90%
}
.learn-box h3{font-size:28px;font-weight:600;letter-spacing:-0.28px;color:var(--ink);margin-bottom:8px}
.learn-box p{font-size:17px;font-weight:400;color:var(--ink-muted);line-height:1.47;letter-spacing:-0.374px;margin-bottom:20px}
.learn-box p strong{color:var(--ink);font-weight:600}
.learn-box .countdown{font-size:56px;font-weight:600;color:var(--action-blue);letter-spacing:-0.374px;margin-bottom:24px}
.lb-cancel{
  padding:12px 28px;border:1px solid var(--hairline);border-radius:980px;
  background:transparent;color:var(--ink-muted);font-family:inherit;
  font-size:17px;font-weight:400;cursor:pointer;min-height:44px
}
.lb-cancel:active{transform:scale(.95)}

/* ── Add device modal ── */
.modal-overlay{
  display:none;position:fixed;inset:0;z-index:500;
  background:rgba(0,0,0,.4);backdrop-filter:blur(8px);
  align-items:flex-end;justify-content:center
}
.modal-overlay.show{display:flex}
.modal-box{
  background:var(--canvas);border-radius:20px 20px 0 0;padding:24px 20px 32px;
  width:100%;max-width:var(--content-max);max-height:85vh;overflow-y:auto
}
.modal-handle{width:36px;height:4px;background:var(--hairline);border-radius:2px;margin:0 auto 16px}
.modal-title{font-size:22px;font-weight:600;letter-spacing:-0.28px;margin-bottom:20px;text-align:center;color:var(--ink)}
.form-row{margin-bottom:16px}
.form-row label{display:block;font-size:13px;color:var(--ink-muted);margin-bottom:6px;font-weight:400}
.form-row input,.form-row select{
  width:100%;padding:12px 14px;background:var(--canvas-parchment);
  border:1px solid var(--hairline);border-radius:12px;color:var(--ink);
  font-family:inherit;font-size:15px;min-height:44px
}
.form-row input:focus,.form-row select:focus{outline:none;border-color:var(--action-blue)}
.btns-add-row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-top:8px}
.btn-add-btn{
  padding:10px 6px;border-radius:12px;border:1px solid var(--hairline);
  background:var(--canvas);color:var(--ink-muted);font-family:inherit;
  font-size:12px;cursor:pointer;text-align:center;min-height:44px;transition:all .15s
}
.btn-add-btn:active{transform:scale(.95)}
.btn-add-btn.sel{border-color:var(--action-blue);background:rgba(0,102,204,.08);color:var(--action-blue)}
.modal-submit{
  width:100%;margin-top:16px;padding:14px;border:none;border-radius:980px;
  background:var(--action-blue);color:#fff;font-family:inherit;
  font-size:17px;font-weight:400;cursor:pointer;min-height:44px
}
.modal-submit:active{transform:scale(.95)}
.modal-cancel{
  width:100%;margin-top:8px;padding:14px;border:1px solid var(--hairline);border-radius:980px;
  background:transparent;color:var(--ink-muted);font-family:inherit;font-size:15px;cursor:pointer;min-height:44px
}
.modal-cancel:active{transform:scale(.95)}

/* ── Toast ── */
.toast{
  position:fixed;bottom:100px;left:50%;transform:translateX(-50%) translateY(70px);
  background:var(--ink);color:#fff;padding:12px 24px;border-radius:980px;
  font-size:15px;font-weight:400;transition:transform .3s ease;z-index:999;
  white-space:nowrap;pointer-events:none;max-width:90vw;overflow:hidden;text-overflow:ellipsis
}
.toast.show{transform:translateX(-50%) translateY(0)}

/* ── Responsive ── */
@media(max-width:1068px){
  .device-grid{grid-template-columns:repeat(2,1fr)}
  .section-inner{padding:60px 20px}
}
@media(max-width:833px){
  .top-nav{overflow-x:auto}
  .hamburger{display:block}
  .section-inner{padding:40px 16px}
}
@media(max-width:640px){
  .device-grid{grid-template-columns:1fr}
  .remote-panel{max-height:85vh}
  .rp-title{font-size:28px}
  .section-inner{padding:32px 14px}
  .top-nav a{padding:10px 12px;font-size:12px}
  .brand-pill{font-size:13px;padding:6px 14px}
}
@media(max-width:374px){
  .dev-card{padding:14px}
  .btns-grid{gap:6px}
  .ir-btn{padding:12px 6px;font-size:12px}
}
</style>
</head>
<body>
<header>
  <button class="hamburger" onclick="toggleMenu()">☰</button>
  <div class="logo-text">IR Remote</div>
  <div class="conn-dot" id="connDot"></div>
</header>
<div class="top-nav" id="topNav">
  <a href="/" class="active">Remote</a>
  <a href="/wifi">WiFi</a>
  <a href="/smarthome">Smart Home</a>
  <a href="/vendor">Vendor</a>
  <a href="/irdb">IRDB</a>
</div>
<div class="tab-bar">
  <div class="tab active" data-cat="tv" onclick="switchTab('tv',this)">TV</div>
  <div class="tab" data-cat="ac" onclick="switchTab('ac',this)">AC</div>
  <div class="tab" data-cat="other" onclick="switchTab('other',this)">Other</div>
</div>
<div class="container">
  <div class="brand-bar" id="brandBar"></div>
  <div class="device-grid" id="deviceGrid"></div>
</div>
<div class="panel-bg" id="panelBg" onclick="closePanel()"></div>
<div class="remote-panel" id="remotePanel">
  <div class="rp-handle"></div>
  <div class="rp-header">
    <span class="rp-title" id="rpTitle">—</span>
    <div class="rp-actions">
      <button class="rp-learn-btn" id="rpLearnBtn" onclick="toggleLearnMode()">Learn</button>
      <button class="rp-close" onclick="closePanel()">Close</button>
    </div>
  </div>
  <div class="btns-grid" id="rpBtns"></div>
</div>
<div class="learn-overlay" id="learnOverlay">
  <div class="learn-box">
    <h3>Waiting for IR Signal</h3>
    <p>Point your remote at the receiver and press <strong id="learnBtnName">—</strong></p>
    <div class="countdown" id="learnCountdown">15</div>
    <button class="lb-cancel" onclick="cancelLearn()">Cancel</button>
  </div>
</div>
<div class="modal-overlay" id="addModal">
  <div class="modal-box">
    <div class="modal-handle"></div>
    <div class="modal-title">Add New Device</div>
    <div class="form-row"><label>Device Name</label><input type="text" id="newDevName" placeholder="e.g. Living Room Projector"></div>
    <div class="form-row"><label>Brand</label><input type="text" id="newDevBrand" placeholder="e.g. Epson"></div>
    <div class="form-row"><label>Category</label><select id="newDevCat"><option value="tv">TV</option><option value="ac">AC</option><option value="other">Other</option></select></div>
    <div class="form-row"><label>Select buttons to learn</label><div class="btns-add-row" id="btnPickerGrid"></div></div>
    <button class="modal-submit" onclick="startAddDevice()">Start Learning</button>
    <button class="modal-cancel" onclick="closeAddModal()">Cancel</button>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
let DEVICES=[];let currentCat='tv';let currentBrand='all';let openDeviceId=null;let learnModeOn=false;let learnPolling=null;let learnCdInterval=null;let pendingLearnBtn=null;let learnQueue=[];let learnQueueIdx=0;let addDeviceData={};const selectedAddBtns=new Set();let CSRFToken='';
const PRESET_BTNS=[{name:"Power",icon:"⏻"},{name:"Vol+",icon:"🔊"},{name:"Vol-",icon:"🔉"},{name:"Mute",icon:"🔇"},{name:"CH+",icon:"⬆"},{name:"CH-",icon:"⬇"},{name:"OK",icon:"✔"},{name:"Up",icon:"▲"},{name:"Down",icon:"▼"},{name:"Left",icon:"◀"},{name:"Right",icon:"▶"},{name:"Back",icon:"↩"},{name:"Home",icon:"🏠"},{name:"Menu",icon:"☰"},{name:"Source",icon:"⎘"},{name:"Cool",icon:"❄"},{name:"Heat",icon:"🔥"},{name:"Fan",icon:"💨"},{name:"Temp+",icon:"🌡"},{name:"Temp-",icon:"🌡"},{name:"Play",icon:"▶"},{name:"Pause",icon:"⏸"},{name:"Stop",icon:"⏹"}];
function catIcon(cat){return cat==='tv'?'📺':cat==='ac'?'❄️':'📽️'}
async function loadDevices(){try{const r=await fetch('/devices');DEVICES=await r.json();renderAll()}catch(e){DEVICES=[];renderAll()}}
function renderAll(){renderBrandBar();renderGrid()}
function switchTab(cat,el){currentCat=cat;currentBrand='all';document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));el.classList.add('active');renderAll()}
function renderBrandBar(){const filtered=DEVICES.filter(d=>d.cat===currentCat);const brands=['all',...new Set(filtered.map(d=>d.brand))];const bar=document.getElementById('brandBar');bar.innerHTML='';brands.forEach(b=>{const el=document.createElement('button');el.className='brand-pill'+(b===currentBrand?' active':'');el.textContent=b==='all'?'All':b;el.onclick=()=>{currentBrand=b;renderBrandBar();renderGrid()};bar.appendChild(el)})}
function renderGrid(){const grid=document.getElementById('deviceGrid');grid.innerHTML='';const list=DEVICES.filter(d=>d.cat===currentCat&&(currentBrand==='all'||d.brand===currentBrand));list.forEach(dev=>{const card=document.createElement('div');card.className='dev-card';card.innerHTML='<div class="dev-card-head"><div class="dev-icon-wrap">'+catIcon(dev.cat)+'</div><div class="dev-name">'+dev.name+'</div><div class="dev-brand">'+dev.brand+'</div></div>';card.onclick=()=>openPanel(dev.id);grid.appendChild(card)});const add=document.createElement('div');add.className='add-card';add.innerHTML='<div class="add-icon-wrap">+</div><div class="add-label">New Device</div>';add.onclick=()=>openAddModal();grid.appendChild(add)}
function openPanel(devId){const dev=DEVICES.find(d=>d.id===devId);if(!dev)return;openDeviceId=devId;learnModeOn=false;document.getElementById('rpLearnBtn').classList.remove('on');document.getElementById('rpLearnBtn').textContent='Learn';document.getElementById('rpTitle').textContent=dev.name;renderPanelBtns(dev);document.getElementById('remotePanel').classList.add('open');document.getElementById('panelBg').classList.add('show')}
function closePanel(){document.getElementById('remotePanel').classList.remove('open');document.getElementById('panelBg').classList.remove('show');openDeviceId=null;learnModeOn=false;document.getElementById('rpLearnBtn').classList.remove('on');document.getElementById('rpLearnBtn').textContent='Learn'}
function renderPanelBtns(dev){const grid=document.getElementById('rpBtns');grid.innerHTML='';dev.btns.forEach(btn=>{const b=document.createElement('button');b.className='ir-btn'+(btn.learned?' learned':'');b.setAttribute('data-btn',btn.name);b.innerHTML='<span class="bi">'+btn.icon+'</span><span>'+btn.name+'</span>';b.onclick=()=>{if(learnModeOn){startLearnBtn(dev.id,btn.name,b)}else{sendIR(dev.id,btn.name,b)}};grid.appendChild(b)})}
function toggleLearnMode(){learnModeOn=!learnModeOn;const btn=document.getElementById('rpLearnBtn');btn.classList.toggle('on',learnModeOn);btn.textContent=learnModeOn?'Cancel':'Learn';if(learnModeOn){showToast('Tap a button to teach',false)}else{showToast('Learn mode off',false)}}
async function startLearnBtn(devId,btnName,btnEl){pendingLearnBtn={devId,btnName,el:btnEl};btnEl.classList.add('learn-target');document.getElementById('learnBtnName').textContent=btnName;document.getElementById('learnCountdown').textContent='15';document.getElementById('learnOverlay').classList.add('show');await fetch('/learn/start',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'device='+encodeURIComponent(devId)+'&btn='+encodeURIComponent(btnName)+'&csrf='+CSRFToken});startLearnPolling(15)}
function startLearnPolling(sec){let remaining=sec;clearInterval(learnCdInterval);clearInterval(learnPolling);learnCdInterval=setInterval(()=>{remaining--;document.getElementById('learnCountdown').textContent=remaining;if(remaining<=0)clearInterval(learnCdInterval)},1000);learnPolling=setInterval(async()=>{try{const r=await fetch('/learn/status');const j=await r.json();if(j.status==='done'){clearInterval(learnPolling);clearInterval(learnCdInterval);onLearnSuccess()}else if(j.status==='timeout'||j.status==='idle'){clearInterval(learnPolling);clearInterval(learnCdInterval);onLearnTimeout()}}catch(e){}},500)}
function onLearnSuccess(){document.getElementById('learnOverlay').classList.remove('show');if(pendingLearnBtn){pendingLearnBtn.el.classList.remove('learn-target');pendingLearnBtn.el.classList.add('learned');showToast('Code saved!',false);pendingLearnBtn=null}if(learnQueue.length>0){learnQueueIdx++;setTimeout(()=>processLearnQueue(),800)}else{learnModeOn=false;document.getElementById('rpLearnBtn').classList.remove('on');document.getElementById('rpLearnBtn').textContent='Learn'}loadDevices()}
function onLearnTimeout(){document.getElementById('learnOverlay').classList.remove('show');if(pendingLearnBtn){pendingLearnBtn.el.classList.remove('learn-target');pendingLearnBtn=null}if(learnQueue.length>0){if(confirm('No signal received. Skip this button?')){learnQueueIdx++;setTimeout(()=>processLearnQueue(),400)}else{learnQueue=[];showToast('Learning stopped',true)}}else{showToast('No signal received',true)}}
async function cancelLearn(){clearInterval(learnPolling);clearInterval(learnCdInterval);await fetch('/learn/cancel',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'csrf='+CSRFToken});document.getElementById('learnOverlay').classList.remove('show');if(pendingLearnBtn){pendingLearnBtn.el.classList.remove('learn-target');pendingLearnBtn=null}learnQueue=[];showToast('Learning cancelled',true)}
async function sendIR(devId,btnName,el){el.classList.add('sent');try{const r=await fetch('/ir?device='+encodeURIComponent(devId)+'&btn='+encodeURIComponent(btnName));await r.json();showToast(btnName,false)}catch(e){showToast('Error sending',true)}setTimeout(()=>el.classList.remove('sent'),800)}
function openAddModal(){selectedAddBtns.clear();renderBtnPicker();document.getElementById('newDevName').value='';document.getElementById('newDevBrand').value='';document.getElementById('addModal').classList.add('show')}
function closeAddModal(){document.getElementById('addModal').classList.remove('show')}
function renderBtnPicker(){const grid=document.getElementById('btnPickerGrid');grid.innerHTML='';PRESET_BTNS.forEach(b=>{const el=document.createElement('div');el.className='btn-add-btn'+(selectedAddBtns.has(b.name)?' sel':'');el.innerHTML=b.icon+' '+b.name;el.onclick=()=>{selectedAddBtns.has(b.name)?selectedAddBtns.delete(b.name):selectedAddBtns.add(b.name);renderBtnPicker()};grid.appendChild(el)})}
function startAddDevice(){const name=document.getElementById('newDevName').value.trim();const brand=document.getElementById('newDevBrand').value.trim();const cat=document.getElementById('newDevCat').value;if(!name||!brand){showToast('Enter name and brand',true);return}if(selectedAddBtns.size===0){showToast('Select at least one button',true);return}const id='custom_'+Date.now();addDeviceData={id,name,brand,cat};learnQueue=Array.from(selectedAddBtns).map(bName=>{const preset=PRESET_BTNS.find(p=>p.name===bName);return{devId:id,btnName:bName,icon:preset?preset.icon:'•'}});learnQueueIdx=0;closeAddModal();const catMap={tv:'TV',ac:'AC',other:'Other'};DEVICES.push({id,name,brand,cat,catLabel:catMap[cat],btns:learnQueue.map(l=>({name:l.btnName,icon:l.icon,learned:false}))});renderAll();openPanel(id);setTimeout(()=>processLearnQueue(),600)}
async function processLearnQueue(){if(learnQueueIdx>=learnQueue.length){showToast('All buttons learned!',false);learnQueue=[];return}const item=learnQueue[learnQueueIdx];const btnEls=document.querySelectorAll('#rpBtns .ir-btn');let btnEl=null;btnEls.forEach(el=>{if(el.getAttribute('data-btn')===item.btnName)btnEl=el});pendingLearnBtn={devId:item.devId,btnName:item.btnName,el:btnEl};if(btnEl)btnEl.classList.add('learn-target');document.getElementById('learnBtnName').textContent=item.btnName;document.getElementById('learnCountdown').textContent='15';document.getElementById('learnOverlay').classList.add('show');await fetch('/learn/start',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'device='+encodeURIComponent(item.devId)+'&btn='+encodeURIComponent(item.btnName)+'&csrf='+CSRFToken});startLearnPolling(15)}
function showToast(msg,err){const t=document.getElementById('toast');t.textContent=msg;t.className='toast show';setTimeout(()=>t.classList.remove('show'),2200)}
function toggleMenu(){document.getElementById('topNav').classList.toggle('open')}
fetch('/csrf').then(r=>r.json()).then(d=>CSRFToken=d.token);
loadDevices();
</script>
</body>
</html>
)rawliteral");
}
