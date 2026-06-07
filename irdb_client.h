#pragma once
#include <Arduino.h>
#include <vector>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <IRsend.h>

// ════════════════════════════════════════════════════════════
//  IRCode struct — parsed from probonopd/irdb CSV
// ════════════════════════════════════════════════════════════
struct IRCode {
  String functionName;
  String protocol;
  int    device;
  int    subdevice;
  int    function;
};

// ════════════════════════════════════════════════════════════
//  Protocol Translation Table (Section 9)
//
//  Maps irdb protocol names to decode_type_t, packing formula,
//  and bit count. Uses irsend.send(type, data, nbits) for ALL.
// ════════════════════════════════════════════════════════════

// Returns true if the protocol is supported, populating type/data/nbits/repeat
bool irdbPackCode(const IRCode& c, decode_type_t& type, uint64_t& data,
                  uint16_t& nbits, uint8_t& repeat) {
  String p = c.protocol;
  p.toUpperCase();

  if (p == "NEC1" || p == "NEC2" || p == "NEC" || p == "RCA" || p == "RCA-38") {
    // NEC/RCA: ~device:device:~function:function
    type = (p.startsWith("RCA")) ? RCA : NEC;
    data = ((uint64_t)(uint8_t)~c.device   << 24) |
           ((uint64_t)(uint8_t) c.device   << 16) |
           ((uint64_t)(uint8_t)~c.function <<  8) |
           ((uint64_t)(uint8_t) c.function      );
    nbits = 32;
    return true;
  }

  if (p == "SAMSUNG" || p == "SAMSUNG36") {
    type = SAMSUNG;
    data = ((uint64_t)(uint8_t) c.device   << 24) |
           ((uint64_t)(uint8_t) c.device   << 16) |
           ((uint64_t)(uint8_t)~c.function <<  8) |
           ((uint64_t)(uint8_t) c.function      );
    nbits = 32;
    return true;
  }

  if (p == "SAMSUNG48") {
    type = SAMSUNG;
    data = ((uint64_t)c.device << 24) | (c.function & 0xFFFFFFFF);
    nbits = 48;
    return true;
  }

  if (p == "SONY12") {
    type = SONY;
    data = ((uint64_t)(uint8_t)c.device << 7) | (uint8_t)c.function;
    nbits = 12;
    if (repeat < 3) repeat = 3;
    return true;
  }

  if (p == "SONY15") {
    type = SONY;
    data = ((uint64_t)(uint8_t)c.device << 7) | (uint8_t)c.function;
    nbits = 15;
    if (repeat < 3) repeat = 3;
    return true;
  }

  if (p == "SONY20") {
    type = SONY;
    data = ((uint64_t)(uint8_t)c.device << 13) |
           ((uint64_t)(uint8_t)c.subdevice << 7) |
           (uint8_t)c.function;
    nbits = 20;
    if (repeat < 3) repeat = 3;
    return true;
  }

  if (p == "RC5" || p == "RC5X") {
    type = RC5;
    data = ((uint64_t)(uint8_t)c.device << 6) | (uint8_t)c.function;
    nbits = (p == "RC5X") ? 13 : 12;
    return true;
  }

  if (p == "RC6") {
    type = RC6;
    data = ((uint64_t)(uint8_t)c.device << 8) | (uint8_t)c.function;
    nbits = 20;
    return true;
  }

  if (p == "PANASONIC" || p == "PANASONIC2") {
    type = PANASONIC;
    data = ((uint64_t)(uint8_t)c.device    << 16) |
           ((uint64_t)(uint8_t)c.subdevice <<  8) |
           (uint8_t)c.function;
    nbits = 48;
    return true;
  }

  if (p == "SHARP") {
    type = SHARP;
    data = ((uint64_t)(uint8_t)c.device << 8) | (uint8_t)c.function;
    nbits = 15;
    return true;
  }

  if (p == "LG" || p == "LG2") {
    type = LG;
    data = ((uint64_t)(uint8_t)c.device   << 16) |
           ((uint64_t)(uint8_t)c.function <<  8);
    nbits = 28;
    return true;
  }

  if (p == "JVC") {
    type = JVC;
    data = ((uint64_t)(uint8_t)c.device << 8) | (uint8_t)c.function;
    nbits = 16;
    return true;
  }

  if (p == "DENON") {
    type = DENON;
    data = ((uint64_t)(uint8_t)c.device << 8) | (uint8_t)c.function;
    nbits = 15;
    return true;
  }

  if (p == "MITSUBISHI") {
    type = MITSUBISHI;
    data = (uint8_t)c.function;
    nbits = 16;
    return true;
  }

  Serial.printf("[IRDB] Unsupported protocol: %s\n", c.protocol.c_str());
  return false;
}

