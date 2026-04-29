#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>

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
constexpr uint16_t kResetButtonPin = 0;  // GPIO0 (typically BOOT button on ESP32 dev boards)
constexpr uint32_t kBaudRate = 115200;
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint16_t kCaptureBufferSize = 1024;
constexpr uint8_t kCaptureTimeoutMs = 50;
// MQTT Configuration
constexpr char kMqttBrokerHost[] = "128.199.20.163";
constexpr uint16_t kMqttBrokerPort = 1883;
constexpr char kMqttUsername[] = "amiuser";
constexpr char kMqttPassword[] = "password";
constexpr char kMqttTopicCommand[] = "ir";
constexpr char kMqttTopicStatus[] = "ir/status";
// WiFi provisioning AP credentials (used when no saved WiFi or manual provisioning)
constexpr char kProvisioningApSsid[] = "UNIVERSAL IR";
constexpr char kProvisioningApPassword[] = "12345678";
// mDNS hostname - changed to irremote
constexpr char kDeviceHostname[] = "irremote";
constexpr char kPublicBaseUrl[] = WIFI_PUBLIC_BASE_URL;
constexpr bool kUseStaticIp = WIFI_USE_STATIC_IP != 0;
constexpr bool kEnableDebugIrReader = true;
constexpr char kPreferencesNamespace[] = "gree-remote";
constexpr char kPreferencesStateKey[] = "state";
constexpr char kPreferencesWifiSsidKey[] = "wifi_ssid";
constexpr char kPreferencesWifiPasswordKey[] = "wifi_pass";
// Button timing (in milliseconds)
constexpr uint32_t kResetButtonProvisioningTime = 5000;  // 5 seconds for provisioning
constexpr uint32_t kResetButtonWifiResetTime = 10000;    // 10 seconds to reset WiFi
constexpr uint32_t kButtonDebounceMs = 50;

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
WebServer *server = nullptr;
DNSServer dnsServer;
Preferences preferences;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void ensureWebServer() {
  if (!server) {
    server = new WebServer(kHttpPort);
  }
}

void resetWebServer() {
  if (server) {
    delete server;
  }
  server = new WebServer(kHttpPort);
}

// WiFi provisioning state
bool isProvisioningMode = false;
uint32_t buttonPressStartTime = 0;
bool buttonPressed = false;
String savedWifiSsid = "";
String savedWifiPassword = "";
// MQTT state
bool mqttConnected = false;
uint32_t lastMqttReconnectAttempt = 0;
const uint32_t kMqttReconnectInterval = 5000;  // Retry MQTT connection every 5 seconds

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

// ============================================================================
// WiFi Credential Management Functions
// ============================================================================

void saveWifiCredentials(const String& ssid, const String& password) {
  if (!preferencesReady) return;
  preferences.putString(kPreferencesWifiSsidKey, ssid);
  preferences.putString(kPreferencesWifiPasswordKey, password);
  savedWifiSsid = ssid;
  savedWifiPassword = password;
  Serial.print(F("WiFi credentials saved: "));
  Serial.println(ssid);
}

bool loadWifiCredentials() {
  if (!preferencesReady) return false;
  savedWifiSsid = preferences.getString(kPreferencesWifiSsidKey, "");
  savedWifiPassword = preferences.getString(kPreferencesWifiPasswordKey, "");
  return savedWifiSsid.length() > 0;
}

void clearWifiCredentials() {
  if (!preferencesReady) return;
  preferences.remove(kPreferencesWifiSsidKey);
  preferences.remove(kPreferencesWifiPasswordKey);
  savedWifiSsid = "";
  savedWifiPassword = "";
  Serial.println(F("WiFi credentials cleared."));
}

// ============================================================================
// Forward Declarations
// ============================================================================
bool isSupportedButton(const String& buttonId);
String sendNativeGreeButtonCode(const String& buttonId, uint16_t count);
bool saveCurrentGreeState();
String getGreeStateJson();
String escapeJson(const String& input);
void publishStatus();
void handleButtonCommand(const String& buttonId);
void onMqttMessage(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

// ============================================================================
// MQTT Functions
// ============================================================================

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  // Parse MQTT command
  if (strcmp(topic, kMqttTopicCommand) != 0) return;
  
  String jsonStr = "";
  for (unsigned int i = 0; i < length; i++) {
    jsonStr += (char)payload[i];
  }
  jsonStr.trim();
  
  Serial.print(F("MQTT Command received: "));
  Serial.println(jsonStr);
  
  // Simple button parsing - expect {"button":"<button_id>"}
  if (jsonStr.indexOf("\"button\"") >= 0) {
    int start = jsonStr.indexOf("\":\"") + 3;
    int end = jsonStr.indexOf("\"", start);
    if (start > 0 && end > start) {
      String buttonId = jsonStr.substring(start, end);
      handleButtonCommand(buttonId);
    }
  } else if (jsonStr.length() > 0) {
    handleButtonCommand(jsonStr);
  }
}

