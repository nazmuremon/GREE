#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include <ir_Gree.h>

#if __has_include("wifi_config.h")
#include "wifi_config.h"
#else
constexpr char kWifiSsid[] = "";
constexpr char kWifiPassword[] = "";
#endif

#ifndef WIFI_PUBLIC_BASE_URL
#define WIFI_PUBLIC_BASE_URL ""
#endif

#ifndef WIFI_USE_STATIC_IP
#define WIFI_USE_STATIC_IP 0
#endif

#ifndef WIFI_STATIC_IP1
#define WIFI_STATIC_IP1 0
#define WIFI_STATIC_IP2 0
#define WIFI_STATIC_IP3 0
#define WIFI_STATIC_IP4 0
#define WIFI_GATEWAY_IP1 0
#define WIFI_GATEWAY_IP2 0
#define WIFI_GATEWAY_IP3 0
#define WIFI_GATEWAY_IP4 0
#define WIFI_SUBNET_MASK1 255
#define WIFI_SUBNET_MASK2 255
#define WIFI_SUBNET_MASK3 255
#define WIFI_SUBNET_MASK4 0
#define WIFI_DNS1_IP1 0
#define WIFI_DNS1_IP2 0
#define WIFI_DNS1_IP3 0
#define WIFI_DNS1_IP4 0
#define WIFI_DNS2_IP1 0
#define WIFI_DNS2_IP2 0
#define WIFI_DNS2_IP3 0
#define WIFI_DNS2_IP4 0
#endif

constexpr uint16_t kRecvPin = 15;
constexpr uint16_t kSendPin = 12;
constexpr uint32_t kBaudRate = 115200;
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint16_t kCaptureBufferSize = 1024;
constexpr uint8_t kCaptureTimeoutMs = 50;
constexpr char kFallbackApSsid[] = "ESP32-IR-Bridge";
constexpr char kFallbackApPassword[] = "irbridge123";
constexpr char kDeviceHostname[] = "gree";
constexpr char kPublicBaseUrl[] = WIFI_PUBLIC_BASE_URL;
constexpr bool kUseStaticIp = WIFI_USE_STATIC_IP != 0;
constexpr bool kEnableDebugIrReader = true;
constexpr char kPreferencesNamespace[] = "gree-remote";
constexpr char kPreferencesStateKey[] = "state";

const IPAddress kStaticIp(WIFI_STATIC_IP1, WIFI_STATIC_IP2, WIFI_STATIC_IP3,
                          WIFI_STATIC_IP4);
const IPAddress kGatewayIp(WIFI_GATEWAY_IP1, WIFI_GATEWAY_IP2,
                           WIFI_GATEWAY_IP3, WIFI_GATEWAY_IP4);
const IPAddress kSubnetMask(WIFI_SUBNET_MASK1, WIFI_SUBNET_MASK2,
                            WIFI_SUBNET_MASK3, WIFI_SUBNET_MASK4);
const IPAddress kDns1Ip(WIFI_DNS1_IP1, WIFI_DNS1_IP2, WIFI_DNS1_IP3,
                        WIFI_DNS1_IP4);
const IPAddress kDns2Ip(WIFI_DNS2_IP1, WIFI_DNS2_IP2, WIFI_DNS2_IP3,
                        WIFI_DNS2_IP4);

IRGreeAC greeAc(kSendPin, gree_ac_remote_model_t::YAW1F);
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kCaptureTimeoutMs, true);
decode_results irResults;
WebServer server(kHttpPort);
Preferences preferences;

String wifiModeLabel = "offline";
String wifiAddress = "offline";
String wifiHostLabel = String(kDeviceHostname) + ".local";
String preferredBaseUrl = "offline";
String lastEventMessage = "Ready.";
String lastNativeAcSummary;
bool mdnsActive = false;
bool preferencesReady = false;

bool hasDebugCapture = false;
String debugProtocol = "";
String debugHexValue = "";
uint16_t debugBits = 0;
uint16_t debugRawLength = 0;
String debugLastEvent = "No remote value captured yet.";

struct GreeSwingVState {
  bool automatic;
  uint8_t position;
};

String escapeJson(const String& input) {
  String escaped;
  escaped.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); i++) {
    const char current = input[i];
    switch (current) {
      case '\\':
        escaped += F("\\\\");
        break;
      case '"':
        escaped += F("\\\"");
        break;
      case '\n':
        escaped += F("\\n");
        break;
      case '\r':
        escaped += F("\\r");
        break;
      case '\t':
        escaped += F("\\t");
        break;
      default:
        escaped += current;
        break;
    }
  }
  return escaped;
}

String normalizeBaseUrl(const String& url) {
  String normalized = url;
  normalized.trim();
  if (normalized.length() == 0) return "";
  if (!normalized.startsWith("http://") &&
      !normalized.startsWith("https://")) {
    normalized = "https://" + normalized;
  }
  while (normalized.endsWith("/")) {
    normalized.remove(normalized.length() - 1);
  }
  return normalized;
}

String getLocalBaseUrl() {
  if (wifiAddress == "offline") return "offline";
  return "http://" + wifiAddress;
}

void updatePreferredBaseUrl() {
  const String publicUrl = normalizeBaseUrl(kPublicBaseUrl);
  if (publicUrl.length() > 0) {
    preferredBaseUrl = publicUrl;
  } else if (mdnsActive) {
    preferredBaseUrl = "http://" + wifiHostLabel;
  } else {
    preferredBaseUrl = getLocalBaseUrl();
  }
}

bool isWifiConfigured() {
  return strlen(kWifiSsid) > 0;
}

uint8_t cycleGreeMode(uint8_t currentMode) {
  switch (currentMode) {
    case kGreeAuto:
      return kGreeCool;
    case kGreeCool:
      return kGreeDry;
    case kGreeDry:
      return kGreeFan;
    case kGreeFan:
      return kGreeHeat;
    case kGreeHeat:
    default:
      return kGreeAuto;
  }
}

