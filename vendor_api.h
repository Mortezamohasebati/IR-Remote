#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

class VendorAPIClient {
private:
  Preferences prefs;

  String getBaseUrl() {
    prefs.begin("vendor", true);
    String url = prefs.getString("base_url", "https://iot.example.com/api/v1");
    prefs.end();
    return url;
  }

  String getApiKey() {
    prefs.begin("vendor", true);
    String key = prefs.getString("api_key", "");
    prefs.end();
    return key;
  }

  bool httpPost(const String& path, const String& body, String& response,
                const String& extraHeader = "") {
    if (!isWiFiConnected()) return false;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = getBaseUrl() + path;
    http.begin(client, url);
    http.setTimeout(5000);
    http.addHeader("X-API-KEY", getApiKey());
    http.addHeader("Content-Type", "application/json");
    if (extraHeader.length() > 0) {
      int colon = extraHeader.indexOf(':');
      if (colon > 0) {
        http.addHeader(extraHeader.substring(0, colon), extraHeader.substring(colon + 1));
      }
    }

    int retries = 0;
    const int maxRetries = 3;
    int httpCode = 0;

    while (retries < maxRetries) {
      httpCode = http.POST(body);
      if (httpCode > 0) break;
      retries++;
      if (retries < maxRetries) delay(1000 * retries);
    }

    response = http.getString();
    http.end();
    return httpCode >= 200 && httpCode < 300;
  }

  bool httpGet(const String& path, String& response) {
    if (!isWiFiConnected()) return false;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = getBaseUrl() + path;
    http.begin(client, url);
    http.setTimeout(5000);
    http.addHeader("X-API-KEY", getApiKey());

    int retries = 0;
    const int maxRetries = 3;
    int httpCode = 0;

    while (retries < maxRetries) {
      httpCode = http.GET();
      if (httpCode > 0) break;
      retries++;
      if (retries < maxRetries) delay(1000 * retries);
    }

    response = http.getString();
    http.end();
    return httpCode >= 200 && httpCode < 300;
  }

public:
  // ── Config ──────────────────────────────────────────────
  void saveConfig(const String& baseUrl, const String& apiKey,
                  int vendorId, int productId, const String& serial) {
    prefs.begin("vendor", false);
    prefs.putString("base_url", baseUrl);
    prefs.putString("api_key", apiKey);
    prefs.putInt("vendor_id", vendorId);
    prefs.putInt("product_id", productId);
    prefs.putString("serial", serial);
    prefs.end();
  }

  String getConfigJSON() {
    prefs.begin("vendor", true);
    String json = "{";
    json += "\"base_url\":\"" + prefs.getString("base_url", "") + "\",";
    json += "\"api_key\":\"" + prefs.getString("api_key", "") + "\",";
    json += "\"vendor_id\":" + String(prefs.getInt("vendor_id", 0)) + ",";
    json += "\"product_id\":" + String(prefs.getInt("product_id", 0)) + ",";
    json += "\"serial\":\"" + prefs.getString("serial", "") + "\"";
    json += "}";
    prefs.end();
    return json;
  }

  // ── POST /api/v1/vendor/manufactured/ ───────────────────
  bool registerDevice(const String& serial, int productId, String& tokenOut) {
    String body = "{\"serial\":\"" + serial + "\",\"product\":" + String(productId) + "}";
    String resp;
    if (!httpPost("/vendor/manufactured/", body, resp)) return false;

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) return false;
    tokenOut = doc["token"].as<String>();
    return true;
  }

  // ── POST /api/v1/device/register/ ───────────────────────
  bool deviceRegister(const String& uniqueId, const String& serial,
                      int vendorId, String& deviceTokenOut) {
    String body = "{\"unique_id\":\"" + uniqueId + "\",\"serial\":\"" +
                  serial + "\",\"vendor\":" + String(vendorId) + "}";
    String resp;
    if (!httpPost("/device/register/", body, resp)) return false;

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) return false;

    deviceTokenOut = doc["token"].as<String>();
    // Optionally store auth_code for display
    String authCode = doc["auth_code"].as<String>();
    if (authCode.length() > 0) {
      prefs.begin("vendor", false);
      prefs.putString("auth_code", authCode);
      prefs.end();
    }

    prefs.begin("vendor", false);
    prefs.putString("device_token", deviceTokenOut);
    prefs.end();
    return true;
  }

  // ── POST /api/v1/device/relay/command/ ──────────────────
  bool sendRelayCommand(const String& deviceToken, uint8_t relay, bool state) {
    String body = "{\"device_id\":\"" + deviceToken + "\",\"relay_key\":\"relay_" +
                  String(relay + 1) + "\",\"status\":" + String(state ? "true" : "false") + "}";
    String resp;
    return httpPost("/device/relay/command/", body, resp);
  }

  // ── GET /api/v1/device/relay/status/ ───────────────────
  bool getRelayStatus(const String& deviceToken, JsonArray& out) {
    String resp;
    if (!httpGet("/device/relay/status/?device_id=" + deviceToken, resp)) return false;

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) return false;
    out = doc["state"].as<JsonArray>();
    return true;
  }

  // ── GET /api/v1/vendor/credit/balance/ ─────────────────
  float getCreditBalance() {
    String resp;
    if (!httpGet("/vendor/credit/balance/", resp)) return 0;
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) return 0;
    return doc["balance"].as<float>();
  }

  // ── WebSocket stubs ─────────────────────────────────────
  // To use WebSocket, install 'arun11299/cppWebSocket' or 'Links2004/WebSockets'
  // library and implement:
  //
  // void connectWebSocket() {
  //   // ws://domain/ws/vendor/ with header X-API-KEY
  //   // Handle: relay_command, status_request, pair_confirmed, heartbeat
  // }
  //
  // void loopWebSocket() {
  //   // Call from main loop, keepalive + message dispatch
  // }

  bool isConfigured() {
    return getApiKey().length() > 0;
  }
};

// ── Global instance defined in main.ino, declared in globals.h ──
