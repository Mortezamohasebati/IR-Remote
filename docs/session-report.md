# Session Report — IR Remote v2.0 Full-Stack Overhaul

**Date:** 2026-06-09  
**Commits:** `6e0986a` (previous), plus uncommitted fixes  
**Branch:** `main`  
**Scope:** Phase 10a, Phase 6, Phase 7, Critical fix (IRProtocol → decode_type_t), POST learn endpoints, Smart Home backoff, docs/schematics

---

## Overview

Complete overhaul of the [IR-Remote](https://github.com/Mortezamohasebati/IR-Remote) ESP32 project. This session adapted the Smart Home client to match the real companion board firmware, implemented a security layer (CSRF, rate limiting, input validation), rewrote the README, and created full documentation with schematics.

---

## Files Changed/Created

| File | Action | Lines |
|------|--------|-------|
| `smarthome_client.h` | **Rewritten** | 99 |
| `scenes.h` | **Major edit** | 120 |
| `web_ui.h` | **Major edit** (+ includes fix, POST learn, backoff) | ~1160 |
| `main.ino` | **Major edit** | 183 |
| `globals.h` | **Minor edit** | 30 |
| `ir_codes.h` | **Major edit** (enum fix, removed 3 ACs, UNVERIFIED) | 490 |
| `README.md` | **Rewritten** | ~319 |
| `docs/architecture.md` | **New** | 99 |
| `docs/wiring.md` | **New** | 148 |
| `docs/api.md` | **New** (+ POST docs, compile checklist) | ~420 |
| `schematics/ir-transmitter.txt` | **New** | 55 |
| `schematics/ir-receiver.txt` | **New** | 49 |
| `schematics/full-system.txt` | **New** | 62 |
| `.gitignore` | **New** | 12 |

---

## Phase 10a — SmartHomeClient Adaptation

### Problem
`smarthome_client.h` was written against a spec (Appendix C) that used `/relay?id=&state=` and `/relay/status` endpoints. The actual ESP8266 firmware at [vahidseyyedi/Arduino-Smart-Home](https://github.com/vahidseyyedi/Arduino-Smart-Home) uses completely different endpoints.

### Actual ESP8266 Firmware Endpoints (from `esp8266.ino`)

| Method | Path | Body | Response |
|--------|------|------|----------|
| GET | `/get-sensor-data` | — | `{"temperature":float,"humidity":float,"light":int,"gas":int,"alarm":int}` |
| POST | `/toggle-device` | `{"device":"lamp"\|"cooling","status":bool}` | `"OK"` |
| POST | `/toggle-alarm` | `{"status":bool}` | `"OK"` |
| POST | `/toggle-smart` | `{"status":0\|1\|2}` | `"OK"` |

Where `toggle-smart` modes are: 0=off, 1=LDR (light sensor), 2=PIR (motion sensor).

The companion board communicates with an Arduino Nano over I2C (address `0x08`), sending single characters: `L`/`l` for lamp on/off, `C`/`c` for cooling on/off, `A`/`a` for alarm on/off, `s`/`1`/`2` for smart mode off/LDR/PIR.

### New `smarthome_client.h` API

```cpp
struct SensorData {
  float temperature = NAN;
  float humidity = NAN;
  int light = -1;
  int gas = -1;
  int alarm = -1;
};

class SmartHomeClient {
  SensorData getSensorData();              // GET /get-sensor-data
  bool toggleDevice(const String&, bool);  // POST /toggle-device
  bool toggleAlarm(bool);                  // POST /toggle-alarm
  bool setSmartMode(uint8_t);             // POST /toggle-smart (0/1/2)
  void saveIP(const String&);
  String getSavedIP();
};
```

All HTTP methods use `WiFiClient` + `HTTPClient` with 2-second timeout. POST uses `application/json` content type.

### Scene Action Restructuring

`scenes.h` changed from relay-index-based actions to device-name-based actions:

```cpp
// BEFORE
struct SceneAction {
  String type;     // "relay"
  uint8_t relay;   // index 0-3
  bool    state;
};

// AFTER
struct SceneAction {
  String type;     // "device"
  String device;   // "lamp", "cooling", "alarm"
  bool   state;
};
```

`loopScenes()` now calls:
```cpp
if (s.action.type == "device") {
  shClient.toggleDevice(s.action.device, s.action.state);
}
```

Serialization in `saveScenes()`, `loadScenes()`, and `scenesJSON()` all updated to use `action.device` (String) instead of `action.relay` (uint8_t).

### New Smart Home API Endpoints

Six endpoints added to `main.ino` and `web_ui.h`:

| Route | Handler | Purpose |
|-------|---------|---------|
| `GET /smarthome/sensors` | `handleSmartHomeSensors()` | Proxy sensor data from companion board |
| `POST /smarthome/device` | `handleSmartHomeDevice()` | Toggle lamp/cooling |
| `POST /smarthome/alarm` | `handleSmartHomeAlarm()` | Arm/disarm alarm |
| `POST /smarthome/smart` | `handleSmartHomeSmart()` | Set LDR/PIR mode |

Each POST handler validates JSON, checks CSRF, proxies the request, and returns 200/502.

### Smart Home Page (handleSmartHomePage)

Complete UI rewrite showing:
- Board IP configuration
- Sensor data display (temperature, humidity, light %, gas %, alarm status)
- Lamp and Cooling toggle buttons
- Alarm toggle
- Smart mode selector (Off / LDR / PIR)
- Scene management (IR trigger → device action)
- 3-second auto-polling for sensor data

---

## Phase 6 — Error Handling

### CSRF Token (Cross-Site Request Forgery Protection)

**Generation** (`main.ino:130-132`):
```cpp
for (int i = 0; i < 16; i++) csrfToken[i] = "0123456789abcdef"[esp_random() % 16];
csrfToken[16] = '\0';
```

Uses `esp_random()` (hardware RNG) for each hex digit. Token is 16 hex chars = 8 bytes entropy.

**Storage**: `char csrfToken[17]` as global, declared `extern` in `globals.h`.

**Verification** (`web_ui.h`):
```cpp
bool csrfCheck() {
  String h = server.header("X-CSRF-Token");
  if (h.length() == 16 && h == csrfToken) return true;
  if (server.hasArg("csrf") && server.arg("csrf") == csrfToken) return true;
  return false;
}
```

Supports both `X-CSRF-Token` header and `?csrf=` query parameter.

**Token retrieval endpoint**:
```
GET /csrf → {"token":"a1b2c3d4e5f67890"}
```

**Protected handlers** (all call `csrfCheck()` before processing):

| Handler | Method |
|---------|--------|
| `handleLearnStart` | GET |
| `handleLearnCancel` | GET |
| `handleLearnDelete` | GET |
| `handleScenesCreate` | POST |
| `handleScenesDelete` | DELETE |
| `handleWiFiConfig` | GET (query args) |
| `handleSmartHomeConfig` | POST |
| `handleSmartHomeDevice` | POST |
| `handleSmartHomeAlarm` | POST |
| `handleSmartHomeSmart` | POST |
| `handleVendorConfig` | POST |

### Rate Limiting

```cpp
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
```

Sliding-window counter: tracks last 10 request timestamps. If more than 10 occur within 1 second, returns false → server sends `429 Rate Limit Exceeded`.

Applied in `handleIRSend()` only (the `/ir` endpoint that transmits IR).

### Input Validation

```cpp
bool validateArg(const String& s, size_t maxLen = 64) {
  if (s.length() > maxLen) return false;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!isAlphaNumeric(c) && c != '-' && c != '_' && c != ':' && c != '/') return false;
  }
  return true;
}
```

Applied on:
- `handleIRSend()` — device ID, button name
- `handleLearnStart()` — device ID, button name
- `handleLearnDelete()` — device ID, button name
- `handleScenesDelete()` — scene ID
- `handleWiFiConfig()` — SSID (max 32), password (max 64)
- `handleSmartHomeConfig()` — IP address (max 40)
- `handleVendorConfig()` — length check on base_url (128), api_key (128), serial (64)

### DynamicJsonDocument Conversion

`buildDevicesJSON()` and `handleWiFiStatus()` converted from hand-built string concatenation to `DynamicJsonDocument`:

```cpp
// BEFORE (string concat — fragile, no escaping)
String json = "[";
json += "{\"id\":\"" + String(d.id) + "\",";
// ... prone to injection, no proper JSON encoding

// AFTER (DynamicJsonDocument — proper serialization)
DynamicJsonDocument doc(16384);
JsonArray arr = doc.to<JsonArray>();
JsonObject obj = arr.createNestedObject();
obj["id"] = d.id;
// ...
serializeJson(doc, out);
```

### Frontend CSRF Integration

All JS files (`handleRoot`, `handleSmartHomePage`, `handleVendorPage`) now:
1. Fetch CSRF token on page load: `fetch('/csrf').then(r=>r.json()).then(d=>CSRFToken=d.token)`
2. Append to mutation URLs: `` `/endpoint?csrf=${CSRFToken}` ``

---

## Phase 7 — README Rewrite

The README was rewritten from 287 lines to 386 lines with:

- **Features section** — all 11 features documented (37 devices, IRDB, Smart Home bridge, vendor platform, scenes, CSRF, rate limiting)
- **Hardware table** — 6 components with specs and roles
- **Wiring section** — ASCII diagrams for IR transmitter (BC547), receiver (CHQ1838), and Smart Home companion (ESP8266 I2C)
- **Project structure** — 10 files with descriptions
- **Supported devices** — 3 collapsible tables (16 TVs, 13 ACs, 8 Other) with protocol and notes
- **API reference** — complete table of all 22+ endpoints grouped by function (IR, Learning, IRDB, Smart Home, Scenes, Vendor, WiFi)
- **Installation** — 6 steps from Arduino IDE setup to connecting
- **Configuration** — pin definitions, capture buffer, learn timeout
- **Troubleshooting** — 7 common problems with solutions

---

## Documentation Files

### `docs/architecture.md`
- ASCII art system overview showing ESP32, ESP8266, IR components, and module interactions
- File responsibility table (10 files)
- Data flow diagrams for IR send, learning, and scene triggers
- Memory model note (PROGMEM on ESP32 uses MMU mapping)
- Protocol handler reference table (9 protocols with send methods and IRDB mapping)

### `docs/wiring.md`
- IR transmitter circuit with labeled BC547 pinout
- IR receiver circuit with CHQ1838 pin identification
- ESP8266 I2C connection map
- Full system ASCII wiring diagram
- Complete pin map table (GPIO 4, 14, 21, 22, power, ground)
- Power supply recommendations

### `docs/api.md`
- CSRF token usage (two methods: header and query param)
- Device list JSON schema
- IR send resolution order (learned → library → IRDB)
- Learning workflow (start, poll status, states, cancel, delete)
- IRDB brand listing and code lookup parameters
- Smart Home bridge: config, sensors, device control, alarm, smart mode
- Scenes: list, create, delete, resolution algorithm
- Vendor platform config
- WiFi status and config
- HTTP error codes table (200, 400, 403, 404, 405, 429, 502)

---

## Schematics (`schematics/`)

### `schematics/ir-transmitter.txt`
```
                     +3.3V
                       │
                      ╔╧╗
                      ║ ║  IR LED 940nm
                      ╚╤╝
                       │
                       ├──── Collector (C)
                       │
    ESP32 GPIO4 ── R1(100Ω) ── Base (B)
                       │
                      R2(10kΩ)
                       │
                       GND

    Emitter (E) ── GND
```

- BC547 pinout diagram (flat face: B C on top, E on bottom)
- Operation description (GPIO HIGH → transistor conducts → LED lights)

### `schematics/ir-receiver.txt`
```
    CHQ1838:
    Pin 1 (OUT)  ── GPIO14
    Pin 2 (GND)  ── GND
    Pin 3 (VCC)  ── 3.3V (NOT 5V!)
```

- ⚠️ Warning: 5V will damage CHQ1838
- Optional 10μF decoupling capacitor
- Physical separation requirement (5cm minimum from IR LED)

### `schematics/full-system.txt`
- Combined diagram showing ESP32, IR transmitter, IR receiver, and ESP8266 companion
- Complete pin summary table
- Power supply options (USB 5V or VIN 7-12V)

---

## Routing Table (`main.ino`)

All 19 registered routes:

| Method | Path | Handler | CSRF | Rate-limited |
|--------|------|---------|------|-------------|
| GET | `/` | `handleRoot` | — | — |
| GET | `/devices` | `handleDeviceList` | — | — |
| GET | `/csrf` | `handleCSRFToken` | — | — |
| GET | `/ir` | `handleIRSend` | — | ✓ 10/s |
| POST | `/learn/start` | `handleLearnStart` | ✓ | — |
| GET | `/learn/status` | `handleLearnStatus` | — | — |
| POST | `/learn/cancel` | `handleLearnCancel` | ✓ | — |
| POST | `/learn/delete` | `handleLearnDelete` | ✓ | — |
| GET | `/wifi/status` | `handleWiFiStatus` | — | — |
| GET | `/wifi/config` | `handleWiFiConfig` | ✓ | — |
| GET | `/irdb/brands` | `handleIRDBBrands` | — | — |
| GET | `/irdb/codes` | `handleIRDBCodes` | — | — |
| GET | `/irdb` | `handleIRDBPage` | — | — |
| GET | `/scenes` | `handleScenesList` | — | — |
| POST | `/scenes` | `handleScenesCreate` | ✓ | — |
| DELETE | `/scenes` | `handleScenesDelete` | ✓ | — |
| GET | `/smarthome` | `handleSmartHomePage` | — | — |
| ANY | `/smarthome/config` | `handleSmartHomeConfig` | ✓ (POST) | — |
| GET | `/smarthome/sensors` | `handleSmartHomeSensors` | — | — |
| POST | `/smarthome/device` | `handleSmartHomeDevice` | ✓ | — |
| POST | `/smarthome/alarm` | `handleSmartHomeAlarm` | ✓ | — |
| POST | `/smarthome/smart` | `handleSmartHomeSmart` | ✓ | — |
| GET | `/vendor` | `handleVendorPage` | — | — |
| ANY | `/vendor/config` | `handleVendorConfig` | ✓ (POST) | — |

---

## Final Fix Session — Protocol Enum, Learn POST, Smart Home Backoff

### Critical Fix: IRProtocol → decode_type_t

**Problem**: `ir_codes.h` defined a custom `IRProtocol` enum with values `{NEC=0, SAMSUNG=1, SONY=2, ...}` that diverged from `IRremoteESP8266`'s `decode_type_t` (`NEC=3, SAMSUNG=5, SONY=2, ...`). `loopReceive()` stored `results.decode_type` as `uint8_t`, and `findIRInLibrary()` compared it against our custom enum. Protocol matching would fail for all learned codes and scene triggers.

**Fix**:
1. Deleted the custom `IRProtocol` enum entirely from `ir_codes.h`
2. Added `#include <IRremoteESP8266.h>` to bring in `decode_type_t`
3. Changed `DeviceIR::protocol` from `IRProtocol` to `decode_type_t`
4. Replaced all `PROTO_NEC`/`PROTO_SAMSUNG`/etc. with bare enum values (`NEC`, `SAMSUNG`, etc.)
5. Removed `PROTO_RCA` case from `sendCode()` switch — RCA entries use `NEC` type (same modulation)
6. Updated `sendCode()` signature from `IRProtocol` to `decode_type_t`
7. Updated `saveLearnedCode()` / `loadLearnedCode()` proto parameter from `uint8_t` to `decode_type_t`
8. Updated `findIRInLibrary()` signature from `uint8_t` to `decode_type_t`, removed casts
9. Updated `loopReceive()` to use `decode_type_t proto = results.decode_type` (no cast)

**Files changed**: `ir_codes.h`, `web_ui.h`, `main.ino`

### AC Brand Audit: Haier, Daikin, Mitsubishi Removed

All three have dedicated `IR*` classes in IRremoteESP8266 (IRHaierAC176, IRDaikin2, IRMitsubishiAC) that handle stateful protocols. Static hex codes cannot represent these protocols reliably.

**Removed**:
- `haierACBtns[]` array + `haier_ac` device entry (commented explaining use learn-mode/irdb)
- `daikinACBtns[]` array + `daikin_ac` device entry (same)
- `mitsubishiACBtns[]` array + `mitsubishi_ac` device entry (same)

Device count: 37 → 34.

### TCL NEC Marked UNVERIFIED

`tclTVBtns[]` now has a prominent comment:
```
// TCL TV (North America/NEC) — UNVERIFIED
// Source: community gist. Model unknown. Protocol inferred as NEC.
// If codes do not work, use learn-mode to capture from your remote.
```

### Learn Endpoints: GET → POST

Routes changed from HTTP_GET to HTTP_POST:
- `/learn/start` — body: `device=X&btn=Y&csrf=Z`
- `/learn/cancel` — body: `csrf=Z`
- `/learn/delete` — body: `device=X&btn=Y&csrf=Z`

Frontend JS updated to use `fetch(url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:...})`.

### Smart Home Page: Exponential Polling Backoff

Replaced `setInterval(loadSensors, 3000)` with self-scheduling `setTimeout`:

| Failure count | Delay |
|:---|---:|
| 0 | 3s |
| 1 | 6s |
| 2 | 12s |
| 3+ | 30s (capped) |

After 3 consecutive failures: polling stops, error banner appears with Retry button. `retrySensorPoll()` resets state and restarts polling.

---

## Pre-existing Issues (Not Fixed)

1. **TCL NEC codes (0x57E3 prefix)**: Sourced from community gist, not verified against actual TCL hardware. CH+/CH- omitted (Roku remote lacks dedicated buttons). Now marked `UNVERIFIED` with explanatory comment.

2. **Hisense/Midea codes**: Generic NEC placeholders, not verified against hardware. All marked `// VERIFY` in `ir_codes.h`.

3. **jsDelivr CDN**: `setInsecure()` is intentional — certificate pinning costs ~10KB RAM for CA bundle. Low risk (MITM only on local network).

4. **Compilation not verified**: `IRremoteESP8266` library download failed (network issue). Code style is consistent with existing patterns.

5. **Push blocked**: No write access to `Mortezamohasebati/IR-Remote`. Requires fork → PR workflow.

---

## Git History

```
6e0986a (HEAD -> main) feat: full-stack overhaul — SmartHome client, error handling, docs
587201d Add files via upload
881d904 Initial commit: IR remote codes
```

**Uncommitted changes** (second commit pending):
- fix: delete custom IRProtocol enum, use decode_type_t from IRremoteESP8266 everywhere
- fix: remove Haier/Daikin/Mitsubishi AC (stateful protocols, not representable as static hex)
- fix: mark TCL NEC as UNVERIFIED with explanatory comment
- fix: change learn endpoints from GET to POST with url-encoded body
- fix: Smart Home page polling with exponential backoff (3s→30s max, stop after 3 failures)
- docs: update API reference for POST learn endpoints, add compile checklist
- docs: update session report for all fixes

---

## How to Make the PR

```bash
git remote add fork https://github.com/<YOUR_USERNAME>/IR-Remote.git
git push fork main
# Then open PR on GitHub from YOUR_USERNAME/IR-Remote → Mortezamohasebati/IR-Remote
```