uint8_t cycleGreeFan(uint8_t currentFan) {
  switch (currentFan) {
    case kGreeFanAuto:
      return kGreeFanMin;
    case kGreeFanMin:
      return kGreeFanMed;
    case kGreeFanMed:
      return kGreeFanMax;
    case kGreeFanMax:
    default:
      return kGreeFanAuto;
  }
}

GreeSwingVState cycleGreeSwingV(bool automatic, uint8_t position) {
  if (automatic) {
    switch (position) {
      case kGreeSwingDownAuto:
        return {false, kGreeSwingDown};
      case kGreeSwingMiddleAuto:
        return {false, kGreeSwingMiddle};
      case kGreeSwingUpAuto:
        return {false, kGreeSwingMiddleUp};
      case kGreeSwingAuto:
      default:
        return {false, kGreeSwingUp};
    }
  }

  switch (position) {
    case kGreeSwingUp:
      return {false, kGreeSwingMiddleUp};
    case kGreeSwingMiddleUp:
      return {false, kGreeSwingMiddle};
    case kGreeSwingMiddle:
      return {false, kGreeSwingMiddleDown};
    case kGreeSwingMiddleDown:
      return {false, kGreeSwingDown};
    case kGreeSwingDown:
    case kGreeSwingLastPos:
    default:
      return {true, kGreeSwingAuto};
  }
}

uint8_t cycleGreeSwingH(uint8_t current) {
  switch (current) {
    case kGreeSwingHAuto:
      return kGreeSwingHLeft;
    case kGreeSwingHLeft:
      return kGreeSwingHMiddle;
    case kGreeSwingHMiddle:
      return kGreeSwingHRight;
    case kGreeSwingHRight:
      return kGreeSwingHMaxLeft;
    case kGreeSwingHMaxLeft:
      return kGreeSwingHMaxRight;
    case kGreeSwingHMaxRight:
    default:
      return kGreeSwingHAuto;
  }
}

uint8_t cycleGreeDisplay(uint8_t current) {
  switch (current) {
    case kGreeDisplayTempSet:
      return kGreeDisplayTempInside;
    case kGreeDisplayTempInside:
      return kGreeDisplayTempOutside;
    case kGreeDisplayTempOutside:
      return kGreeDisplayTempOff;
    case kGreeDisplayTempOff:
    default:
      return kGreeDisplayTempSet;
  }
}

const char* getGreeModeCode(uint8_t mode) {
  switch (mode) {
    case kGreeCool:
      return "cool";
    case kGreeDry:
      return "dry";
    case kGreeFan:
      return "fan";
    case kGreeHeat:
      return "heat";
    case kGreeAuto:
    default:
      return "auto";
  }
}

const char* getGreeModeLabel(uint8_t mode) {
  switch (mode) {
    case kGreeCool:
      return "Cool";
    case kGreeDry:
      return "Dry";
    case kGreeFan:
      return "Fan";
    case kGreeHeat:
      return "Heat";
    case kGreeAuto:
    default:
      return "Auto";
  }
}

const char* getGreeFanLabel(uint8_t fan) {
  switch (fan) {
    case kGreeFanMin:
      return "Low";
    case kGreeFanMed:
      return "Medium";
    case kGreeFanMax:
      return "High";
    case kGreeFanAuto:
    default:
      return "Auto";
  }
}

const char* getGreeSwingVLabel(bool automatic, uint8_t position) {
  if (automatic) {
    switch (position) {
      case kGreeSwingDownAuto:
        return "Auto Down";
      case kGreeSwingMiddleAuto:
        return "Auto Mid";
      case kGreeSwingUpAuto:
        return "Auto Up";
      case kGreeSwingAuto:
      default:
        return "Auto";
    }
  }

  switch (position) {
    case kGreeSwingUp:
      return "Up";
    case kGreeSwingMiddleUp:
      return "Mid Up";
    case kGreeSwingMiddle:
      return "Middle";
    case kGreeSwingMiddleDown:
      return "Mid Down";
    case kGreeSwingDown:
      return "Down";
    case kGreeSwingLastPos:
    default:
      return "Hold";
  }
}

const char* getGreeSwingHLabel(uint8_t position) {
  switch (position) {
    case kGreeSwingHMaxLeft:
      return "Max Left";
    case kGreeSwingHLeft:
      return "Left";
    case kGreeSwingHMiddle:
      return "Middle";
    case kGreeSwingHRight:
      return "Right";
    case kGreeSwingHMaxRight:
      return "Max Right";
    case kGreeSwingHAuto:
      return "Auto";
    case kGreeSwingHOff:
    default:
      return "Off";
  }
}

const char* getGreeDisplaySourceCode(uint8_t source) {
  switch (source) {
    case kGreeDisplayTempInside:
      return "inside";
    case kGreeDisplayTempOutside:
      return "outside";
    case kGreeDisplayTempOff:
      return "off";
    case kGreeDisplayTempSet:
    default:
      return "set";
  }
}

const char* getGreeDisplaySourceLabel(uint8_t source) {
  switch (source) {
    case kGreeDisplayTempInside:
      return "Indoor";
    case kGreeDisplayTempOutside:
      return "Outdoor";
    case kGreeDisplayTempOff:
      return "Display Off";
    case kGreeDisplayTempSet:
    default:
      return "Set Temp";
  }
}

String formatTimerLabel(uint16_t minutes) {
  if (minutes == 0) return "Off";
  String label;
  const uint16_t hours = minutes / 60;
  const uint16_t remainder = minutes % 60;
  if (hours > 0) label += String(hours) + "h";
  if (remainder > 0) {
    if (label.length() > 0) label += " ";
    label += String(remainder) + "m";
  }
  return label;
}

