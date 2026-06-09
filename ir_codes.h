#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <IRremoteESP8266.h>

// ── Button ──────────────────────────────────────────────────
struct IRButton {
  const char* name;
  const char* icon;
  uint64_t    code;
  uint8_t     bits;
  uint8_t     repeat;
};

// ── Device ──────────────────────────────────────────────────
struct DeviceIR {
  const char* id;
  const char* name;
  const char* category;
  const char* brand;
  const char* catLabel;
  decode_type_t  protocol;
  IRButton*   buttons;
  uint8_t     btnCount;
};

// ════════════════════════════════════════════════════════════
//  IRDB_FUNC_NAMES — lookup table for irdb function numbers
// ════════════════════════════════════════════════════════════
struct FuncName { int function; const char* name; const char* icon; };

const FuncName IRDB_FUNC_NAMES[] PROGMEM = {
  {2,  "Power",       "\xf0\x9f\x8f\xbb"},
  {16, "Vol+",        "\xf0\x9f\x94\x8a"},
  {17, "Vol-",        "\xf0\x9f\x94\x89"},
  {13, "Mute",        "\xf0\x9f\x94\x87"},
  {32, "CH+",         "\xe2\xac\x86"},
  {33, "CH-",         "\xe2\xac\x87"},
  {12, "OK",          "\xe2\x9c\x94"},
  {6,  "Up",          "\xe2\x96\xb2"},
  {7,  "Down",        "\xe2\x96\xbc"},
  {8,  "Left",        "\xe2\x97\x80"},
  {9,  "Right",       "\xe2\x96\xb6"},
  {28, "Back",        "\xe2\x86\xa9"},
  {36, "Home",        "\xf0\x9f\x8f\xa0"},
  {35, "Menu",        "\xe2\x98\xb0"},
  {56, "Source",      "\xe2\x8e\x98"},
  {20, "Play",        "\xe2\x96\xb6"},
  {21, "Pause",       "\xe2\x8f\xb8"},
  {22, "Stop",        "\xe2\x8f\xb9"},
  {23, "Prev",        "\xe2\x8f\xae"},
  {24, "Next",        "\xe2\x8f\xad"},
  {-1, nullptr, nullptr}
};

// ╔══════════════════════════════════════════════════════════╗
// ║  TVs — International                                    ║
// ╚══════════════════════════════════════════════════════════╝

IRButton samsungTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xE0E040BF,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0xE0E0E01F,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0xE0E0D02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0xE0E0F00F,32,1},
  {"CH+","\xe2\xac\x86",0xE0E048B7,32,1},  {"CH-","\xe2\xac\x87",0xE0E008F7,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0xE0E09E61,32,1}, {"Menu","\xe2\x98\xb0",0xE0E058A7,32,1},
  {"OK","\xe2\x9c\x94",0xE0E016E9,32,1},   {"Up","\xe2\x96\xb2",0xE0E006F9,32,1},
  {"Down","\xe2\x96\xbc",0xE0E08679,32,1}, {"Left","\xe2\x97\x80",0xE0E0A659,32,1},
  {"Right","\xe2\x96\xb6",0xE0E046B9,32,1},{"Back","\xe2\x86\xa9",0xE0E01AE5,32,1},
  {"Source","\xe2\x8e\x98",0xE0E0807F,32,1},{"Netflix","N",0xE0E031CE,32,1},
  {"Smart Hub","\xf0\x9f\x96\xa5",0xE0E09E61,32,1},
};

IRButton lgTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x20DF10EF,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x20DF40BF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x20DFC03F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x20DF906F,32,1},
  {"CH+","\xe2\xac\x86",0x20DF00FF,32,1},  {"CH-","\xe2\xac\x87",0x20DF807F,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0x20DFBC43,32,1}, {"Menu","\xe2\x98\xb0",0x20DFC23D,32,1},
  {"OK","\xe2\x9c\x94",0x20DF44BB,32,1},   {"Up","\xe2\x96\xb2",0x20DF02FD,32,1},
  {"Down","\xe2\x96\xbc",0x20DF827D,32,1}, {"Left","\xe2\x97\x80",0x20DFE01F,32,1},
  {"Right","\xe2\x96\xb6",0x20DF609F,32,1},{"Back","\xe2\x86\xa9",0x20DF14EB,32,1},
  {"Source","\xe2\x8e\x98",0x20DFD02F,32,1},{"Settings","\xe2\x9a\x99",0x20DF43BC,32,1},
};

IRButton sonyTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x095,12,3},  {"Vol+","\xf0\x9f\x94\x8a",0x092,12,3},
  {"Vol-","\xf0\x9f\x94\x89",0x093,12,3},  {"Mute","\xf0\x9f\x94\x87",0x094,12,3},
  {"CH+","\xe2\xac\x86",0x090,12,3},   {"CH-","\xe2\xac\x87",0x091,12,3},
  {"Home","\xf0\x9f\x8f\xa0",0x0F9,15,3}, {"Menu","\xe2\x98\xb0",0x0E0,15,3},
  {"OK","\xe2\x9c\x94",0x0E5,15,3},   {"Up","\xe2\x96\xb2",0x0E7,12,3},
  {"Down","\xe2\x96\xbc",0x0F5,12,3}, {"Left","\xe2\x97\x80",0x0B4,12,3},
  {"Right","\xe2\x96\xb6",0x0B3,12,3},{"Back","\xe2\x86\xa9",0x0E3,15,3},
  {"Source","\xe2\x8e\x98",0x0A5,12,3},
};

