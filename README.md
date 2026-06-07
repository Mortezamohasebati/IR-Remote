<div align="center">

# 📡 IR Remote — ESP32 Smart Infrared Controller

**Control all your home appliances wirelessly via a beautiful mobile web interface.**  
Built on ESP32 with IRremoteESP8266 — no app install required.

[![ESP32](https://img.shields.io/badge/ESP32-DevKit%20V1-blue?style=flat-square&logo=espressif)](https://www.espressif.com/)
[![IRremoteESP8266](https://img.shields.io/badge/Library-IRremoteESP8266-orange?style=flat-square)](https://github.com/crankyoldgit/IRremoteESP8266)
[![Arduino](https://img.shields.io/badge/IDE-Arduino-teal?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![WiFi](https://img.shields.io/badge/Network-Access%20Point-green?style=flat-square)]()

</div>

---

## ✨ Features

- 📱 **Mobile-first web UI** — open in any browser, no app needed
- 📺 **32 pre-loaded devices** across TVs, ACs, projectors, DVD players, and more
- 🌍 **21 brands** — Samsung, LG, Sony, Philips, Panasonic, Xiaomi, Gree, Daikin, Epson, BenQ, and major Iranian brands (Snowa, Pakshoma, Pars Khazar, XVision, G-Plus, Emersan, Daewoo, Depoo, Electrostil, Entekhab)
- 🧠 **IR Learning mode** — point any remote at the receiver and teach new buttons in seconds
- ➕ **Add new devices** entirely from the web UI — no code editing needed
- 💾 **Persistent memory** — learned codes survive power cycles (stored in ESP32 flash)
- 📶 **Standalone Access Point** — ESP32 creates its own Wi-Fi, no router required
- 🗂️ **Clean two-file architecture** — `ir_codes.h` (library) + `main.ino` (server)

---

## 🛒 Hardware

| Component | Spec | Role |
|-----------|------|------|
| **ESP32 DevKit V1** | 240 MHz dual-core | Main controller + web server |
| **IR LED 940nm 5mm** | 940 nm | Transmit IR signals |
| **IR Receiver CHQ1838** | 38 kHz carrier | Receive & learn IR signals |
| **Transistor BC547** | NPN | Drive IR LED (amplify GPIO current) |
| **Resistor** | 100 Ω | Current limit for IR LED base |
| **Resistor** | 10 kΩ | Pull-down on transistor base |

---

## 🔌 Wiring

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

### Wiring Diagram

```
                    ┌─────────────────────┐
                    │    ESP32 DevKit V1   │
    IR LED ◄── BC547◄── GPIO 4  [SEND ]   │
   CHQ1838 ──────────► GPIO 14  [RECV ]   │
                    │          3.3V / GND  │
                    └─────────────────────┘
```

---

## 📁 Project Structure

```
IR-Remote/
├── main.ino          # Web server, IR send/receive, learn mode logic
└── ir_codes.h        # All device definitions and IR code library
```

### Adding a new device to the library

Open `ir_codes.h` and follow this pattern:

```cpp
// 1. Define buttons
IRButton myDeviceBtns[] = {
  { "Power", "⏻", 0xXXXXXXXX, 32, 1 },
  { "Vol+",  "🔊", 0xXXXXXXXX, 32, 1 },
  // ...
};

// 2. Add to devices[] array at the bottom
{ "my_device", "My Device", "tv", "BrandName", "تلویزیون",
   PROTO_NEC, myDeviceBtns, 2 },
```

Supported protocols: `PROTO_NEC`, `PROTO_SAMSUNG`, `PROTO_SONY`, `PROTO_LG`, `PROTO_RC5`, `PROTO_SHARP`, `PROTO_PANASONIC`

---

## ⚙️ Installation

### 1. Install Arduino IDE
Download from [arduino.cc/en/software](https://arduino.cc/en/software)

### 2. Add ESP32 board support
In **File → Preferences → Additional Boards Manager URLs**, paste:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then go to **Tools → Board → Boards Manager**, search `esp32`, and install.

### 3. Install required libraries
Go to **Sketch → Include Library → Manage Libraries** and install:

| Library | Version |
|---------|---------|
| `IRremoteESP8266` | latest |
| `ArduinoJson` | ≥ 6.x |

### 4. Clone this repository
```bash
git clone https://github.com/Mortezamohasebati/IR-Remote.git
```
Or download the ZIP from GitHub and extract into your Arduino sketchbook folder.

### 5. Select board and upload
- **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
- **Tools → Port** → select your ESP32 COM port
- Click **Upload** ⬆

### 6. Connect and open
After upload, open **Serial Monitor** at baud `115200`. You'll see:
```
=== IR Remote v2.0 ===
AP: IR-Remote  |  IP: 192.168.4.1
Server started.
```
On your phone, connect to Wi-Fi **`IR-Remote`** (password: `12345678`), then open:
```
http://192.168.4.1
```

---

## 📱 How to Use

### Sending commands
1. Open the web UI on your phone
2. Pick a category tab: **TV / AC / Other**
3. Filter by brand using the pills
4. Tap a device card to expand its remote
5. Tap any button — IR signal is sent instantly

### Learning a new code
1. Open a device's remote panel
2. Tap **📡 Learn** button (top right of panel)
3. Tap the button you want to teach
4. Point your **original remote** at the CHQ1838 receiver and press the button
5. The UI confirms with a green dot ✅ — code is saved to flash

### Adding a completely new device
1. Tap **➕ New Device** card in any category
2. Enter device name, brand, and category
3. Select which buttons to teach
4. Follow the on-screen learning prompts one by one
5. Device appears permanently in the list

---

## 🗂️ Supported Devices

<details>
<summary><strong>📺 TVs (13 devices)</strong></summary>

| Brand | Protocol |
|-------|---------|
| Samsung | SAMSUNG |
| LG | NEC |
| Sony | SONY |
| Philips | RC5 |
| Xiaomi | NEC |
| Panasonic | PANASONIC |
| Snowa (اسنوا) | NEC |
| Pakshoma (پاکشوما) | NEC |
| Pars Khazar (پارس خزر) | NEC |
| XVision (ایکس‌ویژن) | SAMSUNG |
| G-Plus (جی‌پلاس) | NEC |
| Emersan (امرسان) | SAMSUNG |
| Daewoo (دوو) | NEC |

</details>

<details>
<summary><strong>❄️ Air Conditioners (11 devices)</strong></summary>

| Brand | Protocol |
|-------|---------|
| Samsung | SAMSUNG |
| LG | NEC |
| Panasonic | PANASONIC |
| Daikin | NEC |
| Mitsubishi | NEC |
| Gree | NEC |
| Snowa (اسنوا) | NEC |
| Entekhab (انتخاب) | NEC |
| Depoo (دپو) | NEC |
| Electrostil (الکترواستیل) | NEC |
| Pakshoma (پاکشوما) | NEC |

</details>

<details>
<summary><strong>📽️ Other (8 devices)</strong></summary>

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

## 🛠️ Configuration

All settings are at the top of `main.ino`:

```cpp
// Wi-Fi Access Point
const char* AP_SSID     = "IR-Remote";   // change network name
const char* AP_PASSWORD = "12345678";    // change password (min 8 chars)

// GPIO pins
const uint16_t IR_SEND_PIN = 4;    // IR LED transmitter
const uint16_t IR_RECV_PIN = 14;   // CHQ1838 receiver

// Learn timeout (ms)
const uint32_t LEARN_TIMEOUT = 15000;  // 15 seconds
```

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't connect to `IR-Remote` Wi-Fi | Reset ESP32, wait 5 seconds and retry |
| Page doesn't load at 192.168.4.1 | Make sure you're on the `IR-Remote` network, not your home Wi-Fi |
| Button press does nothing | Check IR LED wiring — BC547 pin order (B, C, E) |
| Learning mode times out | Hold the original remote **5–10 cm** from CHQ1838, aim directly |
| Wrong device responds | IR codes in library are typical values; use Learn mode to override with your exact remote's codes |
| Upload fails | Hold the **BOOT** button on ESP32 while clicking Upload, release after upload starts |

---

## 🤝 Contributing

Pull requests are welcome. To add a new brand:

1. Fork the repo
2. Add button array and device entry in `ir_codes.h`
3. Test on real hardware
4. Open a PR with the brand name and device model in the title

---

## 📄 License

This project is open source and free to use for personal and educational purposes.

---

<div align="center">
Made with ❤️ by <a href="https://github.com/Mortezamohasebati">Morteza Mohasebati</a>
</div>
