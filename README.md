# IR Remote — ESP32 Smart Infrared Controller

Control all your home appliances wirelessly via a beautiful web interface.  
Built on ESP32 with IRremoteESP8266 — no app install required.

| Badge | |
|-------|-|
| [![ESP32](https://img.shields.io/badge/ESP32-DevKit%20V1-blue?logo=espressif)](https://www.espressif.com/) | [![IRremoteESP8266](https://img.shields.io/badge/Library-IRremoteESP8266-orange)](https://github.com/crankyoldgit/IRremoteESP8266) |
| [![Arduino](https://img.shields.io/badge/IDE-Arduino-teal?logo=arduino)](https://www.arduino.cc/) | [![License](https://img.shields.io/badge/license-MIT-green)](LICENSE) |

---

## Features

- **Mobile-first Apple Design System UI** — open in any browser, no app needed
- **34 pre-loaded devices** across TVs, ACs, projectors, DVD players, fans, and more
- **22 brands** including TCL, Hisense, Philips, Midea, Haier, and major Iranian brands
- **IR Learning mode** — teach new buttons by pointing any remote at the receiver
- **Live IRDB lookup** — fetch codes on-the-fly from the probonopd/irdb database (7-device LRU cache)
- **Smart Home bridge** — control lamps, cooling, and alarm via companion ESP8266 board over I2C
- **Vendor platform client** — REST API integration for IoT cloud platforms
- **Scene automation** — trigger smart home actions from received IR codes
- **Add new devices** entirely from the web UI — no code editing needed
- **Persistent memory** — learned codes survive power cycles (ESP32 flash NVS)
- **Standalone Access Point** — ESP32 creates its own Wi-Fi, no router required
- **CSRF-protected** — all state-changing endpoints require an 8-byte hex token
- **Rate-limited IR send** — max 10 requests/second on the `/ir` endpoint

---

## Hardware

| Component | Spec | Role |
|-----------|------|------|
| **ESP32 DevKit V1** | 240 MHz dual-core | Main controller + web server |
| **IR LED 940nm 5mm** | 940 nm | Transmit IR signals |
| **IR Receiver CHQ1838** | 38 kHz carrier | Receive & learn IR signals |
| **Transistor BC547** | NPN | Drive IR LED (amplify GPIO current) |
| **Resistor** | 100 Ω | Current limit for IR LED base |
| **Resistor** | 10 kΩ | Pull-down on transistor base |
| **ESP8266** (optional) | 80 MHz | Smart Home companion board |

---

## Wiring

### IR Transmitter (Send)

```
ESP32 GPIO4 ──► 100Ω ──► BC547 Base (B)
                         BC547 Collector (C) ──► IR LED (–)
                         IR LED (+) ──► 3.3V
                         BC547 Emitter (E) ──► GND
10kΩ between Base and GND (pull-down)
```

### IR Receiver CHQ1838 (Learn)

```
CHQ1838 Pin 1 (OUT) ──► GPIO14
CHQ1838 Pin 2 (GND) ──► GND
CHQ1838 Pin 3 (VCC) ──► 3.3V   ⚠️ NOT 5V
```

> **Important:** Always use **3.3V** for the CHQ1838 — 5V will damage it.

### Smart Home Companion (optional)

```
ESP32 GPIO ──► I2C SDA ──► ESP8266
ESP32 GPIO ──► I2C SCL ──► ESP8266
```

The companion board firmware is at [vahidseyyedi/Arduino-Smart-Home](https://github.com/vahidseyyedi/Arduino-Smart-Home/).

---

## Project Structure

```
IR-Remote/
├── main.ino             # Entry point — setup, loop, route registration
├── globals.h            # Shared extern declarations (server, irsend, shClient, etc.)
├── ir_codes.h           # Static IR code library — 34 devices, all PROGMEM
├── wifi_manager.h       # Self-contained AP+STA manager
├── web_ui.h             # All HTTP handlers + Apple Design System HTML/CSS/JS
├── smarthome_client.h   # HTTP client for the Smart Home companion board
├── vendor_api.h         # Vendor platform REST API client
├── scenes.h             # Scene automation — IR trigger → Smart Home action
├── irdb_client.h        # IRDB CDN fetcher, CSV parser, LRU cache, protocol translator
└── README.md
```

---

## Supported Devices (34 total)

<details>
<summary><strong>TVs (16 devices)</strong></summary>

| Brand | Protocol | Notes |
|-------|----------|-------|
| Samsung | SAMSUNG | |
| LG | NEC | |
| Sony | SONY12/SONY15 | |
| Philips | RC5 | |
| Philips | RC6 | 20-bit variant |
| Xiaomi | NEC | |
| Panasonic | PANASONIC | |
| TCL (NA/Roku) | NEC | device=0x57E3 |
| TCL (Asia/China) | RCA | device=15 (from irdb) |
| Hisense | NEC | addr 0x04 |
| Snowa (اسنوا) | NEC | |
| Pakshoma (پاکشوما) | NEC | |
| Pars Khazar (پارس خزر) | NEC | |
| XVision (ایکس‌ویژن) | SAMSUNG | |
| G-Plus (جی‌پلاس) | NEC | |
| Emersan (امرسان) | SAMSUNG | |
| Daewoo (دوو) | NEC | |

</details>

<details>
<summary><strong>Air Conditioners (13 devices)</strong></summary>

| Brand | Protocol | Notes |
|-------|----------|-------|
| Samsung | SAMSUNG | |
| LG | NEC | |
| Panasonic | PANASONIC | |
| Daikin | NEC | |
| Mitsubishi | NEC | |
| Gree | NEC | |
| Midea | NEC | addr 0x00; verify on real unit |
| Haier | N/A | placeholder 8-bit; use learn mode |
| Snowa (اسنوا) | NEC | |
| Entekhab (انتخاب) | NEC | |
| Depoo (دپو) | NEC | |
| Electrostil (الکترواستیل) | NEC | |
| Pakshoma (پاکشوما) | NEC | |

</details>

<details>
<summary><strong>Other (8 devices)</strong></summary>

| Device | Brand |
|--------|-------|
| Projector | Epson |
| Projector | BenQ |
| Projector | Xiaomi |
| Set-top Box | Generic |
| DVD Player | Generic |
| A/V Receiver | Generic |
| Smart Fan | Generic |

</details>

---

## API Reference

All endpoints return JSON. State-changing endpoints require CSRF token (see below).

### CSRF Protection

```
GET /csrf
→ {"token":"a1b2c3d4e5f67890"}

Include token in all POST/DELETE requests as:
  • Query arg:  ?csrf=a1b2c3d4e5f67890
  • Header:     X-CSRF-Token: a1b2c3d4e5f67890
```

### IR Control

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/ir?device=X&btn=Y` | Send IR code (learned → library → IRDB fallback) |
| `GET` | `/devices` | List all devices with buttons + learned status |

### Learning

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/learn/start` | Begin learning a button — body: `device=X&btn=Y&csrf=Z` |
| `GET` | `/learn/status` | Poll learning status (waiting/done/timeout/idle) |
| `POST` | `/learn/cancel` | Cancel active learning — body: `csrf=Z` |
| `POST` | `/learn/delete` | Delete a learned code — body: `device=X&btn=Y&csrf=Z` |

### IRDB (Live Code Lookup)

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/irdb/brands?type=TV` | List brands for device type |
| `GET` | `/irdb/codes?brand=X&type=TV` | List available IR codes |
| `GET` | `/irdb` | IRDB browser UI |

### Smart Home Bridge

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/smarthome` | Smart Home dashboard UI |
| `GET/POST` | `/smarthome/config` | Get/set companion board IP |
| `GET` | `/smarthome/sensors` | Read sensor data (temp, humidity, light, gas, alarm) |
| `POST` | `/smarthome/device` | Toggle device: `{"device":"lamp","status":true}` |
| `POST` | `/smarthome/alarm` | Toggle alarm: `{"status":true}` |
| `POST` | `/smarthome/smart` | Set smart mode: `{"status":0}` (0=off, 1=LDR, 2=PIR) |

### Scenes

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/scenes` | List all scenes |
| `POST` | `/scenes` | Create scene: `{"name":"...","trigger":{"device":"...","btn":"..."},"action":{"type":"device","device":"lamp","state":true}}` |
| `DELETE` | `/scenes?id=X` | Delete scene |

### Vendor Platform

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/vendor` | Vendor configuration UI |
| `GET/POST` | `/vendor/config` | Get/set vendor API credentials |

### Wi-Fi

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/wifi/status` | Check STA connection status |
| `GET` | `/wifi/config` | Configure Wi-Fi (requires CSRF) |

---

## Installation

### 1. Install Arduino IDE

Download from [arduino.cc/en/software](https://arduino.cc/en/software)

### 2. Add ESP32 board support

In **File → Preferences → Additional Boards Manager URLs**, paste:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-packages/package_esp32_index.json
```
Then **Tools → Board → Boards Manager**, search `esp32`, and install **ESP32 Arduino** (v3.x).

### 3. Install required libraries

**Sketch → Include Library → Manage Libraries**:

| Library | Version |
|---------|---------|
| `IRremoteESP8266` | ≥ 2.9.0 |
| `ArduinoJson` | ≥ 6.x |

### 4. Clone and open

```bash
git clone https://github.com/Mortezamohasebati/IR-Remote.git
```

Open the `IR-Remote` folder in Arduino IDE — it auto-detects the sketch.

### 5. Select board and upload

- **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
- **Tools → Port** → select your ESP32 COM port
- Click **Upload**

### 6. Connect

After upload, open **Serial Monitor** at 115200 baud:
```
=== IR Remote v2.0 ===
AP: IR-Remote  |  IP: 192.168.4.1
Server started.
```

On your phone, connect to Wi-Fi `IR-Remote` (password: `12345678`), then open:
```
http://192.168.4.1
```

---

## Configuration

Settings at the top of `main.ino`:

```cpp
const uint16_t IR_SEND_PIN = 4;    // IR LED transmitter GPIO
const uint16_t IR_RECV_PIN = 14;   // CHQ1838 receiver GPIO
const uint16_t CAPTURE_BUF = 1024; // IR capture buffer size
const uint32_t LEARN_TIMEOUT = 15000; // 15 seconds
```

Wi-Fi AP credentials in `wifi_manager.h` (default: SSID `IR-Remote`, password `12345678`).

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't connect to `IR-Remote` Wi-Fi | Reset ESP32, wait 5 seconds, retry |
| Page doesn't load at 192.168.4.1 | Make sure you're on the `IR-Remote` network, not your home Wi-Fi |
| Button press does nothing | Check IR LED wiring — BC547 pin order (B, C, E) |
| Learning mode times out | Hold original remote **5–10 cm** from CHQ1838, aim directly |
| Wrong codes for TCL | Try both TCL entries — NEC for Roku models, RCA for Asia/China models |
| Smart Home board unreachable | Verify IP in `/smarthome/config`, ensure companion board is powered |
| Upload fails | Hold **BOOT** button on ESP32 while clicking Upload, release after upload starts |

---

## License

MIT — free to use for personal and educational purposes.
