// =============================================================
//  ir_codes.h  —  کتابخانه کدهای IR
//  نسخه ۲.۰ — با پشتیبانی از یادگیری کد جدید
// =============================================================

#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ── پروتکل‌ها ─────────────────────────────────────────────────
enum IRProtocol {
  PROTO_NEC,
  PROTO_SAMSUNG,
  PROTO_SONY,
  PROTO_RC5,
  PROTO_LG,
  PROTO_SHARP,
  PROTO_PANASONIC,
  PROTO_PHILIPS,
  PROTO_UNKNOWN
};

// ── دکمه ─────────────────────────────────────────────────────
struct IRButton {
  const char* name;
  const char* icon;
  uint64_t    code;
  uint8_t     bits;
  uint8_t     repeat;
};

// ── دستگاه ───────────────────────────────────────────────────
struct DeviceIR {
  const char* id;
  const char* name;
  const char* category;   // "tv" | "ac" | "other"
  const char* brand;
  const char* catLabel;   // نام فارسی
  IRProtocol  protocol;
  IRButton*   buttons;
  uint8_t     btnCount;
};


// ╔══════════════════════════════════════════════════════════╗
// ║  تلویزیون‌ها — خارجی                                    ║
// ╚══════════════════════════════════════════════════════════╝

IRButton samsungTVBtns[] = {
  {"Power","⏻",0xE0E040BF,32,1}, {"Vol+","🔊",0xE0E0E01F,32,1},
  {"Vol-","🔉",0xE0E0D02F,32,1}, {"Mute","🔇",0xE0E0F00F,32,1},
  {"CH+","⬆",0xE0E048B7,32,1},  {"CH-","⬇",0xE0E008F7,32,1},
  {"Home","🏠",0xE0E09E61,32,1}, {"Menu","☰",0xE0E058A7,32,1},
  {"OK","✔",0xE0E016E9,32,1},   {"Up","▲",0xE0E006F9,32,1},
  {"Down","▼",0xE0E08679,32,1}, {"Left","◀",0xE0E0A659,32,1},
  {"Right","▶",0xE0E046B9,32,1},{"Back","↩",0xE0E01AE5,32,1},
  {"Source","⎘",0xE0E0807F,32,1},{"Netflix","N",0xE0E031CE,32,1},
};

IRButton lgTVBtns[] = {
  {"Power","⏻",0x20DF10EF,32,1}, {"Vol+","🔊",0x20DF40BF,32,1},
  {"Vol-","🔉",0x20DFC03F,32,1}, {"Mute","🔇",0x20DF906F,32,1},
  {"CH+","⬆",0x20DF00FF,32,1},  {"CH-","⬇",0x20DF807F,32,1},
  {"Home","🏠",0x20DFBC43,32,1}, {"Menu","☰",0x20DFC23D,32,1},
  {"OK","✔",0x20DF44BB,32,1},   {"Up","▲",0x20DF02FD,32,1},
  {"Down","▼",0x20DF827D,32,1}, {"Left","◀",0x20DFE01F,32,1},
  {"Right","▶",0x20DF609F,32,1},{"Back","↩",0x20DF14EB,32,1},
  {"Source","⎘",0x20DFD02F,32,1},
};

IRButton sonyTVBtns[] = {
  {"Power","⏻",0xA90,12,3},  {"Vol+","🔊",0x490,12,3},
  {"Vol-","🔉",0xC90,12,3},  {"Mute","🔇",0x290,12,3},
  {"CH+","⬆",0x090,12,3},   {"CH-","⬇",0x890,12,3},
  {"Home","🏠",0x6D90,15,3}, {"Menu","☰",0x5A90,15,3},
  {"OK","✔",0xA590,15,3},   {"Up","▲",0x2F90,12,3},
  {"Down","▼",0xAF90,12,3}, {"Left","◀",0x2D90,12,3},
  {"Right","▶",0xCD90,12,3},{"Back","↩",0x6590,15,3},
};