IRButton philipsTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x010C,13,1}, {"Vol+","\xf0\x9f\x94\x8a",0x0110,13,1},
  {"Vol-","\xf0\x9f\x94\x89",0x0111,13,1}, {"Mute","\xf0\x9f\x94\x87",0x010D,13,1},
  {"CH+","\xe2\xac\x86",0x0120,13,1},  {"CH-","\xe2\xac\x87",0x0121,13,1},
  {"Home","\xf0\x9f\x8f\xa0",0x0164,13,1}, {"Menu","\xe2\x98\xb0",0x0112,13,1},
  {"OK","\xe2\x9c\x94",0x010F,13,1},   {"Up","\xe2\x96\xb2",0x0110,13,1},
  {"Down","\xe2\x96\xbc",0x0111,13,1}, {"Back","\xe2\x86\xa9",0x010C,13,1},
  {"Source","\xe2\x8e\x98",0x0138,13,1},
};

IRButton philipsRC6TVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x000C,20,1}, {"Vol+","\xf0\x9f\x94\x8a",0x0010,20,1},
  {"Vol-","\xf0\x9f\x94\x89",0x0011,20,1}, {"Mute","\xf0\x9f\x94\x87",0x000D,20,1},
  {"CH+","\xe2\xac\x86",0x0020,20,1},  {"CH-","\xe2\xac\x87",0x0021,20,1},
  {"Menu","\xe2\x98\xb0",0x000E,20,1}, {"OK","\xe2\x9c\x94",0x000F,20,1},
  {"Up","\xe2\x96\xb2",0x001C,20,1},   {"Down","\xe2\x96\xbc",0x001D,20,1},
  {"Left","\xe2\x97\x80",0x002C,20,1}, {"Right","\xe2\x96\xb6",0x002D,20,1},
  {"Back","\xe2\x86\xa9",0x001A,20,1}, {"Source","\xe2\x8e\x98",0x007B,20,1},
};

// TCL TV (North America/NEC) — UNVERIFIED
// Source: community gist. Model unknown. Protocol inferred as NEC.
// If codes do not work, use learn-mode to capture from your remote.
IRButton tclTVBtns[] PROGMEM = {
  // TCL NEC variant — common for North America/Roku models (0x57E3 prefix)
  {"Power","\xe2\x8f\xbb",0x57E3E817,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x57E3F00F,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x57E308F7,32,1}, {"Mute","\xf0\x9f\x94\x87",0x57E304FB,32,1},
  {"CH+","\xe2\xac\x86",0x57E39867,32,1},  {"CH-","\xe2\xac\x87",0x57E3CC33,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0x57E3C03F,32,1}, {"Menu","\xe2\x98\xb0",0x57E3E01F,32,1},
  {"OK","\xe2\x9c\x94",0x57E354AB,32,1},   {"Up","\xe2\x96\xb2",0x57E39867,32,1},
  {"Down","\xe2\x96\xbc",0x57E3CC33,32,1}, {"Left","\xe2\x97\x80",0x57E37887,32,1},
  {"Right","\xe2\x96\xb6",0x57E3B44B,32,1},{"Back","\xe2\x86\xa9",0x57E36699,32,1},
  {"Source","\xe2\x8e\x98",0x57E3A45B,32,1},{"Netflix","N",0x57E34AB5,32,1},
};
// VERIFY: TCL codes sourced from community gist; some models may use RCA-38 variant (see tclTVRcaBtns)

IRButton tclTVRcaBtns[] PROGMEM = {
  // TCL RCA variant — Asia/China models (irdb 15,-1.csv, device=15)
  // Format: device(0x0F) << 24 | ~device(0xF0) << 16 | function << 8 | ~function
  {"Power","\xe2\x8f\xbb",0x0FF02AD5,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x0FF02FD0,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x0FF02ED1,32,1}, {"Mute","\xf0\x9f\x94\x87",0x0FF03FC0,32,1},
  {"CH+","\xe2\xac\x86",0x0FF02DD2,32,1},  {"CH-","\xe2\xac\x87",0x0FF02CD3,32,1},
  {"Menu","\xe2\x98\xb0",0x0FF008F7,32,1}, {"OK","\xe2\x9c\x94",0x0FF0F40B,32,1},
  {"Up","\xe2\x96\xb2",0x0FF059A6,32,1},   {"Down","\xe2\x96\xbc",0x0FF058A7,32,1},
  {"Left","\xe2\x97\x80",0x0FF056A9,32,1}, {"Right","\xe2\x96\xb6",0x0FF057A8,32,1},
  {"Source","\xe2\x8e\x98",0x0FF0A35C,32,1},{"Back","\xe2\x86\xa9",0x0FF027D8,32,1},
};
// VERIFY: RCA-38 codes use NEC framing with different address space

