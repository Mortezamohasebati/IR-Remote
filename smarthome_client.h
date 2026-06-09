#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Preferences.h>

struct SensorData {
  float temperature = NAN;
  float humidity = NAN;
  int light = -1;
  int gas = -1;
  int alarm = -1;
};

class SmartHomeClient {
private:
  String getIP() {
    Preferences p;
    p.begin("smarthome", true);
    String ip = p.getString("sh_ip", "192.168.4.1");
    p.end();
    return ip;
  }

  bool postJSON(const String& url, const String& body) {
    HTTPClient http;
    WiFiClient client;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);
    int code = http.POST(body);
    http.end();
    return code == 200;
  }

public:
  SensorData getSensorData() {
    SensorData data;
    String ip = getIP();
    if (ip.length() == 0) return data;

    HTTPClient http;
    WiFiClient client;
    http.begin(client, "http://" + ip + "/get-sensor-data");
    http.setTimeout(2000);
    int code = http.GET();
    if (code != 200) { http.end(); return data; }
    String body = http.getString();
    http.end();

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, body);
    if (err) return data;

    data.temperature = doc["temperature"] | NAN;
    data.humidity    = doc["humidity"] | NAN;
    data.light       = doc["light"] | -1;
    data.gas         = doc["gas"] | -1;
    data.alarm       = doc["alarm"] | -1;
    return data;
  }

  bool toggleDevice(const String& device, bool on) {
    String ip = getIP();
    if (ip.length() == 0) return false;

    String json = "{\"device\":\"" + device + "\",\"status\":" + (on ? "true" : "false") + "}";
    return postJSON("http://" + ip + "/toggle-device", json);
  }

  bool toggleAlarm(bool on) {
    String ip = getIP();
    if (ip.length() == 0) return false;

    String json = "{\"status\":" + String(on ? "true" : "false") + "}";
    return postJSON("http://" + ip + "/toggle-alarm", json);
  }

  bool setSmartMode(uint8_t mode) {
    // 0=off, 1=LDR (light sensor), 2=PIR (motion)
    String ip = getIP();
    if (ip.length() == 0) return false;

    String json = "{\"status\":" + String(mode) + "}";
    return postJSON("http://" + ip + "/toggle-smart", json);
  }

  void saveIP(const String& ip) {
    Preferences p;
    p.begin("smarthome", false);
    p.putString("sh_ip", ip);
    p.end();
  }

  String getSavedIP() {
    return getIP();
  }
};