IRButton philipsTVBtns[] = {
  {"Power","⏻",0x010C,13,1}, {"Vol+","🔊",0x0110,13,1},
  {"Vol-","🔉",0x0111,13,1}, {"Mute","🔇",0x010D,13,1},
  {"CH+","⬆",0x0120,13,1},  {"CH-","⬇",0x0121,13,1},
  {"Home","🏠",0x0164,13,1}, {"Menu","☰",0x0112,13,1},
  {"OK","✔",0x010F,13,1},   {"Up","▲",0x0110,13,1},
  {"Down","▼",0x0111,13,1}, {"Back","↩",0x010C,13,1},
  {"Source","⎘",0x0138,13,1},
};

IRButton xiaomiTVBtns[] = {
  {"Power","⏻",0x57E3A05F,32,1}, {"Vol+","🔊",0x57E350AF,32,1},
  {"Vol-","🔉",0x57E3D02F,32,1}, {"Mute","🔇",0x57E3906F,32,1},
  {"CH+","⬆",0x57E300FF,32,1},  {"CH-","⬇",0x57E3807F,32,1},
  {"Home","🏠",0x57E3C03F,32,1}, {"Menu","☰",0x57E3E01F,32,1},
  {"OK","✔",0x57E37887,32,1},   {"Up","▲",0x57E302FD,32,1},
  {"Down","▼",0x57E3827D,32,1}, {"Left","◀",0x57E3E21D,32,1},
  {"Right","▶",0x57E3629D,32,1},{"Back","↩",0x57E3A25D,32,1},
};

IRButton panasonicTVBtns[] = {
  {"Power","⏻",0x400401,22,1}, {"Vol+","🔊",0x400425,22,1},
  {"Vol-","🔉",0x400424,22,1}, {"Mute","🔇",0x400409,22,1},
  {"CH+","⬆",0x400434,22,1},  {"CH-","⬇",0x400435,22,1},
  {"Home","🏠",0x40043D,22,1}, {"Menu","☰",0x40040E,22,1},
  {"OK","✔",0x400449,22,1},   {"Up","▲",0x400458,22,1},
  {"Down","▼",0x400459,22,1}, {"Left","◀",0x40045A,22,1},
  {"Right","▶",0x40045B,22,1},{"Back","↩",0x40046C,22,1},
  {"Source","⎘",0x40044C,22,1},
};

// ── تلویزیون‌های ایرانی ──────────────────────────────────────

IRButton snowaTVBtns[] = {
  {"Power","⏻",0x807F48B7,32,1}, {"Vol+","🔊",0x807F50AF,32,1},
  {"Vol-","🔉",0x807FD02F,32,1}, {"Mute","🔇",0x807F906F,32,1},
  {"CH+","⬆",0x807F00FF,32,1},  {"CH-","⬇",0x807F807F,32,1},
  {"Menu","☰",0x807FA25D,32,1}, {"OK","✔",0x807F22DD,32,1},
  {"Up","▲",0x807FC23D,32,1},   {"Down","▼",0x807F827D,32,1},
  {"Left","◀",0x807FE21D,32,1}, {"Right","▶",0x807F629D,32,1},
  {"Source","⎘",0x807FA05F,32,1},{"Back","↩",0x807FF00F,32,1},
};

IRButton pakshomaTVBtns[] = {
  {"Power","⏻",0xFF02FD,32,1}, {"Vol+","🔊",0xFF42BD,32,1},
  {"Vol-","🔉",0xFFC23D,32,1}, {"Mute","🔇",0xFF52AD,32,1},
  {"CH+","⬆",0xFF12ED,32,1},  {"CH-","⬇",0xFF9267,32,1},
  {"Menu","☰",0xFF8A75,32,1}, {"OK","✔",0xFF38C7,32,1},
  {"Up","▲",0xFF18E7,32,1},   {"Down","▼",0xFF4AB5,32,1},
  {"Left","◀",0xFF10EF,32,1}, {"Right","▶",0xFF5AA5,32,1},
  {"Source","⎘",0xFF22DD,32,1},
};