IRButton hisenseTVBtns[] PROGMEM = {
  // Hisense NEC (address 0x04 per official discrete IR PDF)
  {"Power","\xe2\x8f\xbb",0x04FB0BF4,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x04FB10EF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x04FB11EE,32,1}, {"Mute","\xf0\x9f\x94\x87",0x04FB0DF2,32,1},
  {"CH+","\xe2\xac\x86",0x04FB20DF,32,1},  {"CH-","\xe2\xac\x87",0x04FB21DE,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0x04FB04FB,32,1}, {"Menu","\xe2\x98\xb0",0x04FB54AB,32,1},
  {"OK","\xe2\x9c\x94",0x04FB55AA,32,1},   {"Up","\xe2\x96\xb2",0x04FB58A7,32,1},
  {"Down","\xe2\x96\xbc",0x04FB59A6,32,1}, {"Left","\xe2\x97\x80",0x04FB5AA5,32,1},
  {"Right","\xe2\x96\xb6",0x04FB5BA4,32,1},{"Back","\xe2\x86\xa9",0x04FB1AE5,32,1},
  {"Source","\xe2\x8e\x98",0x04FB54AB,32,1},
};
// VERIFY: Hisense codes use address 0x04; some models may use 0x01 or other

IRButton xiaomiTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x57E3A05F,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x57E350AF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x57E3D02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x57E3906F,32,1},
  {"CH+","\xe2\xac\x86",0x57E300FF,32,1},  {"CH-","\xe2\xac\x87",0x57E3807F,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0x57E3C03F,32,1}, {"Menu","\xe2\x98\xb0",0x57E3E01F,32,1},
  {"OK","\xe2\x9c\x94",0x57E37887,32,1},   {"Up","\xe2\x96\xb2",0x57E302FD,32,1},
  {"Down","\xe2\x96\xbc",0x57E3827D,32,1}, {"Left","\xe2\x97\x80",0x57E3E21D,32,1},
  {"Right","\xe2\x96\xb6",0x57E3629D,32,1},{"Back","\xe2\x86\xa9",0x57E3A25D,32,1},
  {"Mi","\xe2\x97\x8a",0x57E31AE5,32,1},
};

IRButton panasonicTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x400401,22,1}, {"Vol+","\xf0\x9f\x94\x8a",0x400425,22,1},
  {"Vol-","\xf0\x9f\x94\x89",0x400424,22,1}, {"Mute","\xf0\x9f\x94\x87",0x400409,22,1},
  {"CH+","\xe2\xac\x86",0x400434,22,1},  {"CH-","\xe2\xac\x87",0x400435,22,1},
  {"Home","\xf0\x9f\x8f\xa0",0x40043D,22,1}, {"Menu","\xe2\x98\xb0",0x40040E,22,1},
  {"OK","\xe2\x9c\x94",0x400449,22,1},   {"Up","\xe2\x96\xb2",0x400458,22,1},
  {"Down","\xe2\x96\xbc",0x400459,22,1}, {"Left","\xe2\x97\x80",0x40045A,22,1},
  {"Right","\xe2\x96\xb6",0x40045B,22,1},{"Back","\xe2\x86\xa9",0x40046C,22,1},
  {"Source","\xe2\x8e\x98",0x40044C,22,1},
};

// ── Iranian TVs ────────────────────────────────────────────

IRButton snowaTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x807F48B7,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x807F50AF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x807FD02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x807F906F,32,1},
  {"CH+","\xe2\xac\x86",0x807F00FF,32,1},  {"CH-","\xe2\xac\x87",0x807F807F,32,1},
  {"Menu","\xe2\x98\xb0",0x807FA25D,32,1}, {"OK","\xe2\x9c\x94",0x807F22DD,32,1},
  {"Up","\xe2\x96\xb2",0x807FC23D,32,1},   {"Down","\xe2\x96\xbc",0x807F827D,32,1},
  {"Left","\xe2\x97\x80",0x807FE21D,32,1}, {"Right","\xe2\x96\xb6",0x807F629D,32,1},
  {"Source","\xe2\x8e\x98",0x807FA05F,32,1},{"Back","\xe2\x86\xa9",0x807FF00F,32,1},
};

IRButton pakshomaTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xFF02FD,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0xFF42BD,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0xFFC23D,32,1}, {"Mute","\xf0\x9f\x94\x87",0xFF52AD,32,1},
  {"CH+","\xe2\xac\x86",0xFF12ED,32,1},  {"CH-","\xe2\xac\x87",0xFF9267,32,1},
  {"Menu","\xe2\x98\xb0",0xFF8A75,32,1}, {"OK","\xe2\x9c\x94",0xFF38C7,32,1},
  {"Up","\xe2\x96\xb2",0xFF18E7,32,1},   {"Down","\xe2\x96\xbc",0xFF4AB5,32,1},
  {"Left","\xe2\x97\x80",0xFF10EF,32,1}, {"Right","\xe2\x96\xb6",0xFF5AA5,32,1},
  {"Source","\xe2\x8e\x98",0xFF22DD,32,1},
};

IRButton parsKhazarTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xFF02FD,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0xFF52AD,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0xFF4AB5,32,1}, {"Mute","\xf0\x9f\x94\x87",0xFF08F7,32,1},
  {"CH+","\xe2\xac\x86",0xFF18E7,32,1},  {"CH-","\xe2\xac\x87",0xFF10EF,32,1},
  {"Menu","\xe2\x98\xb0",0xFF38C7,32,1}, {"OK","\xe2\x9c\x94",0xFF5AA5,32,1},
  {"Source","\xe2\x8e\x98",0xFF22DD,32,1},{"Back","\xe2\x86\xa9",0xFFD02F,32,1},
};

