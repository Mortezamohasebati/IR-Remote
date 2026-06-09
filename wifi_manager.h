#pragma once
#include <WiFi.h>
#include <Preferences.h>

const char* AP_SSID     = "IR-Remote";
const char* AP_PASSWORD = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GW(192, 168, 4, 1);
const IPAddress AP_MASK(255, 255, 255, 0);

const uint32_t STA_RETRY_MS  = 30000;
const uint32_t STA_CHECK_MS  = 5000;

struct WiFiConfig {
  String ssid;
  String password;
  bool   configured;
};

WiFiConfig loadWiFiConfig() {
  Preferences p;
  p.begin("wifi", true);
  WiFiConfig cfg;
  cfg.ssid       = p.getString("ssid", "");
  cfg.password   = p.getString("pass", "");
  cfg.configured = p.isKey("ssid") && cfg.ssid.length() > 0;
  p.end();
  return cfg;
}

void saveWiFiConfig(const String& ssid, const String& password) {
  Preferences p;
  p.begin("wifi", false);
  p.putString("ssid", ssid);
  p.putString("pass", password);
  p.end();
}

void clearWiFiConfig() {
  Preferences p;
  p.begin("wifi", false);
  p.remove("ssid");
  p.remove("pass");
  p.end();
}

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[WiFi] AP: %s | IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  WiFiConfig cfg = loadWiFiConfig();
  if (cfg.configured) {
    Serial.printf("[WiFi] Connecting to STA: %s\n", cfg.ssid.c_str());
    WiFi.begin(cfg.ssid.c_str(), cfg.password.c_str());
    WiFi.setAutoReconnect(true);
  }
}

void loopWiFi() {
  static uint32_t lastCheck = 0;
  WiFiConfig cfg = loadWiFiConfig();
  if (!cfg.configured) return;

  uint32_t now = millis();
  if (now - lastCheck < STA_CHECK_MS) return;
  lastCheck = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] STA disconnected, reconnecting...");
    WiFi.reconnect();
  }
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifiStatusJSON() {
  WiFiConfig cfg = loadWiFiConfig();
  String json = "{";
  json += "\"ap\":\"" + String(AP_SSID) + "\",";
  json += "\"sta_configured\":" + String(cfg.configured ? "true" : "false") + ",";
  json += "\"sta_connected\":" + String(isWiFiConnected() ? "true" : "false") + ",";
  json += "\"sta_ssid\":\"" + cfg.ssid + "\",";
  json += "\"ip\":\"" + (isWiFiConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\"";
  json += "}";
  return json;
}
