// =============================================================
//  IR Remote v2.0  —  ESP32 Access Point + Learn Mode
//  فایل اصلی — وب‌سرور + یادگیری کد IR
// =============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ir_codes.h"

// ════════════════════════════════════════════════════════════
//  تنظیمات سخت‌افزار
// ════════════════════════════════════════════════════════════
const uint16_t IR_SEND_PIN = 4;    // GPIO فرستنده IR
const uint16_t IR_RECV_PIN = 14;   // GPIO گیرنده IR (CHQ1838)
const uint16_t CAPTURE_BUF = 1024;

// ════════════════════════════════════════════════════════════
//  تنظیمات Access Point
// ════════════════════════════════════════════════════════════
const char* AP_SSID     = "IR-Remote";
const char* AP_PASSWORD = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);

// ════════════════════════════════════════════════════════════
//  اشیاء اصلی
// ════════════════════════════════════════════════════════════
IRsend   irsend(IR_SEND_PIN);
IRrecv   irrecv(IR_RECV_PIN, CAPTURE_BUF, 50, true);
WebServer server(80);
Preferences prefs;

// ── وضعیت یادگیری ─────────────────────────────────────────
bool     learnMode   = false;
String   learnDevice = "";
String   learnBtn    = "";
uint32_t learnStart  = 0;
const uint32_t LEARN_TIMEOUT = 15000; // 15 ثانیه


// ════════════════════════════════════════════════════════════
//  EEPROM helpers — ذخیره/بازیابی کد آموخته‌شده
// ════════════════════════════════════════════════════════════
String learnKey(String devId, String btnName) {
  return "L_" + devId + "_" + btnName;
}

void saveLearnedCode(String devId, String btnName, uint64_t code, uint8_t bits, uint8_t proto) {
  prefs.begin("irlearn", false);
  String k = learnKey(devId, btnName);
  // ذخیره به صورت JSON رشته
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
  // parse
  int c1 = val.indexOf(':'), c2 = val.indexOf(':', c1+1), c3 = val.indexOf(':', c2+1);
  uint32_t hi = strtoul(val.substring(0,c1).c_str(), nullptr, 16);
  uint32_t lo = strtoul(val.substring(c1+1,c2).c_str(), nullptr, 16);
  code  = ((uint64_t)hi << 32) | lo;
  bits  = val.substring(c2+1, c3).toInt();
  proto = val.substring(c3+1).toInt();
  return true;
}

// حذف یک کد آموخته‌شده
void deleteLearnedCode(String devId, String btnName) {
  prefs.begin("irlearn", false);
  prefs.remove(learnKey(devId, btnName).c_str());
  prefs.end();
}


// ════════════════════════════════════════════════════════════
//  ساخت JSON لیست دستگاه‌ها (static + learned)
// ════════════════════════════════════════════════════════════
String buildDevicesJSON() {
  String json = "[";
  for (int i = 0; i < DEVICE_COUNT; i++) {
    DeviceIR& d = devices[i];
    json += "{";
    json += "\"id\":\"" + String(d.id) + "\",";
    json += "\"name\":\"" + String(d.name) + "\",";
    json += "\"cat\":\"" + String(d.category) + "\",";
    json += "\"catLabel\":\"" + String(d.catLabel) + "\",";
    json += "\"brand\":\"" + String(d.brand) + "\",";
    json += "\"btns\":[";
    for (int j = 0; j < d.btnCount; j++) {
      IRButton& b = d.buttons[j];
      uint64_t lCode; uint8_t lBits, lProto;
      bool hasLearned = loadLearnedCode(d.id, b.name, lCode, lBits, lProto);
      json += "{";
      json += "\"name\":\"" + String(b.name) + "\",";
      json += "\"icon\":\"" + String(b.icon) + "\",";
      json += "\"learned\":" + String(hasLearned ? "true" : "false");
      json += "}";
      if (j < d.btnCount - 1) json += ",";
    }
    json += "]}";
    if (i < DEVICE_COUNT - 1) json += ",";
  }
  json += "]";
  return json;
}


// ════════════════════════════════════════════════════════════
//  ارسال IR
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
//  Handler: ارسال IR از وب
// ════════════════════════════════════════════════════════════
void handleIRSend() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  String devId   = server.arg("device");
  String btnName = server.arg("btn");

  // اول چک کن کد آموخته‌شده داریم
  uint64_t lCode; uint8_t lBits, lProto;
  if (loadLearnedCode(devId, btnName, lCode, lBits, lProto)) {
    sendCode((IRProtocol)lProto, lCode, lBits, 1);
    Serial.printf("[IR-L] %s > %s  code=0x%llX\n", devId.c_str(), btnName.c_str(), lCode);
    server.send(200, "application/json", "{\"ok\":true,\"src\":\"learned\"}");
    return;
  }

  // جستجو در کتابخانه
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
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}