IRButton xvisionTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xE0E040BF,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0xE0E0E01F,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0xE0E0D02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0xE0E0F00F,32,1},
  {"CH+","\xe2\xac\x86",0xE0E048B7,32,1},  {"CH-","\xe2\xac\x87",0xE0E008F7,32,1},
  {"Menu","\xe2\x98\xb0",0xE0E058A7,32,1}, {"OK","\xe2\x9c\x94",0xE0E016E9,32,1},
  {"Up","\xe2\x96\xb2",0xE0E006F9,32,1},   {"Down","\xe2\x96\xbc",0xE0E08679,32,1},
  {"Left","\xe2\x97\x80",0xE0E0A659,32,1}, {"Right","\xe2\x96\xb6",0xE0E046B9,32,1},
  {"Source","\xe2\x8e\x98",0xE0E0807F,32,1},{"Back","\xe2\x86\xa9",0xE0E01AE5,32,1},
};

IRButton gplusTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x20DF10EF,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x20DF40BF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x20DFC03F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x20DF906F,32,1},
  {"CH+","\xe2\xac\x86",0x20DF00FF,32,1},  {"CH-","\xe2\xac\x87",0x20DF807F,32,1},
  {"Menu","\xe2\x98\xb0",0x20DFC23D,32,1}, {"OK","\xe2\x9c\x94",0x20DF44BB,32,1},
  {"Up","\xe2\x96\xb2",0x20DF02FD,32,1},   {"Down","\xe2\x96\xbc",0x20DF827D,32,1},
  {"Left","\xe2\x97\x80",0x20DFE01F,32,1}, {"Right","\xe2\x96\xb6",0x20DF609F,32,1},
  {"Source","\xe2\x8e\x98",0x20DFD02F,32,1},
};

IRButton emersanTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xCF3040BF,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0xCF30E01F,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0xCF30D02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0xCF30906F,32,1},
  {"CH+","\xe2\xac\x86",0xCF3048B7,32,1},  {"CH-","\xe2\xac\x87",0xCF3008F7,32,1},
  {"Menu","\xe2\x98\xb0",0xCF3058A7,32,1}, {"OK","\xe2\x9c\x94",0xCF3016E9,32,1},
  {"Up","\xe2\x96\xb2",0xCF3006F9,32,1},   {"Down","\xe2\x96\xbc",0xCF308679,32,1},
  {"Source","\xe2\x8e\x98",0xCF30807F,32,1},
};

IRButton daewooTVBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x827D48B7,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x827D50AF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x827DD02F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x827D10EF,32,1},
  {"CH+","\xe2\xac\x86",0x827D00FF,32,1},  {"CH-","\xe2\xac\x87",0x827D807F,32,1},
  {"Menu","\xe2\x98\xb0",0x827DA05F,32,1}, {"OK","\xe2\x9c\x94",0x827D22DD,32,1},
  {"Up","\xe2\x96\xb2",0x827DC23D,32,1},   {"Down","\xe2\x96\xbc",0x827D827D,32,1},
  {"Source","\xe2\x8e\x98",0x827DA25D,32,1},
};

// ╔══════════════════════════════════════════════════════════╗
// ║  Air Conditioners — International                       ║
// ╚══════════════════════════════════════════════════════════╝

IRButton samsungACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x5480000000LL,48,1},  {"Cool","\xe2\x9d\x84",0x5480040090LL,48,1},
  {"Heat","\xf0\x9f\x94\xa5",0x5480040098LL,48,1},  {"Fan","\xf0\x9f\x92\xa8",0x5480040094LL,48,1},
  {"Auto","\xe2\x99\xbb",0x5480040092LL,48,1},   {"Temp+","\xf0\x9f\x8c\xa1",0x5481040090LL,48,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0x5482040090LL,48,1}, {"Fan+","\xf0\x9f\x92\xa8",0x5480050090LL,48,1},
  {"Fan-","\xf0\x9f\x92\xa8",0x5480060090LL,48,1},  {"Swing","\xe2\x86\x95",0x5480040490LL,48,1},
  {"Sleep","\xf0\x9f\x98\xb4",0x5480240090LL,48,1},
};

IRButton lgACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x88000B70,32,1},  {"Cool 18\xc2\xb0","\xe2\x9d\x84",0x880002A4,32,1},
  {"Cool 20\xc2\xb0","\xe2\x9d\x84",0x880000A4,32,1},{"Cool 22\xc2\xb0","\xe2\x9d\x84",0x880012A4,32,1},
  {"Cool 24\xc2\xb0","\xe2\x9d\x84",0x880022A4,32,1},{"Cool 26\xc2\xb0","\xe2\x9d\x84",0x880032A4,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0x880002A0,32,1},  {"Fan Low","\xf0\x9f\x92\xa8",0x88000240,32,1},
  {"Fan Med","\xf0\x9f\x92\xa8",0x88000280,32,1},{"Fan Hi","\xf0\x9f\x92\xa8",0x880002C0,32,1},
  {"Swing","\xe2\x86\x95",0x88C302A4,32,1},  {"Sleep","\xf0\x9f\x98\xb4",0x88000EA4,32,1},
};