String getGreeDisplayValue() {
  const uint8_t source = greeAc.getDisplayTempSource();
  if (!greeAc.getPower() || source == kGreeDisplayTempOff) return "--";
  if (source == kGreeDisplayTempSet) return String(greeAc.getTemp());
  return "--";
}

String getGreeDisplayNote() {
  const uint8_t source = greeAc.getDisplayTempSource();
  if (!greeAc.getPower()) return "Off";
  switch (source) {
    case kGreeDisplayTempSet:
      return "Set temp";
    case kGreeDisplayTempInside:
      return "Indoor temp mode";
    case kGreeDisplayTempOutside:
      return "Outdoor temp mode";
    case kGreeDisplayTempOff:
    default:
      return "Display off";
  }
}

bool saveCurrentGreeState() {
  if (!preferencesReady) return false;
  const uint8_t* rawState = greeAc.getRaw();
  return preferences.putBytes(kPreferencesStateKey, rawState,
                              kGreeStateLength) == kGreeStateLength;
}

void resetNativeGreeState() {
  greeAc.begin();
  greeAc.stateReset();
  greeAc.setModel(gree_ac_remote_model_t::YAW1F);
  greeAc.setPower(false);
  greeAc.setMode(kGreeAuto);
  greeAc.setTemp(24);
  greeAc.setFan(kGreeFanAuto);
  greeAc.setSwingVertical(true, kGreeSwingAuto);
  greeAc.setSwingHorizontal(kGreeSwingHAuto);
  greeAc.setTimer(0);
  greeAc.setLight(true);
  greeAc.setXFan(false);
  greeAc.setSleep(false);
  greeAc.setTurbo(false);
  greeAc.setEcono(false);
  greeAc.setIFeel(false);
  greeAc.setWiFi(false);
  greeAc.setDisplayTempSource(kGreeDisplayTempSet);
  lastNativeAcSummary = greeAc.toString();
  saveCurrentGreeState();
}

bool loadSavedGreeState() {
  if (!preferencesReady) return false;
  if (preferences.getBytesLength(kPreferencesStateKey) != kGreeStateLength) {
    return false;
  }

  uint8_t rawState[kGreeStateLength];
  if (preferences.getBytes(kPreferencesStateKey, rawState, kGreeStateLength) !=
      kGreeStateLength) {
    return false;
  }
  if (!IRGreeAC::validChecksum(rawState, kGreeStateLength)) {
    return false;
  }

  greeAc.begin();
  greeAc.setModel(gree_ac_remote_model_t::YAW1F);
  greeAc.setRaw(rawState);
  lastNativeAcSummary = greeAc.toString();
  return true;
}

void storeDebugCapture(const decode_results& capture) {
  debugBits = capture.bits;
  debugProtocol = typeToString(capture.decode_type, capture.repeat);
  debugHexValue = resultToHexidecimal(&capture);
  debugRawLength = capture.rawlen;
  hasDebugCapture = true;
  debugLastEvent = String("IR capture: ") + debugProtocol + " " + debugHexValue;
  Serial.println(debugLastEvent);
}

void handleDebugCapture() {
  if (!kEnableDebugIrReader) return;
  if (irrecv.decode(&irResults)) {
    storeDebugCapture(irResults);
    irrecv.resume();
  }
}

String getGreeStateJson() {
  const bool power = greeAc.getPower();
  const uint8_t mode = greeAc.getMode();
  const uint8_t fan = greeAc.getFan();
  const bool swingVAutomatic = greeAc.getSwingVerticalAuto();
  const uint8_t swingVPosition = greeAc.getSwingVerticalPosition();
  const uint8_t swingHPosition = greeAc.getSwingHorizontal();
  const uint16_t timerMinutes = greeAc.getTimer();
  const uint8_t displaySource = greeAc.getDisplayTempSource();

  String payload = "{";
  payload += "\"power\":";
  payload += power ? "true" : "false";
  payload += ",";
  payload += "\"mode\":\"" + escapeJson(getGreeModeCode(mode)) + "\",";
  payload += "\"mode_label\":\"" + escapeJson(getGreeModeLabel(mode)) + "\",";
  payload += "\"temperature\":" + String(greeAc.getTemp()) + ",";
  payload += "\"fan_label\":\"" + escapeJson(getGreeFanLabel(fan)) + "\",";
  payload += "\"swing_vertical_label\":\"" +
             escapeJson(getGreeSwingVLabel(swingVAutomatic, swingVPosition)) +
             "\",";
  payload += "\"swing_horizontal_label\":\"" +
             escapeJson(getGreeSwingHLabel(swingHPosition)) + "\",";
  payload += "\"sleep\":";
  payload += greeAc.getSleep() ? "true" : "false";
  payload += ",";
  payload += "\"turbo\":";
  payload += greeAc.getTurbo() ? "true" : "false";
  payload += ",";
  payload += "\"eco\":";
  payload += greeAc.getEcono() ? "true" : "false";
  payload += ",";
  payload += "\"light\":";
  payload += greeAc.getLight() ? "true" : "false";
  payload += ",";
  payload += "\"xfan\":";
  payload += greeAc.getXFan() ? "true" : "false";
  payload += ",";
  payload += "\"ifeel\":";
  payload += greeAc.getIFeel() ? "true" : "false";
  payload += ",";
  payload += "\"wifi\":";
  payload += greeAc.getWiFi() ? "true" : "false";
  payload += ",";
  payload += "\"timer_label\":\"" + escapeJson(formatTimerLabel(timerMinutes)) +
             "\",";
  payload += "\"display_source\":\"" +
             escapeJson(getGreeDisplaySourceCode(displaySource)) + "\",";
  payload += "\"display_source_label\":\"" +
             escapeJson(getGreeDisplaySourceLabel(displaySource)) + "\",";
  payload += "\"display_value\":\"" + escapeJson(getGreeDisplayValue()) + "\",";
  payload += "\"display_note\":\"" + escapeJson(getGreeDisplayNote()) + "\"";
  payload += "}";
  return payload;
}