IRButton parsKhazarTVBtns[] = {
  {"Power","⏻",0xFF02FD,32,1}, {"Vol+","🔊",0xFF52AD,32,1},
  {"Vol-","🔉",0xFF4AB5,32,1}, {"Mute","🔇",0xFF08F7,32,1},
  {"CH+","⬆",0xFF18E7,32,1},  {"CH-","⬇",0xFF10EF,32,1},
  {"Menu","☰",0xFF38C7,32,1}, {"OK","✔",0xFF5AA5,32,1},
  {"Source","⎘",0xFF22DD,32,1},{"Back","↩",0xFFD02F,32,1},
};

IRButton xvisionTVBtns[] = {
  {"Power","⏻",0xE0E040BF,32,1}, {"Vol+","🔊",0xE0E0E01F,32,1},
  {"Vol-","🔉",0xE0E0D02F,32,1}, {"Mute","🔇",0xE0E0F00F,32,1},
  {"CH+","⬆",0xE0E048B7,32,1},  {"CH-","⬇",0xE0E008F7,32,1},
  {"Menu","☰",0xE0E058A7,32,1}, {"OK","✔",0xE0E016E9,32,1},
  {"Up","▲",0xE0E006F9,32,1},   {"Down","▼",0xE0E08679,32,1},
  {"Left","◀",0xE0E0A659,32,1}, {"Right","▶",0xE0E046B9,32,1},
  {"Source","⎘",0xE0E0807F,32,1},{"Back","↩",0xE0E01AE5,32,1},
};

IRButton gplusTVBtns[] = {
  {"Power","⏻",0x20DF10EF,32,1}, {"Vol+","🔊",0x20DF40BF,32,1},
  {"Vol-","🔉",0x20DFC03F,32,1}, {"Mute","🔇",0x20DF906F,32,1},
  {"CH+","⬆",0x20DF00FF,32,1},  {"CH-","⬇",0x20DF807F,32,1},
  {"Menu","☰",0x20DFC23D,32,1}, {"OK","✔",0x20DF44BB,32,1},
  {"Up","▲",0x20DF02FD,32,1},   {"Down","▼",0x20DF827D,32,1},
  {"Left","◀",0x20DFE01F,32,1}, {"Right","▶",0x20DF609F,32,1},
  {"Source","⎘",0x20DFD02F,32,1},
};

IRButton emersanTVBtns[] = {
  {"Power","⏻",0xCF3040BF,32,1}, {"Vol+","🔊",0xCF30E01F,32,1},
  {"Vol-","🔉",0xCF30D02F,32,1}, {"Mute","🔇",0xCF30906F,32,1},
  {"CH+","⬆",0xCF3048B7,32,1},  {"CH-","⬇",0xCF3008F7,32,1},
  {"Menu","☰",0xCF3058A7,32,1}, {"OK","✔",0xCF3016E9,32,1},
  {"Up","▲",0xCF3006F9,32,1},   {"Down","▼",0xCF308679,32,1},
  {"Source","⎘",0xCF30807F,32,1},
};

IRButton daewooTVBtns[] = {
  {"Power","⏻",0x827D48B7,32,1}, {"Vol+","🔊",0x827D50AF,32,1},
  {"Vol-","🔉",0x827DD02F,32,1}, {"Mute","🔇",0x827D10EF,32,1},
  {"CH+","⬆",0x827D00FF,32,1},  {"CH-","⬇",0x827D807F,32,1},
  {"Menu","☰",0x827DA05F,32,1}, {"OK","✔",0x827D22DD,32,1},
  {"Up","▲",0x827DC23D,32,1},   {"Down","▼",0x827D827D,32,1},
  {"Source","⎘",0x827DA25D,32,1},
};