IRButton panasonicACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x340034002000LL,48,1},
  {"Cool","\xe2\x9d\x84",0x340034003000LL,48,1},
  {"Heat","\xf0\x9f\x94\xa5",0x340034004000LL,48,1},
  {"Auto","\xe2\x99\xbb",0x340034001000LL,48,1},
  {"Dry","\xf0\x9f\x92\xa7",0x340034002400LL,48,1},
  {"Fan Low","\xf0\x9f\x92\xa8",0x340034002100LL,48,1},
  {"Fan Med","\xf0\x9f\x92\xa8",0x340034002200LL,48,1},
  {"Fan Hi","\xf0\x9f\x92\xa8",0x340034002300LL,48,1},
  {"Temp+","\xf0\x9f\x8c\xa1",0x340034002001LL,48,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0x340034002002LL,48,1},
  {"Swing","\xe2\x86\x95",0x340034002010LL,48,1},
  {"Sleep","\xf0\x9f\x98\xb4",0x340034002020LL,48,1},
};

// Daikin AC intentionally omitted from static offline pack.
// Daikin uses IRDaikin2 — 28/32-bit stateful protocol.
// Cannot be represented as a static hex code reliably.
// User must use learn-mode to capture codes from their physical remote,
// OR use the irdb live fetch which handles this via raw pulse arrays.
// Mitsubishi Electric AC intentionally omitted from static offline pack.
// Mitsubishi uses IRMitsubishiAC — stateful proprietary protocol.
// Cannot be represented as a static hex code reliably.
// User must use learn-mode to capture codes from their physical remote,
// OR use the irdb live fetch which handles this via raw pulse arrays.

IRButton greeACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x9B25F00000LL,48,1},  {"Cool 18\xc2\xb0","\xe2\x9d\x84",0x9B09F00000LL,48,1},
  {"Cool 20\xc2\xb0","\xe2\x9d\x84",0x9B29F00000LL,48,1},{"Cool 22\xc2\xb0","\xe2\x9d\x84",0x9B49F00000LL,48,1},
  {"Cool 24\xc2\xb0","\xe2\x9d\x84",0x9B69F00000LL,48,1},{"Cool 26\xc2\xb0","\xe2\x9d\x84",0x9B89F00000LL,48,1},
  {"Heat","\xf0\x9f\x94\xa5",0x9B09F04000LL,48,1},  {"Dry","\xf0\x9f\x92\xa7",0x9B09F02000LL,48,1},
  {"Fan Auto","\xf0\x9f\x92\xa8",0x9B09F00400LL,48,1},{"Swing","\xe2\x86\x95",0x9B09F00200LL,48,1},
  {"Sleep","\xf0\x9f\x98\xb4",0x9B09F08000LL,48,1}, {"Turbo","\xe2\x9a\xa1",0x9B09F00010LL,48,1},
};

IRButton mideaACBtns[] PROGMEM = {
  // Midea AC — NEC protocol, common address 0x00 for many models
  {"Power","\xe2\x8f\xbb",0x00FF807F,32,1},  {"Cool","\xe2\x9d\x84",0x00FF48B7,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0x00FF08F7,32,1},  {"Fan","\xf0\x9f\x92\xa8",0x00FF50AF,32,1},
  {"Auto","\xe2\x99\xbb",0x00FF906F,32,1},   {"Dry","\xf0\x9f\x92\xa7",0x00FFB04F,32,1},
  {"Temp+","\xf0\x9f\x8c\xa1",0x00FF58A7,32,1}, {"Temp-","\xf0\x9f\x8c\xa1",0x00FF5AA5,32,1},
  {"Fan+","\xf0\x9f\x92\xa8",0x00FF52AD,32,1},  {"Sleep","\xf0\x9f\x98\xb4",0x00FF9867,32,1},
};
// VERIFY: Midea AC codes are generic NEC; some models use proprietary Midea protocol

// Haier AC intentionally omitted from static offline pack.
// Haier uses IRHaierAC176 — 176-bit stateful protocol.
// Cannot be represented as a static hex code.
// User must use learn-mode to capture codes from their physical remote,
// OR use the irdb live fetch which handles this via raw pulse arrays.

// ── Iranian ACs ────────────────────────────────────────────

IRButton snowaACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xB24DBF40,32,1},  {"Cool","\xe2\x9d\x84",0xB24D9F60,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0xB24D1FE0,32,1},  {"Fan","\xf0\x9f\x92\xa8",0xB24D7F80,32,1},
  {"Auto","\xe2\x99\xbb",0xB24DEF10,32,1},   {"Dry","\xf0\x9f\x92\xa7",0xB24D5FA0,32,1},
  {"Temp+","\xf0\x9f\x8c\xa1",0xB24DCF30,32,1}, {"Temp-","\xf0\x9f\x8c\xa1",0xB24D4FB0,32,1},
  {"Fan+","\xf0\x9f\x92\xa8",0xB24D6F90,32,1},  {"Fan-","\xf0\x9f\x92\xa8",0xB24DFF00,32,1},
  {"Sleep","\xf0\x9f\x98\xb4",0xB24D3FC0,32,1}, {"Swing","\xe2\x86\x95",0xB24DDF20,32,1},
};

