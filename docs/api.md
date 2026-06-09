# API Reference

Base URL: `http://192.168.4.1`

All responses are JSON. All state-changing endpoints require CSRF authentication. Read-only endpoints (GET with no side effects) do not.

---

## CSRF Token

```
GET /csrf
```

Response:
```json
{"token":"a1b2c3d4e5f67890"}
```

Include the token in all POST/DELETE requests. Two methods supported:

**Query parameter** (simplest with fetch):
```
POST /endpoint?csrf=a1b2c3d4e5f67890
```

**HTTP header**:
```
X-CSRF-Token: a1b2c3d4e5f67890
```

---

## Device List

```
GET /devices
```

Returns all static devices (34 entries) with their buttons and learned status.

```json
[
  {
    "id": "samsung_tv",
    "name": "Samsung TV",
    "cat": "tv",
    "catLabel": "TV",
    "brand": "Samsung",
    "btns": [
      {"name": "Power", "icon": "⏻", "learned": false},
      {"name": "Vol+", "icon": "🔊", "learned": true}
    ]
  }
]
```

`learned: true` means a custom code has been saved for that button via the learning mode.

---

## Send IR

```
GET /ir?device=<id>&btn=<name>
```

Send an IR code for a device button. Resolution order:

1. **Learned code** — if the user has taught a custom code via learn mode
2. **Static library** — if the device/button is in `ir_codes.h`
3. **IRDB fallback** — if WiFi is connected, attempt to fetch from CDN

```json
{"ok":true,"src":"learned"}
```

`src` is one of: `learned`, `library`, `irdb`, `not_found`.

Errors:
- `400` — missing args or invalid args
- `429` — rate limit exceeded (>10 requests in 1 second)
- `404` — code not found in any source

---

## Learning

### Start learning
```
POST /learn/start
  Content-Type: application/x-www-form-urlencoded
  Body: device=<id>&btn=<name>&csrf=<token>
```

Enables IR receiver and waits for a signal.

```json
{"ok":true,"timeout":15}
```

### Check status
```
GET /learn/status
```

Poll this every 500ms after starting a learn session.

```json
{"status":"waiting","remaining":12}
```

States:
- `waiting` — receiver is active, `remaining` = seconds left
- `done` — code received and saved
- `timeout` — no signal received within 15 seconds
- `idle` — no learning session active

### Cancel learning
```
POST /learn/cancel
  Content-Type: application/x-www-form-urlencoded
  Body: csrf=<token>
```

```json
{"ok":true}
```

### Delete learned code
```
POST /learn/delete
  Content-Type: application/x-www-form-urlencoded
  Body: device=<id>&btn=<name>&csrf=<token>
```

Removes a previously learned override for a specific button.

```json
{"ok":true}
```

---

## IRDB (Live Code Database)

### List brands
```
GET /irdb/brands?type=TV
```

`type` can be: `TV`, `AC`, `DVD_Player`, `Projector`, `SetTopBox`, `Audio`, `Fan`

Returns an array of brand strings:
```json
["Samsung","LG","Sony","Panasonic","Philips","Xiaomi"]
```

### List codes for brand
```
GET /irdb/codes?brand=Samsung&type=TV
```

```json
[
  {"fn":"Power","proto":"NEC1"},
  {"fn":"VolumeUp","proto":"NEC1"}
]
```

### IRDB browser UI
```
GET /irdb
```

Full interactive web page for browsing and sending IRDB codes.

---

## Smart Home Bridge

### Config
```
GET /smarthome/config
POST /smarthome/config?csrf=<token>
  Content-Type: application/x-www-form-urlencoded
  Body: ip=192.168.4.1
```

Set the IP address of the companion ESP8266 board.

### Sensor data
```
GET /smarthome/sensors
```

Proxies the companion board's sensor readings:

```json
{
  "ok": true,
  "temperature": 25.3,
  "humidity": 60.0,
  "light": 45,
  "gas": 12,
  "alarm": 0
}
```

`ok: false` if the companion board is unreachable.

### Control devices
```
POST /smarthome/device?csrf=<token>
  Content-Type: application/json
  Body: {"device":"lamp","status":true}
```

`device` can be: `lamp`, `cooling`.  
`status` is boolean.

### Control alarm
```
POST /smarthome/alarm?csrf=<token>
  Content-Type: application/json
  Body: {"status":true}
```

### Smart mode
```
POST /smarthome/smart?csrf=<token>
  Content-Type: application/json
  Body: {"status":0}
```