// ╔══════════════════════════════════════════════════════════╗
// ║  کولر گازی — خارجی                                      ║
// ╚══════════════════════════════════════════════════════════╝

IRButton samsungACBtns[] = {
  {"Power","⏻",0x5480000000LL,48,1},  {"Cool","❄",0x5480040090LL,48,1},
  {"Heat","🔥",0x5480040098LL,48,1},  {"Fan","💨",0x5480040094LL,48,1},
  {"Auto","♻",0x5480040092LL,48,1},   {"Temp+","🌡",0x5481040090LL,48,1},
  {"Temp-","🌡",0x5482040090LL,48,1}, {"Fan+","💨",0x5480050090LL,48,1},
  {"Fan-","💨",0x5480060090LL,48,1},  {"Swing","↕",0x5480040490LL,48,1},
  {"Sleep","😴",0x5480240090LL,48,1},
};

IRButton lgACBtns[] = {
  {"Power","⏻",0x88000B70,32,1},  {"Cool 18°","❄",0x880002A4,32,1},
  {"Cool 20°","❄",0x880000A4,32,1},{"Cool 22°","❄",0x880012A4,32,1},
  {"Cool 24°","❄",0x880022A4,32,1},{"Cool 26°","❄",0x880032A4,32,1},
  {"Heat","🔥",0x880002A0,32,1},  {"Fan Low","💨",0x88000240,32,1},
  {"Fan Med","💨",0x88000280,32,1},{"Fan Hi","💨",0x880002C0,32,1},
  {"Swing","↕",0x88C302A4,32,1},  {"Sleep","😴",0x88000EA4,32,1},
};

IRButton panasonicACBtns[] = {
  {"Power","⏻",0x340034002000LL,48,1},
  {"Cool","❄",0x340034003000LL,48,1},
  {"Heat","🔥",0x340034004000LL,48,1},
  {"Auto","♻",0x340034001000LL,48,1},
  {"Dry","💧",0x340034002400LL,48,1},
  {"Fan Low","💨",0x340034002100LL,48,1},
  {"Fan Med","💨",0x340034002200LL,48,1},
  {"Fan Hi","💨",0x340034002300LL,48,1},
  {"Temp+","🌡",0x340034002001LL,48,1},
  {"Temp-","🌡",0x340034002002LL,48,1},
  {"Swing","↕",0x340034002010LL,48,1},
  {"Sleep","😴",0x340034002020LL,48,1},
};

IRButton daikinACBtns[] = {
  {"Power","⏻",0x1100100,28,1},
  {"Cool","❄",0x1100110,28,1},
  {"Heat","🔥",0x1100140,28,1},
  {"Fan","💨",0x1100160,28,1},
  {"Dry","💧",0x1100120,28,1},
  {"Temp+","🌡",0x1100101,28,1},
  {"Temp-","🌡",0x1100102,28,1},
  {"Fan+","💨",0x1100104,28,1},
  {"Fan-","💨",0x1100108,28,1},
  {"Swing","↕",0x1100150,28,1},
  {"Sleep","😴",0x1100180,28,1},
};

IRButton mitsubishiACBtns[] = {
  {"Power","⏻",0x4FB7080F7,34,1},
  {"Cool","❄",0x4FB708AF7,34,1},
  {"Heat","🔥",0x4FB7098F7,34,1},
  {"Fan","💨",0x4FB70A0F7,34,1},
  {"Dry","💧",0x4FB70B8F7,34,1},
  {"Temp+","🌡",0x4FB7010EF,34,1},
  {"Temp-","🌡",0x4FB7090F7,34,1},
  {"Fan+","💨",0x4FB7040BF,34,1},
  {"Fan-","💨",0x4FB70C0F3,34,1},
  {"Swing","↕",0x4FB7038C7,34,1},
};

