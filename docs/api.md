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

Returns all 37 static devices with their buttons and learned status.

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
GET /learn/start?device=<id>&btn=<name>&csrf=<token>
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
GET /learn/cancel?csrf=<token>
```

```json
{"ok":true}
```

### Delete learned code
```
GET /learn/delete?device=<id>&btn=<name>&csrf=<token>
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