void handleButtonCommand(const String& buttonId) {
  if (!isSupportedButton(buttonId)) {
    Serial.print(F("Unknown button: "));
    Serial.println(buttonId);
    return;
  }
  
  // Use existing button handling logic
  lastEventMessage = sendNativeGreeButtonCode(buttonId, 1);
  
  // Save state
  saveCurrentGreeState();
  
  // Publish updated status
  publishStatus();
}

void publishStatus() {
  if (!mqttConnected) return;
  
  // Build JSON status using existing gree state JSON
  String json = "{\"gree_state\":" + getGreeStateJson() + ",\"last_event\":\"" + escapeJson(lastEventMessage) + "\"}";
  
  mqttClient.publish(kMqttTopicStatus, json.c_str(), true);
}

void reconnectMQTT() {
  // Don't attempt if provisioning or WiFi not connected
  if (isProvisioningMode || WiFi.status() != WL_CONNECTED) return;
  
  // Rate limit connection attempts
  uint32_t now = millis();
  if (now - lastMqttReconnectAttempt < kMqttReconnectInterval) return;
  lastMqttReconnectAttempt = now;
  
  // Create client ID
  String clientId = "esp32-gree-";
  clientId += String(random(0xffff), HEX);
  
  Serial.print(F("Connecting to MQTT broker..."));
  
  if (mqttClient.connect(clientId.c_str(), kMqttUsername, kMqttPassword)) {
    Serial.println(F(" Connected!"));
    mqttConnected = true;
    mqttClient.subscribe(kMqttTopicCommand);
    publishStatus();
  } else {
    Serial.println(F(" Failed!"));
    mqttConnected = false;
  }
}

// ============================================================================
// WiFi Provisioning Web Page
// ============================================================================