IRButton entekhabACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xE0F00AFF,32,1},  {"Cool","\xe2\x9d\x84",0xE0F018E7,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0xE0F048B7,32,1},  {"Fan","\xf0\x9f\x92\xa8",0xE0F028D7,32,1},
  {"Dry","\xf0\x9f\x92\xa7",0xE0F088F7,32,1},   {"Temp+","\xf0\x9f\x8c\xa1",0xE0F058A7,32,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0xE0F038C7,32,1}, {"Fan+","\xf0\x9f\x92\xa8",0xE0F068B7,32,1},
  {"Fan-","\xf0\x9f\x92\xa8",0xE0F008F7,32,1},  {"Sleep","\xf0\x9f\x98\xb4",0xE0F078B7,32,1},
  {"Swing","\xe2\x86\x95",0xE0F0C837,32,1},
};

IRButton depooACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xF30C40BF,32,1},  {"Cool","\xe2\x9d\x84",0xF30C9F60,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0xF30C1FE0,32,1},  {"Auto","\xe2\x99\xbb",0xF30CEF10,32,1},
  {"Dry","\xf0\x9f\x92\xa7",0xF30C5FA0,32,1},   {"Temp+","\xf0\x9f\x8c\xa1",0xF30CCF30,32,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0xF30C4FB0,32,1}, {"Fan+","\xf0\x9f\x92\xa8",0xF30C6F90,32,1},
  {"Fan-","\xf0\x9f\x92\xa8",0xF30CFF00,32,1},  {"Sleep","\xf0\x9f\x98\xb4",0xF30C3FC0,32,1},
  {"Swing","\xe2\x86\x95",0xF30CDF20,32,1},
};

IRButton electrostilACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xA15E40BF,32,1},  {"Cool","\xe2\x9d\x84",0xA15E9060,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0xA15E10E0,32,1},  {"Fan","\xf0\x9f\x92\xa8",0xA15E7080,32,1},
  {"Dry","\xf0\x9f\x92\xa7",0xA15E5020,32,1},   {"Temp+","\xf0\x9f\x8c\xa1",0xA15EC030,32,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0xA15E40B0,32,1}, {"Sleep","\xf0\x9f\x98\xb4",0xA15E30C0,32,1},
  {"Swing","\xe2\x86\x95",0xA15ED020,32,1},
};

IRButton pakshomaACBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xC3003CFF,32,1},  {"Cool","\xe2\x9d\x84",0xC3009F60,32,1},
  {"Heat","\xf0\x9f\x94\xa5",0xC3001FE0,32,1},  {"Auto","\xe2\x99\xbb",0xC300EF10,32,1},
  {"Dry","\xf0\x9f\x92\xa7",0xC3005FA0,32,1},   {"Temp+","\xf0\x9f\x8c\xa1",0xC300CF30,32,1},
  {"Temp-","\xf0\x9f\x8c\xa1",0xC3004FB0,32,1}, {"Fan+","\xf0\x9f\x92\xa8",0xC3006F90,32,1},
  {"Fan-","\xf0\x9f\x92\xa8",0xC300FF00,32,1},  {"Sleep","\xf0\x9f\x98\xb4",0xC3003FC0,32,1},
  {"Swing","\xe2\x86\x95",0xC300DF20,32,1},
};

// ╔══════════════════════════════════════════════════════════╗
// ║  Projectors & Other                                     ║
// ╚══════════════════════════════════════════════════════════╝

IRButton epsonProjBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x0002,8,1},  {"Source","\xe2\x8e\x98",0x006B,8,1},
  {"Menu","\xe2\x98\xb0",0x006E,8,1},   {"OK","\xe2\x9c\x94",0x006C,8,1},
  {"Up","\xe2\x96\xb2",0x0001,8,1},     {"Down","\xe2\x96\xbc",0x0003,8,1},
  {"Left","\xe2\x97\x80",0x0004,8,1},   {"Right","\xe2\x96\xb6",0x0005,8,1},
  {"Vol+","\xf0\x9f\x94\x8a",0x001C,8,1},  {"Vol-","\xf0\x9f\x94\x89",0x001D,8,1},
  {"Freeze","\xe2\x9d\x84",0x0083,8,1}, {"Blank","\xe2\xac\x9b",0x001E,8,1},
  {"HDMI","H",0x00EF,8,1},   {"Zoom+","\xf0\x9f\x94\x8d",0x0007,8,1},
  {"Zoom-","\xf0\x9f\x94\x8e",0x0008,8,1},
};

IRButton benqProjBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xD728,16,1}, {"Source","\xe2\x8e\x98",0xD7F8,16,1},
  {"Menu","\xe2\x98\xb0",0xD710,16,1},  {"OK","\xe2\x9c\x94",0xD7A8,16,1},
  {"Up","\xe2\x96\xb2",0xD760,16,1},    {"Down","\xe2\x96\xbc",0xD7E0,16,1},
  {"Left","\xe2\x97\x80",0xD7C8,16,1},  {"Right","\xe2\x96\xb6",0xD748,16,1},
  {"Vol+","\xf0\x9f\x94\x8a",0xD700,16,1}, {"Vol-","\xf0\x9f\x94\x89",0xD780,16,1},
  {"Mute","\xf0\x9f\x94\x87",0xD7B8,16,1}, {"Freeze","\xe2\x9d\x84",0xD708,16,1},
  {"Blank","\xe2\xac\x9b",0xD7D0,16,1},{"HDMI","H",0xD7B0,16,1},
  {"Aspect","\xe2\xac\x9c",0xD738,16,1},
};