IRButton greeACBtns[] = {
  {"Power","⏻",0x9B25F00000LL,48,1},  {"Cool 18°","❄",0x9B09F00000LL,48,1},
  {"Cool 20°","❄",0x9B29F00000LL,48,1},{"Cool 22°","❄",0x9B49F00000LL,48,1},
  {"Cool 24°","❄",0x9B69F00000LL,48,1},{"Cool 26°","❄",0x9B89F00000LL,48,1},
  {"Heat","🔥",0x9B09F04000LL,48,1},  {"Dry","💧",0x9B09F02000LL,48,1},
  {"Fan Auto","💨",0x9B09F00400LL,48,1},{"Swing","↕",0x9B09F00200LL,48,1},
  {"Sleep","😴",0x9B09F08000LL,48,1}, {"Turbo","⚡",0x9B09F00010LL,48,1},
};

// ── کولر گازی ایرانی ──────────────────────────────────────────

IRButton snowaACBtns[] = {
  {"Power","⏻",0xB24DBF40,32,1},  {"Cool","❄",0xB24D9F60,32,1},
  {"Heat","🔥",0xB24D1FE0,32,1},  {"Fan","💨",0xB24D7F80,32,1},
  {"Auto","♻",0xB24DEF10,32,1},   {"Dry","💧",0xB24D5FA0,32,1},
  {"Temp+","🌡",0xB24DCF30,32,1}, {"Temp-","🌡",0xB24D4FB0,32,1},
  {"Fan+","💨",0xB24D6F90,32,1},  {"Fan-","💨",0xB24DFF00,32,1},
  {"Sleep","😴",0xB24D3FC0,32,1}, {"Swing","↕",0xB24DDF20,32,1},
};

IRButton entekhabACBtns[] = {
  {"Power","⏻",0xE0F00AFF,32,1},  {"Cool","❄",0xE0F018E7,32,1},
  {"Heat","🔥",0xE0F048B7,32,1},  {"Fan","💨",0xE0F028D7,32,1},
  {"Dry","💧",0xE0F088F7,32,1},   {"Temp+","🌡",0xE0F058A7,32,1},
  {"Temp-","🌡",0xE0F038C7,32,1}, {"Fan+","💨",0xE0F068B7,32,1},
  {"Fan-","💨",0xE0F008F7,32,1},  {"Sleep","😴",0xE0F078B7,32,1},
  {"Swing","↕",0xE0F0C837,32,1},
};

IRButton depooACBtns[] = {
  {"Power","⏻",0xF30C40BF,32,1},  {"Cool","❄",0xF30C9F60,32,1},
  {"Heat","🔥",0xF30C1FE0,32,1},  {"Auto","♻",0xF30CEF10,32,1},
  {"Dry","💧",0xF30C5FA0,32,1},   {"Temp+","🌡",0xF30CCF30,32,1},
  {"Temp-","🌡",0xF30C4FB0,32,1}, {"Fan+","💨",0xF30C6F90,32,1},
  {"Fan-","💨",0xF30CFF00,32,1},  {"Sleep","😴",0xF30C3FC0,32,1},
  {"Swing","↕",0xF30CDF20,32,1},
};

IRButton electrostilACBtns[] = {
  {"Power","⏻",0xA15E40BF,32,1},  {"Cool","❄",0xA15E9060,32,1},
  {"Heat","🔥",0xA15E10E0,32,1},  {"Fan","💨",0xA15E7080,32,1},
  {"Dry","💧",0xA15E5020,32,1},   {"Temp+","🌡",0xA15EC030,32,1},
  {"Temp-","🌡",0xA15E40B0,32,1}, {"Sleep","😴",0xA15E30C0,32,1},
  {"Swing","↕",0xA15ED020,32,1},
};