| Mode | Value | Behavior |
|------|-------|----------|
| Off | 0 | Automatic sensor response disabled |
| LDR | 1 | Responds to light sensor |
| PIR | 2 | Responds to motion sensor |

---

## Scenes

### List scenes
```
GET /scenes
```

```json
[
  {
    "id": "scene_5f8a3c",
    "name": "Living Room → Power",
    "trigger": {"device": "samsung_tv", "btn": "Power"},
    "action": {"type": "device", "device": "lamp", "state": true}
  }
]
```

### Create scene
```
POST /scenes?csrf=<token>
  Content-Type: application/json
  Body: {"name":"...","trigger":{"device":"...","btn":"..."},"action":{"type":"device","device":"lamp","state":true}}
```

Returns:
```json
{"ok":true,"id":"scene_abc123"}
```

### Delete scene
```
DELETE /scenes?id=<id>&csrf=<token>
```

```json
{"ok":true}
```

### Scene resolution

When an IR code is received by `loopReceive()`, the system:

1. Decodes the signal
2. Matches it against learned codes and the static library via `findIRInLibrary()`
3. Sets `lastRxDevice` and `lastRxBtn` globals
4. `loopScenes()` loads all saved scenes
5. If any scene's trigger matches, the action is executed: `shClient.toggleDevice(device, state)`

---

## Vendor Platform

### Config
```
GET /vendor/config
POST /vendor/config?csrf=<token>
  Content-Type: application/x-www-form-urlencoded
  Body: base_url=https://iot.example.com/api/v1&api_key=xxx&vendor_id=1&product_id=1&serial=005-IR-REMOTE-v2
```

For integration with IoT cloud platforms. Stores credentials in NVS.

### UI
```
GET /vendor
```

Configuration dashboard with live connection status.

---

## Wi-Fi

### Status
```
GET /wifi/status
```

```json
{"sta_connected":false}
```

### Configure
```
GET /wifi/config?ssid=MyNetwork&password=secret123&csrf=<token>
```

Connects to an existing Wi-Fi network in STA mode. The Access Point remains available at `192.168.4.1`.

---

## HTTP Error Codes

| Code | Meaning |
|------|---------|
| 200 | Success |
| 400 | Missing or invalid arguments |
| 403 | Missing or invalid CSRF token |
| 404 | Resource not found |
| 405 | Wrong HTTP method |
| 429 | Rate limit exceeded (IR endpoint) |
| 502 | Upstream (companion board) unreachable |

---

## Build & Compile

### Dependencies (PlatformIO)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
  crankyoldgit/IRremoteESP8266@^2.9.0
  bblanchon/ArduinoJson@^6.21.0
```

### Dependencies (Arduino IDE)
- `IRremoteESP8266` by crankyoldgit (≥2.9.0)
- `ArduinoJson` by bblanchon (≥6.21.0)

### Compile checklist

| # | Check | Why |
|---|-------|-----|
| 1 | `IRremoteESP8266` ≥2.9.0 installed | Core IR send/receive library |
| 2 | `ArduinoJson` ≥6.21.0 installed | JSON parsing (learned codes, scenes, irdb) |
| 3 | Board set to `ESP32 Dev Module` (or specific variant) | Project targets ESP32, not ESP8266 or other |
| 4 | Partition scheme: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)` | NVS + Preferences need SPIFFS partition |
| 5 | Flash mode: `QIO` / `80MHz` / 4MB flash | Standard ESP32 DevKit config |
| 6 | Enable compiler warnings (`-Wall`) | Catches unused variables, type mismatches |
| 7 | Verify `PROGMEM` attribute on all device/button arrays | Without it, strings corrupt when reading from flash |
| 8 | Check serial monitor at **115200 baud** | Must match `Serial.begin(115200)` |
| 9 | Verify GPIO 4 = IR LED, GPIO 14 = CHQ1838 output | Physical wiring must match pin constants |
| 10 | Test `/csrf` returns a 16-char hex token | CSRF system generates at boot via `esp_random()` |

### Common compile errors

| Error | Likely cause |
|-------|-------------|
| `'decode_type_t' does not name a type` | `IRremoteESP8266.h` not included (missing dep or wrong include path) |
| `'IRsend' was not declared in this scope` | Library not installed or board not set to ESP32 |
| `undefined reference to 'loopReceive'` | `web_ui.h` not included or typo in function name |
| `PROGMEM data declared in function scope` | Button/device arrays must be at file scope, not inside a function |
| `JSON buffer too small` | `DynamicJsonDocument` capacity too low for serialized data |
