# Wiring Guide

## IR Transmitter Circuit

### Components
- 1× BC547 NPN transistor
- 1× 940nm IR LED (5mm)
- 1× 100Ω resistor
- 1× 10kΩ resistor
- Breadboard + jumper wires

### Schematic

```
                     +3.3V
                       │
                      ┌┴┐
                      │ │  IR LED 940nm
                      │ │  (long leg = anode = +)
                      └┬┘
                       │
                       ├──────── Collector (C)
                       │
    ESP32 GPIO4 ──► 100Ω ──► Base (B)
                       │
                      10kΩ
                       │
                       GND
```

### BC547 Pinout

```
       (flat face toward you)
       ┌──────┐
       │  B C │
       │      │
       │  E   │
       └──────┘
  B = Base (1)
  C = Collector (2)
  E = Emitter (3)
```

### Step-by-Step
1. Connect **GPIO4** to one leg of the **100Ω resistor**
2. Connect the other leg of 100Ω resistor to **BC547 Base (B)**
3. Connect **10kΩ resistor** between **Base (B) and GND** (pull-down)
4. Connect **BC547 Emitter (E)** to **GND**
5. Connect **BC547 Collector (C)** to **IR LED cathode** (short leg, –)
6. Connect **IR LED anode** (long leg, +) to **3.3V**

---

## IR Receiver Circuit

### Components
- 1× CHQ1838 IR receiver module (38 kHz)
- 1× 10μF capacitor (optional, for power smoothing)

### Schematic

```
    CHQ1838 (view from front, dome facing you)
    ┌─────────┐
    │  OUT    │  1 ──► GPIO14
    │  GND    │  2 ──► GND
    │  VCC    │  3 ──► 3.3V  ⚠️ NOT 5V
    └─────────┘
```

### Pin Identification
1. **OUT** (left) — data output, connect to GPIO14
2. **GND** (center) — ground
3. **VCC** (right) — power 3.3V

### Notes
- Do **NOT** connect VCC to 5V — the CHQ1838 will be damaged
- Keep the receiver away from the IR LED to avoid self-interference
- Optional: place a 10μF capacitor between VCC and GND near the receiver

---

## ESP8266 Companion Board (Smart Home)

### I2C Connection

```
ESP32                    ESP8266
─────                    ───────
GPIO 21 (SDA) ────────── GPIO 4 (SDA)
GPIO 22 (SCL) ────────── GPIO 5 (SCL)
GND          ────────── GND
```

### Wiring Diagram

```
┌─────────────────────┐    ┌─────────────────────┐
│     ESP32 DevKit    │    │   ESP8266 NodeMCU   │
│                     │    │                     │
│  GPIO 4  (IR SEND)──┼────► BC547 → IR LED     │
│  GPIO 14 (IR RECV)──┼────► CHQ1838 OUT        │
│                     │    │                     │
│  GPIO 21 (SDA) ─────┼─────────────────────── SDA│
│  GPIO 22 (SCL) ─────┼─────────────────────── SCL│
│  3.3V              ─┼──┐                  ┌── 3.3V│
│  GND               ─┼──┼──────────────────┼── GND│
│                     │  │                  │      │
│                     │  └── IR LED + 3.3V  │      │
│                     │       (via BC547)   │      │
│                     │                     │      │
│                     │  Relay 1 ──── Lamp  │      │
│                     │  Relay 2 ──── Fan   │      │
└─────────────────────┘  ...                  └──────┘
```

### Companion Firmware

Flash your ESP8266 with the firmware from:
https://github.com/vahidseyyedi/Arduino-Smart-Home

The firmware exposes these endpoints:
- `GET /get-sensor-data` — temperature, humidity, light, gas, alarm
- `POST /toggle-device` — control lamp, cooling by name
- `POST /toggle-alarm` — arm/disarm alarm
- `POST /toggle-smart` — set LDR/PIR smart mode

---

## Full System Pin Map

| ESP32 Pin | Connection |
|-----------|-----------|
| GPIO 4    | IR LED via BC547 (SEND) |
| GPIO 14   | CHQ1838 OUT (RECV) |
| GPIO 21   | I2C SDA (Smart Home) |
| GPIO 22   | I2C SCL (Smart Home) |
| 3.3V      | CHQ1838 VCC, IR LED anode |
| GND       | BC547 emitter, CHQ1838 GND, common ground |

## Power Supply

The ESP32 can be powered via:
- **USB** (during development) — 5V from USB port, regulated onboard to 3.3V
- **External** (production) — 7-12V DC to VIN pin, or regulated 5V to 5V pin

Total current draw with IR LED active: ~200mA max.