IRButton xiaomiProjBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x57E3A05F,32,1}, {"Source","\xe2\x8e\x98",0x57E3807F,32,1},
  {"Menu","\xe2\x98\xb0",0x57E3E01F,32,1},  {"OK","\xe2\x9c\x94",0x57E37887,32,1},
  {"Up","\xe2\x96\xb2",0x57E302FD,32,1},    {"Down","\xe2\x96\xbc",0x57E3827D,32,1},
  {"Left","\xe2\x97\x80",0x57E3E21D,32,1},  {"Right","\xe2\x96\xb6",0x57E3629D,32,1},
  {"Vol+","\xf0\x9f\x94\x8a",0x57E350AF,32,1}, {"Vol-","\xf0\x9f\x94\x89",0x57E3D02F,32,1},
  {"Mute","\xf0\x9f\x94\x87",0x57E3906F,32,1}, {"Back","\xe2\x86\xa9",0x57E3A25D,32,1},
  {"Home","\xf0\x9f\x8f\xa0",0x57E3C03F,32,1},
};

IRButton setTopBoxBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x4B36906F,32,1}, {"CH+","\xe2\xac\x86",0x4B3600FF,32,1},
  {"CH-","\xe2\xac\x87",0x4B36807F,32,1},  {"Vol+","\xf0\x9f\x94\x8a",0x4B3640BF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x4B36C03F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x4B36906F,32,1},
  {"OK","\xe2\x9c\x94",0x4B3644BB,32,1},    {"Up","\xe2\x96\xb2",0x4B3602FD,32,1},
  {"Down","\xe2\x96\xbc",0x4B36827D,32,1},  {"Left","\xe2\x97\x80",0x4B36E01F,32,1},
  {"Right","\xe2\x96\xb6",0x4B36609F,32,1}, {"Menu","\xe2\x98\xb0",0x4B36C23D,32,1},
  {"Back","\xe2\x86\xa9",0x4B3614EB,32,1},  {"Info","\xe2\x84\xb9",0x4B3615EA,32,1},
};

IRButton dvdPlayerBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x6B8640BF,32,1}, {"Play","\xe2\x96\xb6",0x6B8602FD,32,1},
  {"Pause","\xe2\x8f\xb8",0x6B86827D,32,1}, {"Stop","\xe2\x8f\xb9",0x6B86E01F,32,1},
  {"Prev","\xe2\x8f\xae",0x6B86609F,32,1},  {"Next","\xe2\x8f\xad",0x6B8620DF,32,1},
  {"FFwd","\xe2\x8f\xa9",0x6B86A05F,32,1},  {"RWnd","\xe2\x8f\xaa",0x6B86C03F,32,1},
  {"Vol+","\xf0\x9f\x94\x8a",0x6B8650AF,32,1}, {"Vol-","\xf0\x9f\x94\x89",0x6B86D02F,32,1},
  {"Mute","\xf0\x9f\x94\x87",0x6B86906F,32,1}, {"Menu","\xe2\x98\xb0",0x6B8658A7,32,1},
  {"Open","\xe2\x8f\x8f",0x6B8610EF,32,1},
};

IRButton avReceiverBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0x7A856897,32,1}, {"Vol+","\xf0\x9f\x94\x8a",0x7A8540BF,32,1},
  {"Vol-","\xf0\x9f\x94\x89",0x7A85C03F,32,1}, {"Mute","\xf0\x9f\x94\x87",0x7A85906F,32,1},
  {"HDMI1","\xe2\x91\xa0",0x7A85E21D,32,1}, {"HDMI2","\xe2\x91\xa1",0x7A85629D,32,1},
  {"BT","\xf0\x9f\x94\xb5",0x7A8502FD,32,1},   {"USB","\xf0\x9f\x92\xbe",0x7A85827D,32,1},
  {"Optical","\xf0\x9f\x94\x86",0x7A85A25D,32,1},{"AUX","\xf0\x9f\x8e\xb5",0x7A85C23D,32,1},
  {"EQ+","\xe2\x99\xaa",0x7A8500FF,32,1},   {"EQ-","\xe2\x99\xa9",0x7A85807F,32,1},
  {"Bass+","\xf0\x9f\x94\x88",0x7A8548B7,32,1},
};

IRButton fanBtns[] PROGMEM = {
  {"Power","\xe2\x8f\xbb",0xFE01FC03,32,1}, {"Speed 1","1",0xFE0110EF,32,1},
  {"Speed 2","2",0xFE0190EF,32,1},{"Speed 3","3",0xFE0150AF,32,1},
  {"Timer","\xe2\x8f\xb0",0xFE0140BF,32,1}, {"Oscillate","\xe2\x86\x94",0xFE0120DF,32,1},
  {"Sleep","\xf0\x9f\x98\xb4",0xFE01A05F,32,1}, {"Natural","\xf0\x9f\x8c\xbf",0xFE01C03F,32,1},
};