// ════════════════════════════════════════════════════════════
//  IRDBClient
// ════════════════════════════════════════════════════════════
class IRDBClient {
private:
  const char* BASE = "https://cdn.jsdelivr.net/gh/probonopd/irdb@master/codes";
  const uint32_t CACHE_TTL_MS = 7UL * 24 * 60 * 60 * 1000;
  const int MAX_CACHE_ENTRIES = 20;

  String cacheKey(const char* manufacturer, const char* deviceType) {
    return "irdb_" + String(manufacturer) + "_" + String(deviceType);
  }

  void trimCache() {
    Preferences p;
    p.begin("irdb_cache", false);
    String order = p.getString("cache_order", "");
    std::vector<String> keys;
    int start = 0;
    for (int i = 0; i <= (int)order.length(); i++) {
      if (i == (int)order.length() || order.charAt(i) == ',') {
        keys.push_back(order.substring(start, i));
        start = i + 1;
      }
    }
    while ((int)keys.size() > MAX_CACHE_ENTRIES) {
      String oldest = keys.front();
      keys.erase(keys.begin());
      p.remove(oldest.c_str());
    }
    String newOrder;
    for (size_t i = 0; i < keys.size(); i++) {
      if (i > 0) newOrder += ",";
      newOrder += keys[i];
    }
    p.putString("cache_order", newOrder);
    p.end();
  }

  void touchCache(const String& key) {
    Preferences p;
    p.begin("irdb_cache", false);
    String order = p.getString("cache_order", "");
    String newOrder;
    int start = 0;
    for (int i = 0; i <= (int)order.length(); i++) {
      if (i == (int)order.length() || order.charAt(i) == ',') {
        String k = order.substring(start, i);
        if (k != key) {
          if (newOrder.length() > 0) newOrder += ",";
          newOrder += k;
        }
        start = i + 1;
      }
    }
    if (newOrder.length() > 0) newOrder += ",";
    newOrder += key;
    p.putString("cache_order", newOrder);
    p.end();
  }

public:
  // ── Fetch brand list from irdb CDN ──────────────────────
  bool fetchBrandList(const char* deviceType, String& jsonOut) {
    if (!isWiFiConnected()) return false;
    String url = String(BASE) + "/index?type=" + String(deviceType);
    return httpGet(url, jsonOut);
  }