IRButton pakshomаACBtns[] = {
  {"Power","⏻",0xC3003CFF,32,1},  {"Cool","❄",0xC3009F60,32,1},
  {"Heat","🔥",0xC3001FE0,32,1},  {"Auto","♻",0xC300EF10,32,1},
  {"Dry","💧",0xC3005FA0,32,1},   {"Temp+","🌡",0xC300CF30,32,1},
  {"Temp-","🌡",0xC3004FB0,32,1}, {"Fan+","💨",0xC3006F90,32,1},
  {"Fan-","💨",0xC300FF00,32,1},  {"Sleep","😴",0xC3003FC0,32,1},
  {"Swing","↕",0xC300DF20,32,1},
};


// ╔══════════════════════════════════════════════════════════╗
// ║  پروژکتور / سایر / ست‌تاپ / DVD / A/V                  ║
// ╚══════════════════════════════════════════════════════════╝

IRButton epsonProjBtns[] = {
  {"Power","⏻",0x0002,8,1},  {"Source","⎘",0x006B,8,1},
  {"Menu","☰",0x006E,8,1},   {"OK","✔",0x006C,8,1},
  {"Up","▲",0x0001,8,1},     {"Down","▼",0x0003,8,1},
  {"Left","◀",0x0004,8,1},   {"Right","▶",0x0005,8,1},
  {"Vol+","🔊",0x001C,8,1},  {"Vol-","🔉",0x001D,8,1},
  {"Freeze","❄",0x0083,8,1}, {"Blank","⬛",0x001E,8,1},
  {"HDMI","H",0x00EF,8,1},   {"Zoom+","🔍",0x0007,8,1},
  {"Zoom-","🔎",0x0008,8,1},
};

IRButton benqProjBtns[] = {
  {"Power","⏻",0xD728,16,1}, {"Source","⎘",0xD7F8,16,1},
  {"Menu","☰",0xD710,16,1},  {"OK","✔",0xD7A8,16,1},
  {"Up","▲",0xD760,16,1},    {"Down","▼",0xD7E0,16,1},
  {"Left","◀",0xD7C8,16,1},  {"Right","▶",0xD748,16,1},
  {"Vol+","🔊",0xD700,16,1}, {"Vol-","🔉",0xD780,16,1},
  {"Mute","🔇",0xD7B8,16,1}, {"Freeze","❄",0xD708,16,1},
  {"Blank","⬛",0xD7D0,16,1},{"HDMI","H",0xD7B0,16,1},
  {"Aspect","⬜",0xD738,16,1},
};

IRButton xiaomiProjBtns[] = {
  {"Power","⏻",0x57E3A05F,32,1}, {"Source","⎘",0x57E3807F,32,1},
  {"Menu","☰",0x57E3E01F,32,1},  {"OK","✔",0x57E37887,32,1},
  {"Up","▲",0x57E302FD,32,1},    {"Down","▼",0x57E3827D,32,1},
  {"Left","◀",0x57E3E21D,32,1},  {"Right","▶",0x57E3629D,32,1},
  {"Vol+","🔊",0x57E350AF,32,1}, {"Vol-","🔉",0x57E3D02F,32,1},
  {"Mute","🔇",0x57E3906F,32,1}, {"Back","↩",0x57E3A25D,32,1},
  {"Home","🏠",0x57E3C03F,32,1},
};

IRButton setTopBoxBtns[] = {
  {"Power","⏻",0x4B36906F,32,1}, {"CH+","⬆",0x4B3600FF,32,1},
  {"CH-","⬇",0x4B36807F,32,1},  {"Vol+","🔊",0x4B3640BF,32,1},
  {"Vol-","🔉",0x4B36C03F,32,1}, {"Mute","🔇",0x4B36906F,32,1},
  {"OK","✔",0x4B3644BB,32,1},    {"Up","▲",0x4B3602FD,32,1},
  {"Down","▼",0x4B36827D,32,1},  {"Left","◀",0x4B36E01F,32,1},
  {"Right","▶",0x4B36609F,32,1}, {"Menu","☰",0x4B36C23D,32,1},
  {"Back","↩",0x4B3614EB,32,1},  {"Info","ℹ",0x4B3615EA,32,1},
};

