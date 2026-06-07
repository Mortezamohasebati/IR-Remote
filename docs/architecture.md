# Architecture

## Overview

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32 DevKit V1                     │
│                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │ IR Send  │  │ IR Recv  │  │  WebServer (port 80)  │  │
│  │ GPIO 4   │  │ GPIO 14  │  │  AP: 192.168.4.1    │  │
│  └────┬─────┘  └────┬─────┘  └──────────┬───────────┘  │
│       │             │                    │              │
│       ▼             ▼                    ▼              │
│  ┌────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │BC547+NPN│  │CHQ1838   │  │  Handlers (web_ui.h) │  │
│  │+IR LED  │  │IR Receiver│  │  /ir, /learn, /scenes│  │
│  └────────┘  └──────────┘  └──────────┬───────────┘  │
│                                       │              │
│  ┌──────────┐  ┌──────────┐  ┌───────┴───────────┐  │
│  │wifi_     │  │irdb_     │  │    smarthome_     │  │
│  │manager.h │  │client.h  │  │    client.h       │  │
│  └──────────┘  └──────────┘  └───────┬───────────┘  │
│                                       │              │
│  ┌──────────┐  ┌──────────┐  ┌───────┴───────────┐  │
│  │ir_codes.h│  │scenes.h  │  │   ESP8266 via I2C  │  │
│  │37 devices│  │IR→Relay  │  │   /toggle-device   │  │
│  └──────────┘  └──────────┘  │   /get-sensor-data │  │
│                              └────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## File Responsibilities

| File | Role |
|------|------|
| `main.ino` | Entry point: hardware init, globals, loop, route registration |
| `globals.h` | Single header declaring all shared `extern` objects |
| `ir_codes.h` | Static IR code DB: 37 devices, 6 protocols, all PROGMEM |
| `web_ui.h` | All HTTP handlers + full Apple Design System HTML/CSS/JS |
| `wifi_manager.h` | AP+STA dual-mode WiFi manager |
| `irdb_client.h` | CDN-based IRDB client: CSV parser, LRU cache, protocol packer |
| `smarthome_client.h` | HTTP client for ESP8266 smart home companion board |
| `vendor_api.h` | REST client for IoT vendor platform integration |
| `scenes.h` | Scene automation: IR trigger → smart home action |

## Data Flow

### Sending an IR code
```
User taps button in web UI
  → fetch('/ir?device=samsung_tv&btn=Power')
    → handleIRSend()
      → loadLearnedCode()  [check NVS for learned override]
      → sendCode()          [send from static library]
      → irdbClient.fetchCodesForBrand()  [fallback to IRDB CDN]
        → irsend.send()     [transmit via GPIO 4]
```

### Learning a new code
```
User taps Learn → button in web UI
  → fetch('/learn/start?device=X&btn=Y')
    → irrecv enabled, learnMode = true
  → User points remote at receiver
    → loopReceive() captures decode_results
    → saveLearnedCode() stores to Preferences NVS
    → learnDone = true
  → UI polls /learn/status → "done"
```

### Scene trigger
```
IR received → loopReceive() calls findIRInLibrary()
  → matches static library or learned code
  → sets lastRxDevice / lastRxBtn globals
  → loopScenes() checks saved scenes
  → if trigger matches: shClient.toggleDevice()
```

## Memory Model

ESP32 Arduino core maps PROGMEM data into the MMU address space, so no special read functions are needed (unlike AVR). All device button arrays and lookup tables use PROGMEM to minimize DRAM usage.

## Key Protocols Handled

| Protocol | send() method | IRDB mapping |
|----------|--------------|--------------|
| NEC | `sendNEC()` | NEC1, NEC2 |
| SAMSUNG | `sendSAMSUNG()` | SAMSUNG36, SAMSUNG48 |
| SONY | `sendSony()` | SONY12, SONY15, SONY20 |
| RC5 | `sendRC5()` | RC5, RC5X |
| RC6 | `sendRC6()` | RC6 |
| PANASONIC | `sendPanasonic()` | PANASONIC |
| LG | `sendLG()` | LG |
| SHARP | `sendSharpRaw()` | SHARP |
| RCA | `sendNEC()` | RCA (NEC timing) |

The `irdbPackCode()` function in `irdb_client.h` translates IRDB protocol strings (e.g., `"NEC1"`, `"SAMSUNG36"`) to the correct `decode_type_t` enum values with the appropriate bit counts and data packing formulas.