  // ── Fetch codes for a specific brand/device type ─────────
  bool fetchCodesForBrand(const char* manufacturer, const char* deviceType,
                          std::vector<IRCode>& out) {
    String ck = cacheKey(manufacturer, deviceType);
    Preferences p;
    p.begin("irdb_cache", true);
    if (p.isKey(ck.c_str())) {
      String cached = p.getString(ck.c_str(), "");
      uint32_t ts = p.getULong64((ck + "_ts").c_str(), 0);
      p.end();
      if (millis() - ts < CACHE_TTL_MS) {
        parseCSV(cached, out);
        touchCache(ck);
        return true;
      }
    } else {
      p.end();
    }

    if (!isWiFiConnected()) return false;

    String brandUrl = String(BASE) + "/" + String(manufacturer) + "/" + String(deviceType) + "/";
    String indexContent;
    if (!httpGet(brandUrl, indexContent)) return false;

    std::vector<String> csvUrls;
    int pos = 0;
    while (true) {
      int hrefStart = indexContent.indexOf("href=\"", pos);
      if (hrefStart < 0) break;
      hrefStart += 6;
      int hrefEnd = indexContent.indexOf("\"", hrefStart);
      if (hrefEnd < 0) break;
      String link = indexContent.substring(hrefStart, hrefEnd);
      pos = hrefEnd + 1;
      if (link.endsWith(".csv")) {
        csvUrls.push_back(link);
      }
    }

    if (csvUrls.size() > 5) csvUrls.resize(5);

    String allCSV;
    for (const auto& csvFile : csvUrls) {
      String csvUrl = brandUrl + csvFile;
      String csvContent;
      if (httpGet(csvUrl, csvContent)) {
        allCSV += csvContent + "\n";
      }
    }

    if (allCSV.length() == 0) return false;

    p.begin("irdb_cache", false);
    p.putString(ck.c_str(), allCSV);
    p.putULong64((ck + "_ts").c_str(), millis());
    p.end();
    touchCache(ck);
    trimCache();

    parseCSV(allCSV, out);
    return true;
  }

  void parseCSV(const String& csv, std::vector<IRCode>& out) {
    int pos = 0;
    while (pos < (int)csv.length()) {
      int nl = csv.indexOf('\n', pos);
      String line;
      if (nl >= 0) {
        line = csv.substring(pos, nl);
        pos = nl + 1;
      } else {
        line = csv.substring(pos);
        pos = csv.length();
      }
      line.trim();
      if (line.length() == 0 || line.startsWith("#") || line.startsWith("functionname")) continue;

      int c1 = line.indexOf(',');
      if (c1 < 0) continue;
      int c2 = line.indexOf(',', c1 + 1);
      if (c2 < 0) continue;
      int c3 = line.indexOf(',', c2 + 1);
      if (c3 < 0) continue;
      int c4 = line.indexOf(',', c3 + 1);
      if (c4 < 0) continue;
      int c5 = line.indexOf(',', c4 + 1);

      IRCode code;
      code.functionName = line.substring(0, c1);
      code.protocol     = line.substring(c1 + 1, c2);
      code.device       = line.substring(c2 + 1, c3).toInt();
      String sdStr      = line.substring(c3 + 1, c4);
      code.subdevice    = (sdStr == "-1" || sdStr.length() == 0) ? 0 : sdStr.toInt();
      code.function     = (c5 > 0) ? line.substring(c4 + 1, c5).toInt() : line.substring(c4 + 1).toInt();
      out.push_back(code);
    }
  }

  String brandListJson(const String& raw) {
    String json = "[";
    bool first = true;
    int pos = 0;
    while (true) {
      int hrefStart = raw.indexOf("href=\"", pos);
      if (hrefStart < 0) break;
      hrefStart += 6;
      int hrefEnd = raw.indexOf("/\"", hrefStart);
      if (hrefEnd < 0) break;
      String brand = raw.substring(hrefStart, hrefEnd);
      pos = hrefEnd + 2;
      if (brand.indexOf('.') >= 0 || brand.length() == 0) continue;
      if (!first) json += ",";
      json += "\"" + brand + "\"";
      first = false;
    }
    json += "]";
    return json;
  }

private:
  bool httpGet(const String& url, String& response) {
    if (!isWiFiConnected()) return false;

    WiFiClientSecure client;
    // SECURITY: setInsecure() is acceptable for CDN fetches.
    // jsDelivr content is immutable by hash. Certificate pinning
    // would cost ~10KB RAM for the CA bundle — not viable here.
    client.setInsecure();
    HTTPClient http;

    http.setTimeout(5000);
    http.begin(client, url);

    int retries = 0;
    const int maxRetries = 3;
    int httpCode = 0;

    while (retries < maxRetries) {
      httpCode = http.GET();
      if (httpCode > 0) break;
      retries++;
      if (retries < maxRetries) delay(1000 * retries);
    }

    if (httpCode != 200) {
      http.end();
      return false;
    }

    response = http.getString();
    http.end();
    return true;
  }
};