IRButton dvdPlayerBtns[] = {
  {"Power","⏻",0x6B8640BF,32,1}, {"Play","▶",0x6B8602FD,32,1},
  {"Pause","⏸",0x6B86827D,32,1}, {"Stop","⏹",0x6B86E01F,32,1},
  {"Prev","⏮",0x6B86609F,32,1},  {"Next","⏭",0x6B8620DF,32,1},
  {"FFwd","⏩",0x6B86A05F,32,1},  {"RWnd","⏪",0x6B86C03F,32,1},
  {"Vol+","🔊",0x6B8650AF,32,1}, {"Vol-","🔉",0x6B86D02F,32,1},
  {"Mute","🔇",0x6B86906F,32,1}, {"Menu","☰",0x6B8658A7,32,1},
  {"Open","⏏",0x6B8610EF,32,1},
};

IRButton avReceiverBtns[] = {
  {"Power","⏻",0x7A856897,32,1}, {"Vol+","🔊",0x7A8540BF,32,1},
  {"Vol-","🔉",0x7A85C03F,32,1}, {"Mute","🔇",0x7A85906F,32,1},
  {"HDMI1","①",0x7A85E21D,32,1}, {"HDMI2","②",0x7A85629D,32,1},
  {"BT","🔵",0x7A8502FD,32,1},   {"USB","💾",0x7A85827D,32,1},
  {"Optical","🔆",0x7A85A25D,32,1},{"AUX","🎵",0x7A85C23D,32,1},
  {"EQ+","♪",0x7A8500FF,32,1},   {"EQ-","♩",0x7A85807F,32,1},
  {"Bass+","🔈",0x7A8548B7,32,1},
};

IRButton fanBtns[] = {
  {"Power","⏻",0xFE01FC03,32,1}, {"Speed 1","1",0xFE0110EF,32,1},
  {"Speed 2","2",0xFE0190EF,32,1},{"Speed 3","3",0xFE0150AF,32,1},
  {"Timer","⏰",0xFE0140BF,32,1}, {"Oscillate","↔",0xFE0120DF,32,1},
  {"Sleep","😴",0xFE01A05F,32,1}, {"Natural","🌿",0xFE01C03F,32,1},
};


// ╔══════════════════════════════════════════════════════════╗
// ║  لیست اصلی دستگاه‌ها                                    ║
// ╚══════════════════════════════════════════════════════════╝
// category: "tv" | "ac" | "other"