// ╔══════════════════════════════════════════════════════════╗
// ║  Master device list (PROGMEM)                           ║
// ╚══════════════════════════════════════════════════════════╝

const DeviceIR devices[] PROGMEM = {
  // ── International TVs ──
  {"samsung_tv",   "Samsung TV",    "tv", "Samsung",   "TV", SAMSUNG,        samsungTVBtns,     17},
  {"lg_tv",        "LG TV",         "tv", "LG",        "TV", NEC,            lgTVBtns,          16},
  {"sony_tv",      "Sony TV",       "tv", "Sony",      "TV", SONY,           sonyTVBtns,        14},
  {"philips_tv",   "Philips TV",    "tv", "Philips",   "TV", RC5,            philipsTVBtns,     13},
  {"philips_tv_rc6","Philips TV RC6","tv","Philips",   "TV", RC6,            philipsRC6TVBtns,  14},
  {"tcl_tv_nec",   "TCL TV (NEC)",  "tv", "TCL",       "TV", NEC,            tclTVBtns,         16},
  {"tcl_tv_rca",   "TCL TV (RCA)",  "tv", "TCL",       "TV", NEC,            tclTVRcaBtns,      14},
  {"hisense_tv",   "Hisense TV",    "tv", "Hisense",   "TV", NEC,            hisenseTVBtns,     14},
  {"xiaomi_tv",    "Xiaomi TV",     "tv", "Xiaomi",    "TV", NEC,            xiaomiTVBtns,      15},
  {"panasonic_tv", "Panasonic TV",  "tv", "Panasonic", "TV", PANASONIC,      panasonicTVBtns,   15},

  // ── Iranian TVs ──
  {"snowa_tv",     "Snowa TV",      "tv", "Snowa",     "TV", NEC,            snowaTVBtns,       14},
  {"pakshoma_tv",  "Pakshoma TV",   "tv", "Pakshoma",  "TV", NEC,            pakshomaTVBtns,    13},
  {"parskhazar",   "Pars Khazar TV","tv", "Pars Khazar","TV", NEC,           parsKhazarTVBtns,  10},
  {"xvision_tv",   "XVision TV",    "tv", "XVision",   "TV", SAMSUNG,        xvisionTVBtns,     14},
  {"gplus_tv",     "G-Plus TV",     "tv", "G-Plus",    "TV", NEC,            gplusTVBtns,       13},
  {"emersan_tv",   "Emersan TV",    "tv", "Emersan",   "TV", SAMSUNG,        emersanTVBtns,     11},
  {"daewoo_tv",    "Daewoo TV",     "tv", "Daewoo",    "TV", NEC,            daewooTVBtns,      11},

  // ── International ACs ──
  {"samsung_ac",   "Samsung AC",    "ac", "Samsung",   "AC", SAMSUNG,        samsungACBtns,     11},
  {"lg_ac",        "LG AC",         "ac", "LG",        "AC", NEC,            lgACBtns,          12},
  {"panasonic_ac", "Panasonic AC",  "ac", "Panasonic", "AC", PANASONIC,      panasonicACBtns,   12},
  // Daikin AC — removed: uses IRDaikin2 stateful protocol, not representable as static hex
  // Mitsubishi AC — removed: uses IRMitsubishiAC stateful protocol, not representable as static hex
  {"gree_ac",      "Gree AC",       "ac", "Gree",      "AC", NEC,            greeACBtns,        12},
  {"midea_ac",     "Midea AC",      "ac", "Midea",     "AC", NEC,            mideaACBtns,       10},

  // ── Iranian ACs ──
  {"snowa_ac",     "Snowa AC",      "ac", "Snowa",     "AC", NEC,            snowaACBtns,       12},
  {"entekhab_ac",  "Entekhab AC",   "ac", "Entekhab",  "AC", NEC,            entekhabACBtns,    11},
  {"depoo_ac",     "Depoo AC",      "ac", "Depoo",     "AC", NEC,            depooACBtns,       11},
  {"electrostil_ac","Electrostil AC","ac","Electrostil","AC", NEC,           electrostilACBtns,  9},
  {"pakshoma_ac",  "Pakshoma AC",   "ac", "Pakshoma",  "AC", NEC,            pakshomaACBtns,    11},

  // ── Other ──
  {"epson_proj",   "Epson Proj",    "other","Epson",   "Projector", NEC, epsonProjBtns,       15},
  {"benq_proj",    "BenQ Proj",     "other","BenQ",    "Projector", NEC, benqProjBtns,        15},
  {"xiaomi_proj",  "Xiaomi Proj",   "other","Xiaomi",  "Projector", NEC, xiaomiProjBtns,      13},
  {"settop",       "Set-Top Box",   "other","Generic",  "STB",      NEC, setTopBoxBtns,       14},
  {"dvd",          "DVD Player",    "other","Generic",  "DVD",      NEC, dvdPlayerBtns,       13},
  {"av_receiver",  "A/V Receiver",  "other","Generic",  "A/V",      NEC, avReceiverBtns,      13},
  {"fan",          "Smart Fan",     "other","Generic",  "Fan",      NEC, fanBtns,              8},
};

const uint8_t DEVICE_COUNT = sizeof(devices) / sizeof(devices[0]);