// ════════════════════════════════════════════════════════════
//  Handler: شروع حالت یادگیری
// ════════════════════════════════════════════════════════════
void handleLearnStart() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  learnDevice = server.arg("device");
  learnBtn    = server.arg("btn");
  learnMode   = true;
  learnStart  = millis();

  irrecv.enableIRIn();
  Serial.printf("[LEARN] Start: %s / %s\n", learnDevice.c_str(), learnBtn.c_str());
  server.send(200, "application/json", "{\"ok\":true,\"timeout\":15}");
}


// ════════════════════════════════════════════════════════════
//  Handler: وضعیت یادگیری (polling)
// ════════════════════════════════════════════════════════════
void handleLearnStatus() {
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
  server.send(200, "application/json", "{\"status\":\"waiting\",\"remaining\":" +
    String((LEARN_TIMEOUT - (millis() - learnStart)) / 1000) + "}");
}


// ════════════════════════════════════════════════════════════
//  Handler: لغو یادگیری
// ════════════════════════════════════════════════════════════
void handleLearnCancel() {
  learnMode = false;
  irrecv.disableIRIn();
  server.send(200, "application/json", "{\"ok\":true}");
}


// ════════════════════════════════════════════════════════════
//  Handler: حذف کد آموخته‌شده
// ════════════════════════════════════════════════════════════
void handleLearnDelete() {
  if (!server.hasArg("device") || !server.hasArg("btn")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }
  deleteLearnedCode(server.arg("device"), server.arg("btn"));
  server.send(200, "application/json", "{\"ok\":true}");
}


// ════════════════════════════════════════════════════════════
//  Handler: لیست دستگاه‌ها (JSON)
// ════════════════════════════════════════════════════════════
void handleDeviceList() {
  server.send(200, "application/json", buildDevicesJSON());
}