String getDebugCaptureJson() {
  String payload = "{";
  payload += "\"enabled\":";
  payload += kEnableDebugIrReader ? "true" : "false";
  payload += ",";
  payload += "\"available\":";
  payload += hasDebugCapture ? "true" : "false";
  payload += ",";
  payload += "\"protocol\":\"" + escapeJson(debugProtocol) + "\",";
  payload += "\"bits\":" + String(debugBits) + ",";
  payload += "\"hex\":\"" + escapeJson(debugHexValue) + "\",";
  payload += "\"raw_length\":" + String(debugRawLength) + ",";
  payload += "\"message\":\"" + escapeJson(debugLastEvent) + "\"";
  payload += "}";
  return payload;
}

String getStatusJson() {
  String payload = "{";
  payload += "\"wifi_mode\":\"" + escapeJson(wifiModeLabel) + "\",";
  payload += "\"address\":\"" + escapeJson(wifiAddress) + "\",";
  payload += "\"hostname\":\"" + escapeJson(wifiHostLabel) + "\",";
  payload += "\"local_url\":\"" + escapeJson(getLocalBaseUrl()) + "\",";
  payload += "\"preferred_url\":\"" + escapeJson(preferredBaseUrl) + "\",";
  payload += "\"bookmark_url\":\"" + escapeJson(preferredBaseUrl) + "\",";
  payload += "\"public_url\":\"" + escapeJson(normalizeBaseUrl(kPublicBaseUrl)) +
             "\",";
  payload += "\"mdns_active\":";
  payload += mdnsActive ? "true" : "false";
  payload += ",";
  payload += "\"native_ac_last_state\":\"" + escapeJson(lastNativeAcSummary) +
             "\",";
  payload += "\"gree_state\":" + getGreeStateJson() + ",";
  payload += "\"debug_ir\":" + getDebugCaptureJson() + ",";
  payload += "\"last_event\":\"" + escapeJson(lastEventMessage) + "\"";
  payload += "}";
  return payload;
}

bool isSupportedButton(const String& buttonId) {
  static const char* const kButtons[] = {
      "power",   "mode",   "fan",     "temp_up", "temp_down", "cool",
      "heat",    "dry",    "auto",    "fan_only","swing",     "swing_h",
      "timer",   "sleep",  "turbo",   "xfan",    "light",     "wifi",
      "ifeel",   "display","energy",
  };

  for (const char* id : kButtons) {
    if (buttonId == id) return true;
  }
  return false;
}

String sendNativeGreeButtonCode(const String& buttonId, uint16_t count) {
  if (count == 0) return "Count must be at least 1.";
  if (!isSupportedButton(buttonId)) return "Unknown button.";

  if (buttonId == "power") {
    greeAc.setPower(!greeAc.getPower());
  } else if (buttonId == "mode") {
    greeAc.setMode(cycleGreeMode(greeAc.getMode()));
    greeAc.setPower(true);
  } else if (buttonId == "fan") {
    greeAc.setFan(cycleGreeFan(greeAc.getFan()));
    greeAc.setPower(true);
  } else if (buttonId == "wifi") {
    greeAc.setWiFi(!greeAc.getWiFi());
  } else if (buttonId == "temp_up") {
    uint8_t nextTemp = greeAc.getTemp();
    if (nextTemp < kGreeMaxTempC) nextTemp++;
    greeAc.setMode(kGreeCool);
    greeAc.setTemp(nextTemp);
    greeAc.setPower(true);
  } else if (buttonId == "temp_down") {
    uint8_t nextTemp = greeAc.getTemp();
    if (nextTemp > kGreeMinTempC) nextTemp--;
    greeAc.setMode(kGreeCool);
    greeAc.setTemp(nextTemp);
    greeAc.setPower(true);
  } else if (buttonId == "cool") {
    greeAc.setMode(kGreeCool);
    greeAc.setPower(true);
  } else if (buttonId == "heat") {
    greeAc.setMode(kGreeHeat);
    greeAc.setPower(true);
  } else if (buttonId == "dry") {
    greeAc.setMode(kGreeDry);
    greeAc.setPower(true);
  } else if (buttonId == "auto") {
    greeAc.setMode(kGreeAuto);
    greeAc.setPower(true);
  } else if (buttonId == "fan_only") {
    greeAc.setMode(kGreeFan);
    greeAc.setPower(true);
  } else if (buttonId == "swing") {
    const GreeSwingVState nextSwing =
        cycleGreeSwingV(greeAc.getSwingVerticalAuto(),
                        greeAc.getSwingVerticalPosition());
    greeAc.setSwingVertical(nextSwing.automatic, nextSwing.position);
    greeAc.setPower(true);
  } else if (buttonId == "swing_h") {
    greeAc.setSwingHorizontal(cycleGreeSwingH(greeAc.getSwingHorizontal()));
    greeAc.setPower(true);
  } else if (buttonId == "timer") {
    uint16_t nextTimer = greeAc.getTimer() + 30;
    if (nextTimer > kGreeTimerMax) nextTimer = 0;
    greeAc.setTimer(nextTimer);
  } else if (buttonId == "sleep") {
    greeAc.setSleep(!greeAc.getSleep());
    greeAc.setPower(true);
  } else if (buttonId == "turbo") {
    const bool turbo = !greeAc.getTurbo();
    greeAc.setTurbo(turbo);
    if (turbo) greeAc.setEcono(false);
    greeAc.setPower(true);
  } else if (buttonId == "xfan") {
    greeAc.setXFan(!greeAc.getXFan());
    greeAc.setPower(true);
  } else if (buttonId == "light") {
    greeAc.setLight(!greeAc.getLight());
  } else if (buttonId == "ifeel") {
    greeAc.setIFeel(!greeAc.getIFeel());
    greeAc.setPower(true);
  } else if (buttonId == "display") {
    greeAc.setDisplayTempSource(cycleGreeDisplay(greeAc.getDisplayTempSource()));
  } else if (buttonId == "energy") {
    const bool econo = !greeAc.getEcono();
    greeAc.setEcono(econo);
    if (econo) greeAc.setTurbo(false);
    greeAc.setPower(true);
  }

  if (kEnableDebugIrReader) {
    irrecv.pause();
    delay(40);
  }
  for (uint16_t i = 0; i < count; i++) {
    greeAc.send();
    delay(120);
  }
  if (kEnableDebugIrReader) {
    irrecv.resume();
  }

  lastNativeAcSummary = greeAc.toString();
  saveCurrentGreeState();
  lastEventMessage = String("Sent: ") + buttonId;
  Serial.println(lastEventMessage);
  return lastEventMessage;
}