String getProvisioningPage() {
  return R"HTML(<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>UNIVERSAL IR WiFi Setup</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      padding: 40px;
      max-width: 500px;
      width: 100%;
    }
    h1 {
      color: #333;
      margin-bottom: 10px;
      text-align: center;
      font-size: 28px;
    }
    .subtitle {
      color: #666;
      text-align: center;
      margin-bottom: 30px;
      font-size: 14px;
    }
    .status {
      background: #f0f4ff;
      border-left: 4px solid #667eea;
      padding: 15px;
      margin-bottom: 20px;
      border-radius: 8px;
      display: none;
    }
    .status.show { display: block; }
    .status.success { 
      background: #f0fdf4;
      border-left-color: #10b981;
    }
    .status.error { 
      background: #fef2f2;
      border-left-color: #ef4444;
    }
    .status.success { color: #065f46; }
    .status.error { color: #7f1d1d; }
    .status.info { color: #1e40af; }
    
    .section {
      margin-bottom: 25px;
    }
    .section-title {
      color: #333;
      font-weight: 600;
      margin-bottom: 12px;
      font-size: 14px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    
    .networks {
      border: 1px solid #e5e7eb;
      border-radius: 12px;
      max-height: 250px;
      overflow-y: auto;
      margin-bottom: 15px;
    }
    .network {
      padding: 12px 15px;
      border-bottom: 1px solid #e5e7eb;
      cursor: pointer;
      transition: background 0.2s;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .network:last-child { border-bottom: none; }
    .network:hover { background: #f9fafb; }
    .network.active {
      background: #f0f4ff;
      color: #667eea;
      font-weight: 600;
    }
    .network-name { flex: 1; }
    .network-signal { font-size: 12px; color: #999; }
    
    input[type="text"], input[type="password"] {
      width: 100%;
      padding: 12px;
      border: 1px solid #e5e7eb;
      border-radius: 8px;
      font-size: 14px;
      margin-bottom: 10px;
      transition: border 0.2s;
    }
    input[type="text"]:focus, input[type="password"]:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    
    button {
      width: 100%;
      padding: 12px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
      margin-top: 10px;
    }
    button:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
    }
    button:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    button.secondary {
      background: #f3f4f6;
      color: #333;
      margin-top: 5px;
    }
    button.secondary:hover:not(:disabled) {
      background: #e5e7eb;
      box-shadow: none;
    }
    
    .help-text {
      font-size: 12px;
      color: #666;
      margin-top: 8px;
      line-height: 1.4;
    }
    
    .spinner {
      display: inline-block;
      width: 16px;
      height: 16px;
      border: 2px solid #f3f3f3;
      border-top: 2px solid #667eea;
      border-radius: 50%;
      animation: spin 1s linear infinite;
      margin-right: 8px;
      vertical-align: middle;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌐 UNIVERSAL IR</h1>
    <div class="subtitle">WiFi Setup - Connect to your home network</div>
    
    <div class="status" id="status"></div>
    
    <div class="section">
      <div class="section-title">Available Networks</div>
      <div class="networks" id="networks">
        <div class="network" style="color: #999; justify-content: center; padding: 30px 15px;">
          <span class="spinner"></span>Scanning networks...
        </div>
      </div>
    </div>
    
    <div class="section">
      <div class="section-title">Network Name (SSID)</div>
      <input type="text" id="ssid" placeholder="Enter network name" autocomplete="off">
      <div class="help-text">Select from list or type manually</div>
    </div>
    
    <div class="section">
      <div class="section-title">Password</div>
      <input type="password" id="password" placeholder="Enter WiFi password" autocomplete="off">
      <div class="help-text">Leave empty for open networks</div>
    </div>
    
    <button onclick="connectWifi()" id="connectBtn">Connect to WiFi</button>
    <button class="secondary" onclick="rescanNetworks()">Rescan Networks</button>
  </div>

  <script>
    const statusEl = document.getElementById('status');
    
    function showStatus(msg, type) {
      statusEl.textContent = msg;
      statusEl.className = 'status show ' + type;
    }
    
    async function scanNetworks() {
      try {
        const response = await fetch('/api/scan-networks');
        const data = await response.json();
        
        if (!data.networks || data.networks.length === 0) {
          document.getElementById('networks').innerHTML = 
            '<div class="network" style="color: #999; justify-content: center;">No networks found</div>';
          return;
        }
        
        const networksHtml = data.networks.map(net => `
          <div class="network" onclick="selectNetwork('${net.ssid}')">
            <span class="network-name">${escapeHtml(net.ssid)}</span>
            <span class="network-signal">${net.rssi}dBm</span>
          </div>
        `).join('');
        
        document.getElementById('networks').innerHTML = networksHtml;
      } catch (e) {
        showStatus('Failed to scan networks', 'error');
      }
    }
    
    function selectNetwork(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('password').focus();
    }
    
    async function connectWifi() {
      const ssid = document.getElementById('ssid').value.trim();
      const password = document.getElementById('password').value;
      
      if (!ssid) {
        showStatus('Please enter a network name', 'error');
        return;
      }
      
      document.getElementById('connectBtn').disabled = true;
      showStatus('Connecting to WiFi...', 'info');
      
      try {
        const response = await fetch('/api/connect-wifi', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ ssid, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
          showStatus('✓ Connected! Redirecting...', 'success');
          setTimeout(() => {
            window.location.href = 'http://irremote.local';
          }, 2000);
        } else {
          showStatus('Failed to connect: ' + (data.error || 'Unknown error'), 'error');
          document.getElementById('connectBtn').disabled = false;
        }
      } catch (e) {
        showStatus('Connection failed: ' + e.message, 'error');
        document.getElementById('connectBtn').disabled = false;
      }
    }
    
    function rescanNetworks() {
      document.getElementById('networks').innerHTML = 
        '<div class="network" style="color: #999; justify-content: center; padding: 30px 15px;"><span class="spinner"></span>Scanning...</div>';
      scanNetworks();
    }
    
    function escapeHtml(text) {
      const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
      return text.replace(/[&<>"']/g, m => map[m]);
    }
    
    // Initial scan
    scanNetworks();
    setInterval(scanNetworks, 10000); // Rescan every 10 seconds
  </script>
</body>
</html>)HTML";
}

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
      case '"':
        escaped += F("\\\"");
        break;
      case '\\':
        escaped += F("\\\\");
        break;
      case '\b':
        escaped += F("\\b");
        break;
      case '\f':
        escaped += F("\\f");
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
  return F("<!doctype html><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
           "<title>ESP32 Gree Remote</title><body style=\"font-family:sans-serif\">"
           "<h1>ESP32 Gree Remote</h1>"
           "<p>Device control is handled by MQTT topic <code>ir</code>.</p>"
           "<p>Use the GitHub Pages remote UI after provisioning WiFi.</p>"
           "</body>");
}

void sendJson(int statusCode, const String& payload) {
  server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server->sendHeader("Access-Control-Allow-Origin", "*");
  server->send(statusCode, "application/json", payload);
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
  if (server->hasArg(name)) return true;
  sendJsonResponse(400, false, String("Missing: ") + name);
  return false;
}

uint16_t parseSendCount() {
  if (!server->hasArg("count")) return 1;
  const long count = server->arg("count").toInt();
  if (count < 1) return 1;
  if (count > 5) return 5;
  return static_cast<uint16_t>(count);
}

void handleRootRequest() {
  server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server->send(200, "text/html; charset=utf-8", getRootPage());
}

void handleStatusRequest() {
  sendJson(200, getStatusJson());
}

void handleDebugCaptureRequest() {
  sendJson(200, getDebugCaptureJson());
}

void handleButtonSendRequest() {
  if (!requireArg("buttonId")) return;

  const String buttonId = server->arg("buttonId");
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
  Serial.println(F("Control: MQTT topic ir"));
  Serial.print(F("Broker: "));
  Serial.print(kMqttBrokerHost);
  Serial.print(F(":"));
  Serial.println(kMqttBrokerPort);
  Serial.println(F("Serial commands: status, help, reset"));
  Serial.println(F("Config: WIFI_USE_STATIC_IP and WIFI_PUBLIC_BASE_URL"));
  Serial.print(F("Debug IR reader: "));
  Serial.println(kEnableDebugIrReader ? F("enabled") : F("disabled"));
  Serial.println(F("WiFi Provisioning: Press RESET button"));
  Serial.println(F("  5 seconds: Enter WiFi provisioning mode"));
  Serial.println(F("  10 seconds: Clear WiFi credentials and restart"));
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
  Serial.print(F("  MQTT broker: "));
  Serial.print(kMqttBrokerHost);
  Serial.print(F(":"));
  Serial.println(kMqttBrokerPort);
  Serial.print(F("  MQTT command topic: "));
  Serial.println(kMqttTopicCommand);
  Serial.print(F("  MQTT status topic: "));
  Serial.println(kMqttTopicStatus);
  Serial.print(F("  MQTT connected: "));
  Serial.println(mqttConnected ? F("yes") : F("no"));
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

// ============================================================================
// WiFi Provisioning Handler Functions
// ============================================================================

void handleProvisioningRoot() {
  server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server->send(200, "text/html; charset=utf-8", getProvisioningPage());
}

void handleScanNetworks() {
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  
  String payload = "{\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) payload += ",";
    payload += "{\"ssid\":\"";
    payload += WiFi.SSID(i);
    payload += "\",\"rssi\":";
    payload += WiFi.RSSI(i);
    payload += "}";
  }
  payload += "]}";
  
  sendJson(200, payload);
}

void handleConnectWifi() {
  if (server->method() != HTTP_POST) {
    sendJsonResponse(405, false, "Method not allowed");
    return;
  }
  
  String body = server->arg("plain");
  
  // Simple JSON parsing
  int ssidStart = body.indexOf("\"ssid\":");
  int ssidQuote1 = body.indexOf("\"", ssidStart + 8);
  int ssidQuote2 = body.indexOf("\"", ssidQuote1 + 1);
  String ssid = body.substring(ssidQuote1 + 1, ssidQuote2);
  
  int passStart = body.indexOf("\"password\":");
  int passQuote1 = body.indexOf("\"", passStart + 11);
  int passQuote2 = body.indexOf("\"", passQuote1 + 1);
  String password = body.substring(passQuote1 + 1, passQuote2);
  
  if (ssid.length() == 0) {
    String response = "{\"success\":false,\"error\":\"SSID required\"}";
    sendJson(400, response);
    return;
  }
  
  // Save credentials
  saveWifiCredentials(ssid, password);
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait for connection
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    timeout--;
  }
  
  String response;
  if (WiFi.status() == WL_CONNECTED) {
    lastEventMessage = String("WiFi connected: ") + ssid;
    Serial.println(lastEventMessage);
    response = "{\"success\":true,\"message\":\"Connected to WiFi\"}";
    
    // Exit provisioning mode and restart
    isProvisioningMode = false;
    delay(1000);
    ESP.restart();
  } else {
    response = "{\"success\":false,\"error\":\"Failed to connect to WiFi\"}";
  }
  
  sendJson(200, response);
}

void startProvisioning() {
  isProvisioningMode = true;
  Serial.println(F("Starting WiFi Provisioning Mode..."));
  Serial.print(F("Connect to SSID: "));
  Serial.println(kProvisioningApSsid);
  Serial.print(F("Password: "));
  Serial.println(kProvisioningApPassword);
  Serial.println(F("Then open browser to: http://192.168.4.1"));
  
  // Stop current WiFi and start AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kProvisioningApSsid, kProvisioningApPassword);
  
  // Start DNS server for captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  wifiModeLabel = "provisioning";
  wifiAddress = WiFi.softAPIP().toString();
  lastEventMessage = "WiFi Provisioning Mode Active";
  
  // Stop mDNS if running
  mdnsActive = false;
  
  // Update web server routes for provisioning
  resetWebServer();
  server->on("/", HTTP_GET, handleProvisioningRoot);
  server->on("/api/scan-networks", HTTP_GET, handleScanNetworks);
  server->on("/api/connect-wifi", HTTP_POST, handleConnectWifi);
  server->onNotFound(handleProvisioningRoot);  // Redirect all requests to main page
  server->begin();
}

// ============================================================================
// Button Handling
// ============================================================================

void handleResetButton() {
  // Check if button is pressed (GPIO0 is active low on most ESP32 boards)
  bool pressed = (digitalRead(kResetButtonPin) == LOW);
  
  if (pressed && !buttonPressed) {
    // Button just pressed
    buttonPressed = true;
    buttonPressStartTime = millis();
  } else if (!pressed && buttonPressed) {
    // Button just released
    buttonPressed = false;
    uint32_t pressDuration = millis() - buttonPressStartTime;
    
    Serial.print(F("Reset button press duration: "));
    Serial.print(pressDuration);
    Serial.println(F("ms"));
    
    if (pressDuration >= kResetButtonWifiResetTime) {
      // 10 seconds - clear WiFi and provisioning
      Serial.println(F("Long press detected - clearing WiFi credentials"));
      clearWifiCredentials();
      lastEventMessage = "WiFi credentials cleared. Restarting...";
      delay(500);
      ESP.restart();
    } else if (pressDuration >= kResetButtonProvisioningTime) {
      // 5 seconds - start provisioning
      Serial.println(F("Medium press detected - starting WiFi provisioning"));
      startProvisioning();
    }
  }
}

void startWebServer() {
  if (!isProvisioningMode) return;
}

void startMdnsIfPossible() {
  mdnsActive = false;
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

  // Check for saved WiFi credentials in NVS
  bool hasSavedCredentials = loadWifiCredentials();
  bool hasConfigCredentials = strlen(kWifiSsid) > 0;

  if (!hasSavedCredentials && !hasConfigCredentials) {
    // No credentials configured - start provisioning mode
    Serial.println(F("No WiFi credentials found. Starting provisioning mode."));
    startProvisioning();
    return;
  }

  // Try to connect with saved credentials first
  if (hasSavedCredentials) {
    Serial.print(F("Attempting to connect to saved WiFi: "));
    Serial.println(savedWifiSsid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedWifiSsid.c_str(), savedWifiPassword.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < kWifiConnectTimeoutMs) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiModeLabel = "station";
      wifiAddress = WiFi.localIP().toString();
      updatePreferredBaseUrl();
      lastEventMessage = String("Wi-Fi connected: ") + savedWifiSsid;
      Serial.println(lastEventMessage);
      return;
    }
    
    Serial.println(F("Failed to connect to saved WiFi."));
  }

  // Fall back to hardcoded credentials if available
  if (hasConfigCredentials) {
    Serial.print(F("Attempting to connect to configured WiFi: "));
    Serial.println(kWifiSsid);
    
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
  }

  // Connection failed - start provisioning
  Serial.println(F("WiFi connection failed. Starting provisioning mode."));
  startProvisioning();
}

void setup() {
  Serial.begin(kBaudRate);
  delay(200);

  // Initialize reset button
  pinMode(kResetButtonPin, INPUT_PULLUP);

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

  // Initialize MQTT client
  mqttClient.setServer(kMqttBrokerHost, kMqttBrokerPort);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);

  connectToWifi();
  
  startMdnsIfPossible();
  
  startWebServer();
  printHelp();
  printStatus();
}

void loop() {
  if (server) server->handleClient();
  
  // Handle DNS for captive portal during provisioning
  if (isProvisioningMode) {
    dnsServer.processNextRequest();
  }
  
  // Handle MQTT connection and messaging
  if (!isProvisioningMode && WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    } else {
      mqttClient.loop();
    }
  }
  
  handleResetButton();
  handleSerial();
  handleDebugCapture();
}