// ════════════════════════════════════════════════════════════
//  صفحه اصلی HTML
// ════════════════════════════════════════════════════════════
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>IR Remote</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Vazirmatn:wght@300;400;600;800&display=swap');
  :root {
    --bg:#0d0d14; --surface:#16161f; --card:#1e1e2c; --border:#2a2a3d;
    --accent:#7c6af5; --accent2:#e44d7b; --text:#e8e6f0; --muted:#6b6882;
    --success:#3ecf8e; --warn:#f59e0b; --r:14px;
  }
  *{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
  body{font-family:'Vazirmatn',sans-serif;background:var(--bg);color:var(--text);min-height:100vh}

  /* ── Header ── */
  header{
    background:linear-gradient(135deg,#1a1a2e,#16213e);
    border-bottom:1px solid var(--border);
    padding:14px 20px;
    display:flex;align-items:center;justify-content:space-between;
    position:sticky;top:0;z-index:200;
  }
  .logo{display:flex;align-items:center;gap:10px}
  .logo-icon{width:36px;height:36px;background:linear-gradient(135deg,var(--accent),var(--accent2));
    border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:18px}
  .logo-text{font-size:17px;font-weight:800}
  .logo-text span{color:var(--accent)}
  .hdr-right{display:flex;align-items:center;gap:10px}
  .learn-global-btn{
    padding:7px 14px;border-radius:20px;border:1px solid var(--accent);
    background:transparent;color:var(--accent);font-family:'Vazirmatn',sans-serif;
    font-size:12px;font-weight:600;cursor:pointer;transition:all .2s;
  }
  .learn-global-btn.active{background:var(--accent2);border-color:var(--accent2);color:#fff}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--success);
    box-shadow:0 0 8px var(--success);animation:pulse 2s infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

  /* ── Tab bar (TV / AC / Other) ── */
  .tab-bar{
    display:grid;grid-template-columns:repeat(3,1fr);
    background:var(--surface);border-bottom:1px solid var(--border);
    position:sticky;top:65px;z-index:100;
  }
  .tab{
    padding:14px 8px;text-align:center;cursor:pointer;
    font-size:12px;font-weight:600;color:var(--muted);
    border-bottom:2px solid transparent;transition:all .2s;
    display:flex;flex-direction:column;align-items:center;gap:4px;
  }
  .tab .tab-icon{font-size:22px}
  .tab.active{color:var(--accent);border-bottom-color:var(--accent)}

  /* ── Container ── */
  .container{max-width:520px;margin:0 auto;padding:16px 14px 80px}

  /* ── Brand filter pills ── */
  .brand-bar{
    display:flex;gap:7px;overflow-x:auto;padding-bottom:4px;
    margin-bottom:16px;scrollbar-width:none;
  }
  .brand-bar::-webkit-scrollbar{display:none}
  .brand-pill{
    flex-shrink:0;padding:6px 14px;border:1px solid var(--border);
    border-radius:20px;background:var(--surface);color:var(--muted);
    font-family:'Vazirmatn',sans-serif;font-size:12px;cursor:pointer;transition:all .2s;
  }
  .brand-pill.active{background:var(--accent);border-color:var(--accent);color:#fff;font-weight:600}

  /* ── Device grid ── */
  .device-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin-bottom:24px}

  .dev-card{
    background:var(--card);border:1px solid var(--border);border-radius:var(--r);
    overflow:hidden;transition:border-color .2s;cursor:pointer;
  }
  .dev-card:active{opacity:.85}
  .dev-card-head{padding:14px 12px;display:flex;flex-direction:column;align-items:center;gap:6px;text-align:center}
  .dev-cat-icon{font-size:32px}
  .dev-name{font-size:13px;font-weight:600;line-height:1.3}
  .dev-brand{font-size:11px;color:var(--muted)}

  /* ── Add device card ── */
  .add-card{
    background:var(--surface);border:1.5px dashed var(--border);
    border-radius:var(--r);display:flex;flex-direction:column;
    align-items:center;justify-content:center;gap:6px;
    padding:20px 12px;cursor:pointer;text-align:center;
    transition:border-color .2s;min-height:110px;
  }
  .add-card:active{border-color:var(--accent)}
  .add-icon{font-size:28px;color:var(--muted)}
  .add-label{font-size:12px;color:var(--muted)}

  /* ── Remote panel (دکمه‌های دستگاه) ── */
  .remote-panel{
    position:fixed;bottom:0;left:0;right:0;z-index:300;
    background:var(--surface);border-top:1px solid var(--border);
    border-radius:22px 22px 0 0;
    max-height:72vh;overflow-y:auto;
    transform:translateY(100%);transition:transform .35s cubic-bezier(.22,1,.36,1);
    padding-bottom:env(safe-area-inset-bottom,16px);
  }
  .remote-panel.open{transform:translateY(0)}

  .rp-handle{width:36px;height:4px;background:var(--border);border-radius:2px;
    margin:12px auto 8px}
  .rp-header{padding:0 16px 12px;display:flex;align-items:center;justify-content:space-between}
  .rp-title{font-size:15px;font-weight:700}
  .rp-actions{display:flex;gap:8px}
  .rp-close{padding:6px 12px;border-radius:20px;border:1px solid var(--border);
    background:transparent;color:var(--muted);font-size:12px;cursor:pointer;
    font-family:'Vazirmatn',sans-serif}
  .rp-learn-btn{padding:6px 14px;border-radius:20px;border:1px solid var(--accent2);
    background:transparent;color:var(--accent2);font-size:12px;font-weight:600;
    cursor:pointer;font-family:'Vazirmatn',sans-serif;transition:all .2s}
  .rp-learn-btn.on{background:var(--accent2);color:#fff}

  .btns-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;padding:0 12px 16px}
  .ir-btn{
    background:var(--card);border:1px solid var(--border);border-radius:10px;
    padding:14px 6px;display:flex;flex-direction:column;align-items:center;gap:5px;
    cursor:pointer;font-family:'Vazirmatn',sans-serif;color:var(--text);
    font-size:12px;text-align:center;position:relative;transition:all .15s;
  }
  .ir-btn .bi{font-size:20px;line-height:1}
  .ir-btn[data-btn="Power"]{
    grid-column:span 3;flex-direction:row;justify-content:center;gap:8px;
    padding:14px;background:linear-gradient(135deg,#e44d7b18,#e44d7b08);
    border-color:#e44d7b44;font-size:14px;font-weight:600;
  }
  .ir-btn:active{transform:scale(.94);background:var(--accent)22;border-color:var(--accent)}
  .ir-btn.learned{border-color:var(--success)}
  .ir-btn.learned::after{
    content:'●';position:absolute;top:4px;right:6px;
    font-size:8px;color:var(--success);
  }
  .ir-btn.sent{background:var(--success)18;border-color:var(--success)}
  .ir-btn.learn-target{
    border-color:var(--warn)!important;background:var(--warn)15!important;
    animation:blink .7s infinite;
  }
  @keyframes blink{0%,100%{opacity:1}50%{opacity:.5}}

  /* ── Learn overlay ── */
  .learn-overlay{
    display:none;position:fixed;inset:0;z-index:500;
    background:#000a;backdrop-filter:blur(4px);
    align-items:center;justify-content:center;
  }
  .learn-overlay.show{display:flex}
  .learn-box{
    background:var(--card);border:1px solid var(--accent2);
    border-radius:20px;padding:28px 24px;text-align:center;
    max-width:300px;width:90%;
  }
  .learn-box .lb-icon{font-size:48px;margin-bottom:12px}
  .learn-box h3{font-size:17px;font-weight:700;margin-bottom:8px}
  .learn-box p{font-size:13px;color:var(--muted);line-height:1.6;margin-bottom:16px}
  .learn-box .countdown{font-size:28px;font-weight:800;color:var(--accent2);margin-bottom:20px}
  .lb-cancel{
    padding:10px 24px;border-radius:20px;border:1px solid var(--border);
    background:transparent;color:var(--muted);font-family:'Vazirmatn',sans-serif;
    font-size:13px;cursor:pointer;
  }

  /* ── Add device modal ── */
  .modal-overlay{
    display:none;position:fixed;inset:0;z-index:500;
    background:#000b;backdrop-filter:blur(4px);
    align-items:flex-end;justify-content:center;
  }
  .modal-overlay.show{display:flex}
  .modal-box{
    background:var(--card);border:1px solid var(--border);
    border-radius:22px 22px 0 0;padding:20px;
    width:100%;max-width:520px;
    max-height:85vh;overflow-y:auto;
  }
  .modal-handle{width:36px;height:4px;background:var(--border);border-radius:2px;margin:0 auto 16px}
  .modal-title{font-size:16px;font-weight:700;margin-bottom:16px;text-align:center}
  .form-row{margin-bottom:14px}
  .form-row label{display:block;font-size:12px;color:var(--muted);margin-bottom:6px}
  .form-row input, .form-row select{
    width:100%;padding:11px 14px;background:var(--surface);
    border:1px solid var(--border);border-radius:10px;
    color:var(--text);font-family:'Vazirmatn',sans-serif;font-size:14px;
  }
  .form-row input:focus, .form-row select:focus{outline:none;border-color:var(--accent)}
  .btns-add-row{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}
  .btn-add-btn{
    padding:10px 6px;border-radius:10px;border:1px solid var(--border);
    background:var(--surface);color:var(--muted);font-family:'Vazirmatn',sans-serif;
    font-size:11px;cursor:pointer;text-align:center;
  }
  .btn-add-btn.sel{border-color:var(--accent);background:var(--accent)22;color:var(--accent)}
  .modal-submit{
    width:100%;margin-top:16px;padding:14px;border-radius:12px;
    background:linear-gradient(135deg,var(--accent),var(--accent2));
    border:none;color:#fff;font-family:'Vazirmatn',sans-serif;
    font-size:15px;font-weight:700;cursor:pointer;
  }
  .modal-cancel{
    width:100%;margin-top:8px;padding:12px;border-radius:12px;
    background:transparent;border:1px solid var(--border);
    color:var(--muted);font-family:'Vazirmatn',sans-serif;font-size:14px;cursor:pointer;
  }

  /* ── Toast ── */
  .toast{
    position:fixed;bottom:80px;left:50%;transform:translateX(-50%) translateY(60px);
    background:var(--card);border:1px solid var(--success);color:var(--success);
    padding:9px 20px;border-radius:24px;font-size:13px;font-weight:600;
    transition:transform .3s ease;z-index:999;white-space:nowrap;pointer-events:none;
  }
  .toast.err{border-color:var(--accent2);color:var(--accent2)}
  .toast.show{transform:translateX(-50%) translateY(0)}

  /* ── Overlay backdrop for panel ── */
  .panel-bg{
    display:none;position:fixed;inset:0;z-index:290;background:#0008;
  }
  .panel-bg.show{display:block}

  option{background:var(--card)}
</style>
</head>
<body>

<!-- Header -->
<header>
  <div class="logo">
    <div class="logo-icon">📡</div>
    <div class="logo-text">IR <span>Remote</span></div>
  </div>
  <div class="hdr-right">
    <div class="dot"></div>
  </div>
</header>

<!-- Tab bar -->
<div class="tab-bar">
  <div class="tab active" data-cat="tv" onclick="switchTab('tv',this)">
    <span class="tab-icon">📺</span>تلویزیون
  </div>
  <div class="tab" data-cat="ac" onclick="switchTab('ac',this)">
    <span class="tab-icon">❄️</span>کولر گازی
  </div>
  <div class="tab" data-cat="other" onclick="switchTab('other',this)">
    <span class="tab-icon">📽️</span>سایر
  </div>
</div>

<div class="container">
  <div class="brand-bar" id="brandBar"></div>
  <div class="device-grid" id="deviceGrid"></div>
</div>

<!-- Remote panel -->
<div class="panel-bg" id="panelBg" onclick="closePanel()"></div>
<div class="remote-panel" id="remotePanel">
  <div class="rp-handle"></div>
  <div class="rp-header">
    <span class="rp-title" id="rpTitle">—</span>
    <div class="rp-actions">
      <button class="rp-learn-btn" id="rpLearnBtn" onclick="toggleLearnMode()">📡 یادگیری</button>
      <button class="rp-close" onclick="closePanel()">✕ بستن</button>
    </div>
  </div>
  <div class="btns-grid" id="rpBtns"></div>
</div>

<!-- Learn overlay -->
<div class="learn-overlay" id="learnOverlay">
  <div class="learn-box">
    <div class="lb-icon">📡</div>
    <h3>منتظر سیگنال IR</h3>
    <p>ریموت اصلی را جلوی گیرنده بگیر و دکمه <strong id="learnBtnName">—</strong> را بزن</p>
    <div class="countdown" id="learnCountdown">15</div>
    <button class="lb-cancel" onclick="cancelLearn()">لغو</button>
  </div>
</div>

<!-- Add device modal -->
<div class="modal-overlay" id="addModal">
  <div class="modal-box">
    <div class="modal-handle"></div>
    <div class="modal-title">➕ اضافه کردن دستگاه جدید</div>

    <div class="form-row">
      <label>نام دستگاه</label>
      <input type="text" id="newDevName" placeholder="مثلاً: پروژکتور اتاق">
    </div>
    <div class="form-row">
      <label>برند</label>
      <input type="text" id="newDevBrand" placeholder="مثلاً: Epson">
    </div>
    <div class="form-row">
      <label>دسته‌بندی</label>
      <select id="newDevCat">
        <option value="tv">📺 تلویزیون</option>
        <option value="ac">❄️ کولر گازی</option>
        <option value="other">📽️ سایر</option>
      </select>
    </div>

    <div class="form-row">
      <label>دکمه‌هایی که می‌خوای یاد بگیری (انتخاب کن)</label>
      <div class="btns-add-row" id="btnPickerGrid"></div>
    </div>

    <button class="modal-submit" onclick="startAddDevice()">✔ شروع یادگیری دکمه‌ها</button>
    <button class="modal-cancel" onclick="closeAddModal()">انصراف</button>
  </div>
</div>

<!-- Toast -->
<div class="toast" id="toast"></div>

<script>
// ── State ─────────────────────────────────────────────────
let DEVICES = [];
let currentCat = 'tv';
let currentBrand = 'all';
let openDeviceId = null;
let learnModeOn = false;
let learnPolling = null;
let learnCdInterval = null;
let pendingLearnBtn = null;
let learnQueue = [];    // برای add device
let learnQueueIdx = 0;
let addDeviceData = {}; // داده دستگاه جدید
const selectedAddBtns = new Set();

// ── دکمه‌های پیش‌فرض برای add device ────────────────────
const PRESET_BTNS = [
  {name:"Power",icon:"⏻"}, {name:"Vol+",icon:"🔊"}, {name:"Vol-",icon:"🔉"},
  {name:"Mute",icon:"🔇"}, {name:"CH+",icon:"⬆"},  {name:"CH-",icon:"⬇"},
  {name:"OK",icon:"✔"},   {name:"Up",icon:"▲"},    {name:"Down",icon:"▼"},
  {name:"Left",icon:"◀"}, {name:"Right",icon:"▶"}, {name:"Back",icon:"↩"},
  {name:"Home",icon:"🏠"}, {name:"Menu",icon:"☰"}, {name:"Source",icon:"⎘"},
  {name:"Cool",icon:"❄"},  {name:"Heat",icon:"🔥"}, {name:"Fan",icon:"💨"},
  {name:"Temp+",icon:"🌡"},{name:"Temp-",icon:"🌡"},
  {name:"Play",icon:"▶"},  {name:"Pause",icon:"⏸"},{name:"Stop",icon:"⏹"},
];

// ── آیکون دسته ──────────────────────────────────────────
function catIcon(cat, label) {
  if (cat === 'tv') return '📺';
  if (cat === 'ac') return '❄️';
  // other: بر اساس catLabel
  if (label === 'پروژکتور') return '📽️';
  if (label === 'ست‌تاپ باکس') return '📡';
  if (label === 'DVD Player') return '💿';
  if (label === 'A/V Receiver') return '🔊';
  if (label === 'پنکه') return '💨';
  return '📱';
}

// ── بارگذاری دستگاه‌ها ───────────────────────────────────
async function loadDevices() {
  try {
    const r = await fetch('/devices');
    DEVICES = await r.json();
    renderAll();
  } catch(e) {
    DEVICES = [];
    renderAll();
  }
}

// ── رندر کلی ────────────────────────────────────────────
function renderAll() {
  renderBrandBar();
  renderGrid();
}

function switchTab(cat, el) {
  currentCat = cat;
  currentBrand = 'all';
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  el.classList.add('active');
  renderAll();
}

// ── برند بار ────────────────────────────────────────────
function renderBrandBar() {
  const filtered = DEVICES.filter(d => d.cat === currentCat);
  const brands = ['all', ...new Set(filtered.map(d => d.brand))];
  const bar = document.getElementById('brandBar');
  bar.innerHTML = '';
  brands.forEach(b => {
    const el = document.createElement('button');
    el.className = 'brand-pill' + (b === currentBrand ? ' active' : '');
    el.textContent = b === 'all' ? 'همه' : b;
    el.onclick = () => { currentBrand = b; renderBrandBar(); renderGrid(); };
    bar.appendChild(el);
  });
}

// ── گرید دستگاه‌ها ──────────────────────────────────────
function renderGrid() {
  const grid = document.getElementById('deviceGrid');
  grid.innerHTML = '';
  const list = DEVICES.filter(d =>
    d.cat === currentCat &&
    (currentBrand === 'all' || d.brand === currentBrand)
  );

  list.forEach(dev => {
    const card = document.createElement('div');
    card.className = 'dev-card';
    card.innerHTML = `
      <div class="dev-card-head">
        <div class="dev-cat-icon">${catIcon(dev.cat, dev.catLabel)}</div>
        <div class="dev-name">${dev.name}</div>
        <div class="dev-brand">${dev.brand}</div>
      </div>`;
    card.onclick = () => openPanel(dev.id);
    grid.appendChild(card);
  });

  // کارت اضافه کردن
  const add = document.createElement('div');
  add.className = 'add-card';
  add.innerHTML = `<div class="add-icon">➕</div><div class="add-label">دستگاه جدید</div>`;
  add.onclick = () => openAddModal();
  grid.appendChild(add);
}

// ── باز کردن ریموت ──────────────────────────────────────
function openPanel(devId) {
  const dev = DEVICES.find(d => d.id === devId);
  if (!dev) return;
  openDeviceId = devId;
  learnModeOn = false;
  document.getElementById('rpLearnBtn').classList.remove('on');
  document.getElementById('rpTitle').textContent = dev.name + ' — ' + dev.brand;
  renderPanelBtns(dev);
  document.getElementById('remotePanel').classList.add('open');
  document.getElementById('panelBg').classList.add('show');
}

function closePanel() {
  document.getElementById('remotePanel').classList.remove('open');
  document.getElementById('panelBg').classList.remove('show');
  openDeviceId = null;
  learnModeOn = false;
  document.getElementById('rpLearnBtn').classList.remove('on');
}

function renderPanelBtns(dev) {
  const grid = document.getElementById('rpBtns');
  grid.innerHTML = '';
  dev.btns.forEach(btn => {
    const b = document.createElement('button');
    b.className = 'ir-btn' + (btn.learned ? ' learned' : '');
    b.setAttribute('data-btn', btn.name);
    b.innerHTML = `<span class="bi">${btn.icon}</span><span>${btn.name}</span>`;
    b.onclick = () => {
      if (learnModeOn) {
        startLearnBtn(dev.id, btn.name, b);
      } else {
        sendIR(dev.id, btn.name, b);
      }
    };
    grid.appendChild(b);
  });
}

// ── حالت یادگیری ────────────────────────────────────────
function toggleLearnMode() {
  learnModeOn = !learnModeOn;
  const btn = document.getElementById('rpLearnBtn');
  btn.classList.toggle('on', learnModeOn);
  btn.textContent = learnModeOn ? '✕ لغو یادگیری' : '📡 یادگیری';
  if (learnModeOn) {
    showToast('روی دکمه موردنظر کلیک کن', false);
  } else {
    showToast('حالت یادگیری خاموش شد', false);
  }
}

async function startLearnBtn(devId, btnName, btnEl) {
  pendingLearnBtn = {devId, btnName, el: btnEl};
  btnEl.classList.add('learn-target');
  document.getElementById('learnBtnName').textContent = btnName;
  document.getElementById('learnCountdown').textContent = '15';
  document.getElementById('learnOverlay').classList.add('show');

  await fetch(`/learn/start?device=${encodeURIComponent(devId)}&btn=${encodeURIComponent(btnName)}`);
  startLearnPolling(15);
}

function startLearnPolling(sec) {
  let remaining = sec;
  clearInterval(learnCdInterval);
  clearInterval(learnPolling);

  learnCdInterval = setInterval(() => {
    remaining--;
    document.getElementById('learnCountdown').textContent = remaining;
    if (remaining <= 0) clearInterval(learnCdInterval);
  }, 1000);

  learnPolling = setInterval(async () => {
    try {
      const r = await fetch('/learn/status');
      const j = await r.json();
      if (j.status === 'done') {
        clearInterval(learnPolling);
        clearInterval(learnCdInterval);
        onLearnSuccess();
      } else if (j.status === 'timeout' || j.status === 'idle') {
        clearInterval(learnPolling);
        clearInterval(learnCdInterval);
        onLearnTimeout();
      }
    } catch(e) {}
  }, 500);
}

function onLearnSuccess() {
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn.el.classList.add('learned');
    showToast('✅ کد ذخیره شد!', false);
    pendingLearnBtn = null;
  }
  // اگر در حال یادگیری دستگاه جدید هستیم
  if (learnQueue.length > 0) {
    learnQueueIdx++;
    setTimeout(() => processLearnQueue(), 800);
  } else {
    learnModeOn = false;
    document.getElementById('rpLearnBtn').classList.remove('on');
    document.getElementById('rpLearnBtn').textContent = '📡 یادگیری';
  }
  loadDevices(); // refresh
}

function onLearnTimeout() {
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn = null;
  }
  if (learnQueue.length > 0) {
    // پرسش ادامه یا رد
    if (confirm('سیگنالی دریافت نشد. این دکمه رو رد کنم و بریم سراغ بعدی؟')) {
      learnQueueIdx++;
      setTimeout(() => processLearnQueue(), 400);
    } else {
      learnQueue = [];
      showToast('یادگیری متوقف شد', true);
    }
  } else {
    showToast('⏰ سیگنالی دریافت نشد', true);
  }
}

async function cancelLearn() {
  clearInterval(learnPolling);
  clearInterval(learnCdInterval);
  await fetch('/learn/cancel');
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn = null;
  }
  learnQueue = [];
  showToast('یادگیری لغو شد', true);
}

// ── ارسال IR ─────────────────────────────────────────────
async function sendIR(devId, btnName, el) {
  el.classList.add('sent');
  try {
    const r = await fetch(`/ir?device=${encodeURIComponent(devId)}&btn=${encodeURIComponent(btnName)}`);
    const j = await r.json();
    showToast('📡 ' + btnName, false);
  } catch(e) {
    showToast('⚠️ خطا', true);
  }
  setTimeout(() => el.classList.remove('sent'), 800);
}

// ── Add device modal ─────────────────────────────────────
function openAddModal() {
  selectedAddBtns.clear();
  renderBtnPicker();
  document.getElementById('newDevName').value = '';
  document.getElementById('newDevBrand').value = '';
  document.getElementById('addModal').classList.add('show');
}

function closeAddModal() {
  document.getElementById('addModal').classList.remove('show');
}

function renderBtnPicker() {
  const grid = document.getElementById('btnPickerGrid');
  grid.innerHTML = '';
  PRESET_BTNS.forEach(b => {
    const el = document.createElement('div');
    el.className = 'btn-add-btn' + (selectedAddBtns.has(b.name) ? ' sel' : '');
    el.innerHTML = b.icon + ' ' + b.name;
    el.onclick = () => {
      selectedAddBtns.has(b.name) ? selectedAddBtns.delete(b.name) : selectedAddBtns.add(b.name);
      renderBtnPicker();
    };
    grid.appendChild(el);
  });
}

function startAddDevice() {
  const name  = document.getElementById('newDevName').value.trim();
  const brand = document.getElementById('newDevBrand').value.trim();
  const cat   = document.getElementById('newDevCat').value;
  if (!name || !brand) { showToast('نام و برند رو پر کن', true); return; }
  if (selectedAddBtns.size === 0) { showToast('حداقل یک دکمه انتخاب کن', true); return; }

  const id = 'custom_' + Date.now();
  addDeviceData = { id, name, brand, cat };

  // ساخت صف یادگیری
  learnQueue = Array.from(selectedAddBtns).map(bName => {
    const preset = PRESET_BTNS.find(p => p.name === bName);
    return { devId: id, btnName: bName, icon: preset ? preset.icon : '•' };
  });
  learnQueueIdx = 0;

  closeAddModal();

  // اضافه کردن دستگاه به لیست موقت
  const catMap = {tv:'تلویزیون', ac:'کولر گازی', other:'سایر'};
  DEVICES.push({
    id, name, brand, cat, catLabel: catMap[cat],
    btns: learnQueue.map(l => ({name:l.btnName, icon:l.icon, learned:false}))
  });
  renderAll();
  openPanel(id);

  // شروع یادگیری صف
  setTimeout(() => processLearnQueue(), 600);
}

async function processLearnQueue() {
  if (learnQueueIdx >= learnQueue.length) {
    showToast('🎉 همه دکمه‌ها یاد گرفته شد!', false);
    learnQueue = [];
    return;
  }
  const item = learnQueue[learnQueueIdx];
  // پیدا کردن المان دکمه در پنل
  const btnEls = document.querySelectorAll('#rpBtns .ir-btn');
  let btnEl = null;
  btnEls.forEach(el => { if(el.getAttribute('data-btn') === item.btnName) btnEl = el; });

  pendingLearnBtn = {devId: item.devId, btnName: item.btnName, el: btnEl};
  if (btnEl) btnEl.classList.add('learn-target');
  document.getElementById('learnBtnName').textContent = item.btnName;
  document.getElementById('learnCountdown').textContent = '15';
  document.getElementById('learnOverlay').classList.add('show');

  await fetch(`/learn/start?device=${encodeURIComponent(item.devId)}&btn=${encodeURIComponent(item.btnName)}`);
  startLearnPolling(15);
}

// ── Toast ────────────────────────────────────────────────
function showToast(msg, err) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast' + (err ? ' err' : '') + ' show';
  setTimeout(() => t.classList.remove('show'), 2200);
}

