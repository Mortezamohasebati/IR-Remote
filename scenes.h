#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <vector>
#include "globals.h"
#include "smarthome_client.h"

struct SceneTrigger {
  String device;
  String btn;
};

struct SceneAction {
  String type;   // "device"
  String device; // "lamp", "cooling", "alarm"
  bool   state;
};

struct Scene {
  String      id;
  String      name;
  SceneTrigger trigger;
  SceneAction  action;
};

void saveScenes(const std::vector<Scene>& scenes) {
  Preferences p;
  p.begin("scenes", false);
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& s : scenes) {
    JsonObject obj = arr.createNestedObject();
    obj["id"]   = s.id;
    obj["name"] = s.name;
    JsonObject t = obj.createNestedObject("trigger");
    t["device"] = s.trigger.device;
    t["btn"]    = s.trigger.btn;
    JsonObject a = obj.createNestedObject("action");
    a["type"]   = s.action.type;
    a["device"] = s.action.device;
    a["state"]  = s.action.state;
  }
  String output;
  serializeJson(doc, output);
  p.putString("scenes", output);
  p.end();
}

std::vector<Scene> loadScenes() {
  Preferences p;
  p.begin("scenes", true);
  std::vector<Scene> scenes;
  if (!p.isKey("scenes")) { p.end(); return scenes; }
  String raw = p.getString("scenes", "[]");
  p.end();

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, raw);
  if (err) return scenes;

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    Scene s;
    s.id   = obj["id"].as<String>();
    s.name = obj["name"].as<String>();
    s.trigger.device = obj["trigger"]["device"].as<String>();
    s.trigger.btn    = obj["trigger"]["btn"].as<String>();
    s.action.type    = obj["action"]["type"].as<String>();
    s.action.device  = obj["action"]["device"].as<String>();
    s.action.state   = obj["action"]["state"].as<bool>();
    scenes.push_back(s);
  }
  return scenes;
}

void loopScenes() {
  // Guard: only check if we received a new IR code
  if (lastRxDevice.length() == 0 || lastRxBtn.length() == 0) return;

  Serial.printf("[SCENE] Received IR: %s / %s\n",
    lastRxDevice.c_str(), lastRxBtn.c_str());

  std::vector<Scene> scenes = loadScenes();
  for (const auto& s : scenes) {
    if (s.trigger.device == lastRxDevice && s.trigger.btn == lastRxBtn) {
      Serial.printf("[SCENE] Triggered: %s → %s = %d\n",
        s.name.c_str(), s.action.device.c_str(), s.action.state);

      if (s.action.type == "device") {
        shClient.toggleDevice(s.action.device, s.action.state);
      }
    }
  }

  // Clear after processing
  lastRxDevice = "";
  lastRxBtn    = "";
}

String scenesJSON() {
  std::vector<Scene> scenes = loadScenes();
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& s : scenes) {
    JsonObject obj = arr.createNestedObject();
    obj["id"]   = s.id;
    obj["name"] = s.name;
    JsonObject t = obj.createNestedObject("trigger");
    t["device"] = s.trigger.device;
    t["btn"]    = s.trigger.btn;
    JsonObject a = obj.createNestedObject("action");
    a["type"]   = s.action.type;
    a["device"] = s.action.device;
    a["state"]  = s.action.state;
  }
  String output;
  serializeJson(doc, output);
  return output;
}
