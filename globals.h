#pragma once
#include <IRsend.h>
#include <IRrecv.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Arduino.h>

class SmartHomeClient;
class VendorAPIClient;

extern SmartHomeClient shClient;
extern VendorAPIClient vendorAPI;

extern WebServer server;
extern Preferences prefs;

extern IRsend irsend;
extern IRrecv irrecv;

extern bool learnMode;
extern bool learnDone;
extern String learnDevice;
extern String learnBtn;
extern uint32_t learnStart;
extern const uint32_t LEARN_TIMEOUT;

extern String lastRxDevice;
extern String lastRxBtn;

extern char csrfToken[17];