DeviceIR devices[] = {
  // ── تلویزیون خارجی ──
  {"samsung_tv",  "Samsung TV",    "tv", "Samsung",      "تلویزیون", PROTO_SAMSUNG,   samsungTVBtns,     16},
  {"lg_tv",       "LG TV",         "tv", "LG",           "تلویزیون", PROTO_NEC,        lgTVBtns,         15},
  {"sony_tv",     "Sony TV",       "tv", "Sony",         "تلویزیون", PROTO_SONY,       sonyTVBtns,       14},
  {"philips_tv",  "Philips TV",    "tv", "Philips",      "تلویزیون", PROTO_RC5,        philipsTVBtns,    13},
  {"xiaomi_tv",   "Xiaomi TV",     "tv", "Xiaomi",       "تلویزیون", PROTO_NEC,        xiaomiTVBtns,     14},
  {"panasonic_tv","Panasonic TV",  "tv", "Panasonic",    "تلویزیون", PROTO_PANASONIC,  panasonicTVBtns,  15},
  // ── تلویزیون ایرانی ──
  {"snowa_tv",    "اسنوا TV",      "tv", "Snowa",        "تلویزیون", PROTO_NEC,        snowaTVBtns,      14},
  {"pakshoma_tv", "پاکشوما TV",   "tv", "Pakshoma",     "تلویزیون", PROTO_NEC,        pakshomaTVBtns,   13},
  {"parskhazar",  "پارس خزر TV",  "tv", "Pars Khazar",  "تلویزیون", PROTO_NEC,        parsKhazarTVBtns, 10},
  {"xvision_tv",  "ایکس‌ویژن TV", "tv", "XVision",      "تلویزیون", PROTO_SAMSUNG,    xvisionTVBtns,    14},
  {"gplus_tv",    "جی‌پلاس TV",   "tv", "G-Plus",       "تلویزیون", PROTO_NEC,        gplusTVBtns,      13},
  {"emersan_tv",  "امرسان TV",     "tv", "Emersan",      "تلویزیون", PROTO_SAMSUNG,    emersanTVBtns,    11},
  {"daewoo_tv",   "دوو TV",        "tv", "Daewoo",       "تلویزیون", PROTO_NEC,        daewooTVBtns,     11},
  // ── کولر گازی خارجی ──
  {"samsung_ac",  "Samsung AC",    "ac", "Samsung",      "کولر گازی", PROTO_SAMSUNG,   samsungACBtns,    11},
  {"lg_ac",       "LG AC",         "ac", "LG",           "کولر گازی", PROTO_NEC,        lgACBtns,         12},
  {"panasonic_ac","Panasonic AC",  "ac", "Panasonic",    "کولر گازی", PROTO_PANASONIC,  panasonicACBtns,  12},
  {"daikin_ac",   "Daikin AC",     "ac", "Daikin",       "کولر گازی", PROTO_NEC,        daikinACBtns,     11},
  {"mitsubishi_ac","Mitsubishi AC","ac", "Mitsubishi",   "کولر گازی", PROTO_NEC,        mitsubishiACBtns, 10},
  {"gree_ac",     "Gree AC",       "ac", "Gree",         "کولر گازی", PROTO_NEC,        greeACBtns,       12},
  // ── کولر گازی ایرانی ──
  {"snowa_ac",    "اسنوا AC",      "ac", "Snowa",        "کولر گازی", PROTO_NEC,        snowaACBtns,      12},
  {"entekhab_ac", "انتخاب AC",     "ac", "Entekhab",     "کولر گازی", PROTO_NEC,        entekhabACBtns,   11},
  {"depoo_ac",    "دپو AC",        "ac", "Depoo",        "کولر گازی", PROTO_NEC,        depooACBtns,      11},
  {"electrostil_ac","الکترواستیل AC","ac","Electrostil", "کولر گازی", PROTO_NEC,        electrostilACBtns, 9},
  {"pakshoma_ac", "پاکشوما AC",    "ac", "Pakshoma",     "کولر گازی", PROTO_NEC,        pakshomаACBtns,   11},
  // ── سایر ──
  {"epson_proj",  "Epson Proj",    "other","Epson",      "پروژکتور",  PROTO_NEC,        epsonProjBtns,    15},
  {"benq_proj",   "BenQ Proj",     "other","BenQ",       "پروژکتور",  PROTO_NEC,        benqProjBtns,     15},
  {"xiaomi_proj", "Xiaomi Proj",   "other","Xiaomi",     "پروژکتور",  PROTO_NEC,        xiaomiProjBtns,   13},
  {"settop",      "ست‌تاپ باکس",   "other","Generic",   "ست‌تاپ باکس",PROTO_NEC,       setTopBoxBtns,    14},
  {"dvd",         "DVD Player",    "other","Generic",    "DVD Player", PROTO_NEC,        dvdPlayerBtns,    13},
  {"av_receiver", "A/V Receiver",  "other","Generic",    "A/V Receiver",PROTO_NEC,      avReceiverBtns,   13},
  {"fan",         "پنکه هوشمند",   "other","Generic",    "پنکه",       PROTO_NEC,        fanBtns,           8},
};

const uint8_t DEVICE_COUNT = sizeof(devices) / sizeof(devices[0]);