String getRootPage() {
  return R"HTML(<!doctype html>
<html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Universal Gree Remote</title>
<style>
:root{--bg:#dee8ec;--bg2:#f7fafc;--remote:#f3f2ef;--remote-edge:#dad7d1;--ink:#1d2d39;--muted:#64717a;--lcd:#e6efdc;--lcd2:#f3f7ed;--lcd-ink:#4b5651;--silver1:#fcfcfb;--silver2:#d7d9d7;--silver3:#b8bab8;--orange1:#efb46c;--orange2:#d88833;--orange3:#be6c22}
*{box-sizing:border-box}body{margin:0;min-height:100vh;display:grid;place-items:center;font-family:"Segoe UI",Tahoma,sans-serif;background:radial-gradient(circle at top left,rgba(26,82,72,.12),transparent 25%),linear-gradient(180deg,var(--bg),var(--bg2));color:var(--ink)}button{font:inherit}
.stage{width:min(100%,430px);padding:14px}.remote{position:relative;padding:14px 22px 18px;border-radius:34px;background:linear-gradient(180deg,#fff,var(--remote) 38%,var(--remote-edge));box-shadow:inset 0 2px 1px rgba(255,255,255,.95),0 28px 40px rgba(20,35,48,.18)}.remote:before,.remote:after{content:"";position:absolute;top:10px;bottom:10px;width:12px;border-radius:18px;background:linear-gradient(180deg,#e8ecee,#c8ced2)}.remote:before{left:6px}.remote:after{right:6px}
.screen-shell{margin:0 auto 16px;width:100%;padding:14px 12px 12px;border-radius:12px;background:linear-gradient(180deg,#0f1418,#1a2126 18%,#10161b);box-shadow:inset 0 1px 0 rgba(255,255,255,.08)}.screen{position:relative;min-height:220px;padding:12px 12px 16px;border-radius:3px;background:linear-gradient(180deg,var(--lcd2),var(--lcd));color:var(--lcd-ink);overflow:hidden}.screen:before,.screen:after{content:"";position:absolute;top:0;bottom:0;width:1px;background:rgba(75,86,81,.22)}.screen:before{left:36px}.screen:after{right:36px}
.lcd-top,.lcd-mid{position:relative;z-index:1}.lcd-top{display:flex;justify-content:space-between;font-size:.9rem;font-weight:700;letter-spacing:.08em}.lcd-icons{display:flex;justify-content:space-between;align-items:flex-start;margin-top:8px;font-size:1rem}.lcd-temp{display:flex;justify-content:center;align-items:flex-start;gap:4px;margin-top:6px;font-family:"Courier New",monospace;color:#5a6460}.lcd-temp strong{font-size:5.4rem;line-height:.9;font-weight:700}.lcd-temp span{font-size:2rem;font-weight:700;padding-top:16px}.lcd-mid{text-align:center;margin-top:4px;font-size:.96rem;font-weight:700}.lcd-row{display:flex;justify-content:space-between;align-items:center;margin-top:10px;font-size:1rem;font-weight:700}.lcd-flag-row{display:flex;justify-content:space-between;gap:8px;flex-wrap:wrap;margin-top:16px;font-size:.92rem}.lcd-note{margin-top:8px;font-size:.78rem;line-height:1.3;opacity:.82;text-align:center}.lcd-logo{position:absolute;right:12px;bottom:10px;font-size:.78rem;font-weight:800;letter-spacing:.24em;opacity:.18}
.button-grid{display:grid;gap:10px}.button-row-2{grid-template-columns:repeat(2,minmax(0,1fr))}.button-row-3{grid-template-columns:repeat(3,minmax(0,1fr))}.button-row-4{grid-template-columns:repeat(4,minmax(0,1fr))}
.btn{min-height:38px;padding:0 8px;border:1px solid rgba(95,101,106,.55);border-radius:999px;background:linear-gradient(180deg,var(--silver1),var(--silver2) 58%,var(--silver3));color:#33424c;box-shadow:inset 0 1px 0 rgba(255,255,255,.92),0 2px 3px rgba(0,0,0,.12);font-size:.76rem;font-weight:800;letter-spacing:.03em;cursor:pointer}.btn:active{transform:translateY(1px)}.btn.active{background:linear-gradient(180deg,#f5d6b0,var(--orange1) 58%,var(--orange2));border-color:rgba(163,97,29,.7);color:#5a2a00}.btn.soft{font-size:.72rem}
.power-row{display:grid;grid-template-columns:1fr 74px 1fr;gap:10px;align-items:center;margin:12px 0 10px}.power-btn{width:74px;height:74px;border:0;border-radius:999px;background:radial-gradient(circle at 35% 30%,#ffd8a3,var(--orange1) 28%,var(--orange2) 62%,var(--orange3));color:#5e2e05;font-weight:900;letter-spacing:.05em;box-shadow:inset 0 2px 4px rgba(255,255,255,.6),0 10px 18px rgba(190,108,34,.28);cursor:pointer}.power-btn.on{box-shadow:0 0 0 5px rgba(239,180,108,.2),inset 0 2px 4px rgba(255,255,255,.6),0 10px 18px rgba(190,108,34,.28)}.rule{height:3px;border-radius:99px;background:linear-gradient(90deg,transparent,#d3d5d3 22%,#b5b7b5 50%,#d3d5d3 78%,transparent);margin:6px 22px 8px}
.debug{margin-top:10px;padding:10px 12px;border-radius:14px;background:rgba(0,0,0,.06)}.debug summary{cursor:pointer;font-weight:700}.debug-grid{display:grid;gap:6px;margin-top:8px;font-size:.8rem}.debug-grid code{display:block;padding:7px 8px;border-radius:8px;background:#10161b;color:#d7f0dc;overflow:auto}
.footer{margin-top:14px;padding:12px 0 10px;border-radius:0 0 14px 14px;background:linear-gradient(180deg,#151b20,#0d1115);color:#eef3f6;text-align:center}.footer strong{display:block;font-size:1.55rem;letter-spacing:.08em}.footer span{display:block;margin-top:6px;font-size:.76rem;color:#c6d0d8;min-height:18px}.chip{opacity:.55}.chip.on{opacity:1}
@media(max-width:380px){.stage{padding:8px}.remote{padding:12px 18px 16px}.power-row{grid-template-columns:1fr 64px 1fr}.power-btn{width:64px;height:64px}.btn{font-size:.7rem}.screen{min-height:205px}.lcd-temp strong{font-size:4.8rem}}
</style></head><body><main class="stage"><div class="remote">
<div class="screen-shell"><div class="screen">
<div class="lcd-top"><span id="lcdFanTop">FAN AUTO</span><span id="lcdPowerTop">OFF</span></div>
<div class="lcd-icons"><span id="lcdLeftIcon">AIR</span><span id="lcdSwingTop">AUTO</span><span id="lcdRightIcon">WIFI</span></div>
<div class="lcd-temp"><strong id="lcdDigits">--</strong><span id="lcdUnit">C</span></div>
<div id="lcdMode" class="lcd-mid">AUTO</div>
<div class="lcd-row"><span id="lcdTimer">TIMER OFF</span><span id="lcdDisplaySource">SET TEMP</span></div>
<div class="lcd-flag-row"><span id="flagSleep" class="chip">SLEEP</span><span id="flagTurbo" class="chip">TURBO</span><span id="flagEco" class="chip">ECO</span><span id="flagIFeel" class="chip">I FEEL</span></div>
<div class="lcd-flag-row"><span id="flagXFan" class="chip">X-FAN</span><span id="flagLight" class="chip">LIGHT</span><span id="flagWifi" class="chip">WIFI</span><span id="flagDisplay" class="chip">DISPLAY</span></div>
<div id="lcdNote" class="lcd-note">Waiting for remote state...</div><div class="lcd-logo">NAZMUR</div></div></div>
<div class="button-grid button-row-2"><button class="btn" onclick="sendButton('mode')">MODE</button><button class="btn" onclick="sendButton('fan')">FAN</button></div>
<div class="rule"></div>
<div class="power-row"><button class="btn soft" onclick="sendButton('swing')">SWING V</button><button id="powerBtn" class="power-btn" onclick="sendButton('power')">POWER</button><button class="btn soft" onclick="sendButton('temp_up')">TEMP UP</button></div>
<div class="button-grid button-row-3"><button class="btn soft" onclick="sendButton('swing_h')">SWING H</button><button id="btnTurbo" class="btn" onclick="sendButton('turbo')">TURBO</button><button class="btn soft" onclick="sendButton('temp_down')">TEMP DN</button></div>
<div class="button-grid button-row-2" style="margin-top:10px"><button id="btnSleep" class="btn" onclick="sendButton('sleep')">SLEEP</button><button id="btnIFeel" class="btn" onclick="sendButton('ifeel')">I FEEL</button></div>
<div class="button-grid button-row-2" style="margin-top:10px"><button class="btn" onclick="sendButton('timer')">TIMER</button><button id="btnDisplay" class="btn" onclick="sendButton('display')">DISPLAY</button></div>
<div class="button-grid button-row-3" style="margin-top:10px"><button id="btnWifi" class="btn" onclick="sendButton('wifi')">WIFI</button><button id="btnEco" class="btn" onclick="sendButton('energy')">ECO</button><button id="btnLight" class="btn" onclick="sendButton('light')">LIGHT</button></div>
<div class="button-grid button-row-4" style="margin-top:10px"><button id="modeAuto" class="btn soft" onclick="sendButton('auto')">AUTO</button><button id="modeCool" class="btn soft" onclick="sendButton('cool')">COOL</button><button id="modeDry" class="btn soft" onclick="sendButton('dry')">DRY</button><button id="modeHeat" class="btn soft" onclick="sendButton('heat')">HEAT</button></div>
<details class="debug"><summary>IR Debug Reader</summary><div class="debug-grid"><div>Protocol: <span id="dbgProtocol">-</span></div><div>Bits: <span id="dbgBits">-</span></div><div>Raw Length: <span id="dbgRaw">-</span></div><code id="dbgHex">No capture yet</code><div id="dbgMsg">Waiting for remote value...</div></div></details>
<div class="footer"><strong>GREE</strong><span id="msg">Connecting to the remote...</span></div></div></main>
<script>
const UiState={BOOT:"boot",SYNC:"sync",READY:"ready",ERROR:"error"};const app={state:UiState.BOOT,status:null,busy:false,timer:null,redirected:false};const $=id=>document.getElementById(id);
function setMessage(text,isError=false){const n=$("msg");n.textContent=text;n.style.color=isError?"#ffd3c2":"#c6d0d8"}function transition(nextState,message=""){app.state=nextState;if(message)setMessage(message,nextState===UiState.ERROR)}function scheduleSync(){clearTimeout(app.timer);app.timer=setTimeout(syncStatus,document.hidden?30000:12000)}
function setChip(id,active){$(id).className=active?"chip on":"chip"}function setActive(id,active){$(id).classList.toggle("active",Boolean(active))}
function updateButtons(g){$("powerBtn").classList.toggle("on",Boolean(g&&g.power));setActive("btnTurbo",g&&g.turbo);setActive("btnSleep",g&&g.sleep);setActive("btnIFeel",g&&g.ifeel);setActive("btnDisplay",g&&g.display_source!=="off");setActive("btnWifi",g&&g.wifi);setActive("btnEco",g&&g.eco);setActive("btnLight",g&&g.light);setActive("modeAuto",g&&g.mode==="auto");setActive("modeCool",g&&g.mode==="cool");setActive("modeDry",g&&g.mode==="dry");setActive("modeHeat",g&&g.mode==="heat")}
function maybeRedirectToBookmark(s){if(!s||!s.public_url||app.redirected)return;if(location.origin===s.public_url)return;app.redirected=true;location.replace(s.public_url+"/")}
function renderDebug(d){$("dbgProtocol").textContent=d&&d.available?d.protocol:"-";$("dbgBits").textContent=d&&d.available?d.bits:"-";$("dbgRaw").textContent=d&&d.available?d.raw_length:"-";$("dbgHex").textContent=d&&d.available?d.hex:"No capture yet";$("dbgMsg").textContent=d?d.message:"Waiting for remote value..."}
function renderRemote(){const g=app.status&&app.status.gree_state?app.status.gree_state:null;const d=app.status&&app.status.debug_ir?app.status.debug_ir:null;$("lcdFanTop").textContent=g?`FAN ${g.fan_label.toUpperCase()}`:"FAN AUTO";$("lcdPowerTop").textContent=g&&g.power?"OPER":"OFF";$("lcdLeftIcon").textContent=g?g.swing_vertical_label.toUpperCase():"AIR";$("lcdSwingTop").textContent=g?g.mode_label.toUpperCase():"AUTO";$("lcdRightIcon").textContent=g&&g.wifi?"WIFI":"";$("lcdDigits").textContent=g?g.display_value:"--";$("lcdUnit").textContent=g&&g.display_value!=="--"?"C":"";$("lcdMode").textContent=g?g.mode_label.toUpperCase():"AUTO";$("lcdTimer").textContent=g?`TIMER ${g.timer_label.toUpperCase()}`:"TIMER OFF";$("lcdDisplaySource").textContent=g?g.display_source_label.toUpperCase():"SET TEMP";$("lcdNote").textContent=g?g.display_note:"Waiting for remote state...";setChip("flagSleep",Boolean(g&&g.sleep));setChip("flagTurbo",Boolean(g&&g.turbo));setChip("flagEco",Boolean(g&&g.eco));setChip("flagIFeel",Boolean(g&&g.ifeel));setChip("flagXFan",Boolean(g&&g.xfan));setChip("flagLight",Boolean(g&&g.light));setChip("flagWifi",Boolean(g&&g.wifi));setChip("flagDisplay",Boolean(g&&g.display_source!=="off"));updateButtons(g);renderDebug(d)}
async function requestJson(url){const r=await fetch(url,{cache:"no-store"});const b=await r.json();if(!r.ok)throw new Error(b.message||"Request failed");return b}
async function syncStatus(){if(app.busy)return;app.busy=true;transition(UiState.SYNC);try{app.status=await requestJson("/api/status");maybeRedirectToBookmark(app.status);renderRemote();transition(UiState.READY,app.status.last_event||"Ready")}catch(e){transition(UiState.ERROR,e.message||"Could not load the remote.")}finally{app.busy=false;scheduleSync()}}
async function sendButton(buttonId){if(app.busy)return;app.busy=true;transition(UiState.SYNC);try{const r=await requestJson(`/api/button/send?buttonId=${encodeURIComponent(buttonId)}&count=1`);if(r.status)app.status=r.status;renderRemote();transition(UiState.READY,r.message||"Done")}catch(e){transition(UiState.ERROR,e.message||"Request failed")}finally{app.busy=false;scheduleSync()}}
document.addEventListener("visibilitychange",scheduleSync);syncStatus();
</script></body></html>)HTML";
}

void sendJson(int statusCode, const String& payload) {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(statusCode, "application/json", payload);
}

void sendJsonResponse(int statusCode, bool ok, const String& message) {
  String payload = "{";
  payload += "\"ok\":";
  payload += ok ? "true" : "false";
  payload += ",";
  payload += "\"message\":\"" + escapeJson(message) + "\",";
  payload += "\"status\":" + getStatusJson();
  payload += "}";
  sendJson(statusCode, payload);
}

bool requireArg(const char* name) {
  if (server.hasArg(name)) return true;
  sendJsonResponse(400, false, String("Missing: ") + name);
  return false;
}

uint16_t parseSendCount() {
  if (!server.hasArg("count")) return 1;
  const long count = server.arg("count").toInt();
  if (count < 1) return 1;
  if (count > 5) return 5;
  return static_cast<uint16_t>(count);
}

void handleRootRequest() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "text/html; charset=utf-8", getRootPage());
}

void handleStatusRequest() {
  sendJson(200, getStatusJson());
}

void handleDebugCaptureRequest() {
  sendJson(200, getDebugCaptureJson());
}

void handleButtonSendRequest() {
  if (!requireArg("buttonId")) return;

  const String buttonId = server.arg("buttonId");
  if (!isSupportedButton(buttonId)) {
    sendJsonResponse(404, false, "Unknown button.");
    return;
  }

  const uint16_t count = parseSendCount();
  const String message = sendNativeGreeButtonCode(buttonId, count);
  sendJsonResponse(200, true, message);
}

void handleRemoteResetRequest() {
  resetNativeGreeState();
  lastEventMessage = "Reset to defaults.";
  sendJsonResponse(200, true, lastEventMessage);
}

void handleNotFoundRequest() {
  sendJsonResponse(404, false, "Not found.");
}

void printHelp() {
  Serial.println();
  Serial.println(F("ESP32 Universal Gree Remote"));
  Serial.println(F("Routes:"));
  Serial.println(F("  /"));
  Serial.println(F("  /api/status"));
  Serial.println(F("  /api/debug/capture"));
  Serial.println(F("  /api/read-ir"));
  Serial.println(F("  /api/button/send?buttonId=power"));
  Serial.println(F("  /api/remote/clear"));
  Serial.println(F("Serial commands: status, help, reset"));
  Serial.println(F("Config: WIFI_USE_STATIC_IP and WIFI_PUBLIC_BASE_URL"));
  Serial.print(F("Debug IR reader: "));
  Serial.println(kEnableDebugIrReader ? F("enabled") : F("disabled"));
  Serial.println();
}

void printStatus() {
  Serial.println();
  Serial.println(F("Current status"));
  Serial.print(F("  TX pin: "));
  Serial.println(kSendPin);
  Serial.print(F("  RX pin: "));
  if (kEnableDebugIrReader) {
    Serial.println(kRecvPin);
  } else {
    Serial.println(F("disabled"));
  }
  Serial.print(F("  Wi-Fi mode: "));
  Serial.println(wifiModeLabel);
  Serial.print(F("  Address: "));
  Serial.println(wifiAddress);
  Serial.print(F("  Hostname: "));
  Serial.println(wifiHostLabel);
  Serial.print(F("  Local URL: "));
  Serial.println(getLocalBaseUrl());
  Serial.print(F("  Preferred URL: "));
  Serial.println(preferredBaseUrl);
  Serial.print(F("  mDNS active: "));
  Serial.println(mdnsActive ? F("yes") : F("no"));
  Serial.print(F("  Last state: "));
  Serial.println(lastNativeAcSummary);
  if (kEnableDebugIrReader) {
    Serial.print(F("  Last IR capture: "));
    if (hasDebugCapture) {
      Serial.print(debugProtocol);
      Serial.print(F(" / "));
      Serial.println(debugHexValue);
    } else {
      Serial.println(F("none"));
    }
  }
  Serial.println();
}

void handleCommand(const String& command) {
  if (command == "status") {
    printStatus();
    return;
  }
  if (command == "help") {
    printHelp();
    return;
  }
  if (command == "reset") {
    resetNativeGreeState();
    lastEventMessage = "Reset from serial.";
    Serial.println(lastEventMessage);
    return;
  }
  if (command.length() > 0) {
    Serial.println(F("Unknown command. Try: status, help, reset"));
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  const String command = Serial.readStringUntil('\n');
  String normalized = command;
  normalized.trim();
  normalized.toLowerCase();
  handleCommand(normalized);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRootRequest);
  server.on("/api/status", HTTP_GET, handleStatusRequest);
  server.on("/api/debug/capture", HTTP_GET, handleDebugCaptureRequest);
  server.on("/api/read-ir", HTTP_GET, handleDebugCaptureRequest);
  server.on("/api/button/send", HTTP_GET, handleButtonSendRequest);
  server.on("/api/remote/clear", HTTP_GET, handleRemoteResetRequest);
  server.onNotFound(handleNotFoundRequest);
  server.begin();
}

void startMdnsIfPossible() {
  mdnsActive = false;
  if (WiFi.status() != WL_CONNECTED) {
    updatePreferredBaseUrl();
    return;
  }

  if (MDNS.begin(kDeviceHostname)) {
    mdnsActive = true;
  }
  updatePreferredBaseUrl();
}

void connectToWifi() {
  wifiModeLabel = "offline";
  wifiAddress = "offline";
  wifiHostLabel = String(kDeviceHostname) + ".local";
  preferredBaseUrl = "offline";

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(kDeviceHostname);
  WiFi.setSleep(true);

  if (!isWifiConfigured()) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kFallbackApSsid, kFallbackApPassword);
    wifiModeLabel = "fallback_ap";
    wifiAddress = WiFi.softAPIP().toString();
    updatePreferredBaseUrl();
    lastEventMessage = "No Wi-Fi config. Fallback AP started.";
    Serial.println(lastEventMessage);
    return;
  }

  WiFi.mode(WIFI_STA);
  if (kUseStaticIp) {
    WiFi.config(kStaticIp, kGatewayIp, kSubnetMask, kDns1Ip, kDns2Ip);
  }
  WiFi.begin(kWifiSsid, kWifiPassword);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < kWifiConnectTimeoutMs) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiModeLabel = "station";
    wifiAddress = WiFi.localIP().toString();
    updatePreferredBaseUrl();
    lastEventMessage = String("Wi-Fi connected: ") + kWifiSsid;
    Serial.println(lastEventMessage);
    return;
  }

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kFallbackApSsid, kFallbackApPassword);
  wifiModeLabel = "fallback_ap";
  wifiAddress = WiFi.softAPIP().toString();
  updatePreferredBaseUrl();
  lastEventMessage = "Wi-Fi failed. Fallback AP started.";
  Serial.println(lastEventMessage);
}

void setup() {
  Serial.begin(kBaudRate);
  delay(200);

  preferencesReady = preferences.begin(kPreferencesNamespace, false);

  greeAc.begin();
  if (!loadSavedGreeState()) {
    resetNativeGreeState();
  }

  if (kEnableDebugIrReader) {
    irrecv.enableIRIn();
    debugLastEvent = "IR reader ready.";
  } else {
    debugLastEvent = "IR reader disabled in code.";
  }

  connectToWifi();
  startMdnsIfPossible();
  startWebServer();
  printHelp();
  printStatus();
}

void loop() {
  server.handleClient();
  handleSerial();
  handleDebugCapture();
}