// ── Init ─────────────────────────────────────────────────
loadDevices();
</script>
</body>
</html>
)rawliteral");
}


// ════════════════════════════════════════════════════════════
//  حلقه یادگیری (در loop اجرا میشه)
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
      Serial.printf("[LEARN] OK: %s/%s = 0x%llX (%d bits, proto=%d)\n",
        learnDevice.c_str(), learnBtn.c_str(), code, bits, proto);

      learnMode = false;
      irrecv.disableIRIn();

      // اعلام به polling endpoint
      // (client از /learn/status میخونه — status رو done می‌ذاریم)
      // از یه flag استفاده می‌کنیم
      extern bool learnDone;
      learnDone = true;
    }
    irrecv.resume();
  }
}

bool learnDone = false;

void handleLearnStatusFixed() {
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


// ════════════════════════════════════════════════════════════
//  Setup
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== IR Remote v2.0 ===");

  irsend.begin();
  Serial.printf("IR Send  → GPIO %d\n", IR_SEND_PIN);
  Serial.printf("IR Recv  → GPIO %d\n", IR_RECV_PIN);

  // Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("AP: %s  |  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  // Routes
  server.on("/",              HTTP_GET,  handleRoot);
  server.on("/devices",       HTTP_GET,  handleDeviceList);
  server.on("/ir",            HTTP_GET,  handleIRSend);
  server.on("/learn/start",   HTTP_GET,  handleLearnStart);
  server.on("/learn/status",  HTTP_GET,  handleLearnStatusFixed);
  server.on("/learn/cancel",  HTTP_GET,  handleLearnCancel);
  server.on("/learn/delete",  HTTP_GET,  handleLearnDelete);
  server.onNotFound([]{ server.send(404,"text/plain","Not Found"); });

  server.begin();
  Serial.println("Server started.");
  Serial.println("وای‌فای: IR-Remote  |  پسورد: 12345678");
  Serial.println("آدرس: http://192.168.4.1");
}

void loop() {
  server.handleClient();
  loopLearn();
}
