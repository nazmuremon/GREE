# Code Snippets & Examples for Blog Posts

This document contains ready-to-use code examples with explanations, formatted for blogging.

---

## 1. Basic Setup for Beginners

### What It Does

This shows the absolute minimum setup to get your ESP32 communicating with a Gree AC.

### Code

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <IRsend.h>
#include <ir_Gree.h>

// Configuration
const uint16_t kSendPin = 12;           // GPIO12 = IR LED
const char* ssid = "YOUR_NETWORK";      // Your WiFi name
const char* password = "YOUR_PASSWORD"; // Your WiFi password

// Create IR transmitter object for Gree AC
IRGreeAC greeAc(kSendPin, gree_ac_remote_model_t::YAW1F);

void setup() {
  Serial.begin(115200);
  delay(200);
  
  // Initialize IR transmitter
  greeAc.begin();
  greeAc.stateReset();
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
  }
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    // Keep IR receiver ready
    delay(100);
  }
}

// Function to send a command
void sendCommand(uint8_t mode, uint8_t temp) {
  greeAc.setPower(true);
  greeAc.setMode(mode);      // kGreeCool, kGreeHeat, etc.
  greeAc.setTemp(temp);      // 16-30°C
  greeAc.setFan(kGreeFanAuto);
  
  Serial.print("Sending command: Mode=");
  Serial.print(mode);
  Serial.print(", Temp=");
  Serial.println(temp);
  
  greeAc.send();  // Transmit via IR LED on GPIO12
}
```

### How to Use It

```cpp
// In setup() or loop():
sendCommand(kGreeCool, 24);  // Cool mode, 24°C
```

---

## 2. WiFi Provisioning Without Hardcoding

### What It Does

This allows users to press a button to set up WiFi without editing code.

### Code

```cpp
#include <Preferences.h>
#include <DNSServer.h>
#include <WebServer.h>

const uint16_t kResetButtonPin = 0;     // GPIO0 = BOOT button
const uint32_t kProvisioningTime = 5000; // 5 seconds
const char* kProvisioningSSID = "UNIVERSAL IR";
const char* kProvisioningPassword = "12345678";

Preferences preferences;
DNSServer dnsServer;
WebServer server(80);

void saveWifiCredentials(const String& ssid, const String& password) {
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_pass", password);
  Serial.print("✓ Saved WiFi: ");
  Serial.println(ssid);
}

bool loadWifiCredentials(String& ssid, String& password) {
  ssid = preferences.getString("wifi_ssid", "");
  password = preferences.getString("wifi_pass", "");
  return ssid.length() > 0;
}

void startProvisioningMode() {
  Serial.println("Starting WiFi Provisioning Mode");
  Serial.println("SSID: UNIVERSAL IR");
  Serial.println("Password: -12345678");
  Serial.println("Open: http://192.168.4.1");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kProvisioningSSID, kProvisioningPassword);
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  // Setup provisioning web page
  server.on("/", handleProvisioningPage);
  server.begin();
}

void handleResetButton() {
  static bool buttonPressed = false;
  static uint32_t pressTime = 0;
  
  bool isPressed = (digitalRead(kResetButtonPin) == LOW);
  
  if (isPressed && !buttonPressed) {
    buttonPressed = true;
    pressTime = millis();
  }
  else if (!isPressed && buttonPressed) {
    buttonPressed = false;
    uint32_t duration = millis() - pressTime;
    
    // 5-10 seconds: Enter provisioning
    if (duration >= kProvisioningTime && duration < 10000) {
      startProvisioningMode();
    }
    // 10+ seconds: Clear WiFi and restart
    else if (duration >= 10000) {
      Serial.println("Clearing WiFi credentials...");
      preferences.remove("wifi_ssid");
      preferences.remove("wifi_pass");
      ESP.restart();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(kResetButtonPin, INPUT_PULLUP);
  
  preferences.begin("gree-remote", false);
  
  // Load saved WiFi
  String ssid, password;
  if (loadWifiCredentials(ssid, password)) {
    Serial.println("Connecting to saved WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    Serial.println("No saved WiFi, starting provisioning...");
    startProvisioningMode();
  }
}

void loop() {
  handleResetButton();
  server.handleClient();
  dnsServer.processOneRequest();
  delay(10);
}
```

### Button Timing Reference

```
Press Duration     | Action
------------------|---------------------------
< 5 seconds        | No action
5-9 seconds        | Enter WiFi provisioning
≥ 10 seconds       | Clear WiFi & restart
```

---

## 3. Web API Endpoint

### What It Does

Allows other devices to control your AC via HTTP requests.

### Code

```cpp
#include <WebServer.h>
#include <ir_Gree.h>

WebServer server(80);
IRGreeAC greeAc(12, gree_ac_remote_model_t::YAW1F);

// JSON response helper
void sendJsonStatus(int code, bool success, const String& message) {
  String json = "{";
  json += "\"success\":" + String(success ? "true" : "false") + ",";
  json += "\"message\":\"" + message + "\",";
  json += "\"power\":" + String(greeAc.getPower() ? "true" : "false") + ",";
  json += "\"temperature\":" + String(greeAc.getTemp());
  json += "}";
  
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", json);
}

// GET /api/button/send?buttonId=power
void handleButtonSend() {
  if (!server.hasArg("buttonId")) {
    sendJsonStatus(400, false, "Missing: buttonId");
    return;
  }
  
  String buttonId = server.arg("buttonId");
  
  // Power button
  if (buttonId == "power") {
    greeAc.setPower(!greeAc.getPower());
    greeAc.send();
    sendJsonStatus(200, true, "Power toggled");
  }
  // Temperature up
  else if (buttonId == "temp_up") {
    uint8_t temp = greeAc.getTemp();
    if (temp < 30) {
      greeAc.setTemp(temp + 1);
      greeAc.setPower(true);
      greeAc.send();
      sendJsonStatus(200, true, "Temperature increased");
    } else {
      sendJsonStatus(400, false, "Max temperature reached");
    }
  }
  // Temperature down
  else if (buttonId == "temp_down") {
    uint8_t temp = greeAc.getTemp();
    if (temp > 16) {
      greeAc.setTemp(temp - 1);
      greeAc.send();
      sendJsonStatus(200, true, "Temperature decreased");
    } else {
      sendJsonStatus(400, false, "Min temperature reached");
    }
  }
  // Mode cycle
  else if (buttonId == "mode") {
    uint8_t mode = greeAc.getMode();
    uint8_t nextMode;
    
    switch (mode) {
      case kGreeCool:  nextMode = kGreeDry;   break;
      case kGreeDry:   nextMode = kGreeFan;   break;
      case kGreeFan:   nextMode = kGreeHeat;  break;
      case kGreeHeat:  nextMode = kGreeAuto;  break;
      default:         nextMode = kGreeCool;  break;
    }
    
    greeAc.setMode(nextMode);
    greeAc.setPower(true);
    greeAc.send();
    sendJsonStatus(200, true, "Mode changed");
  }
  else {
    sendJsonStatus(404, false, "Unknown button: " + buttonId);
  }
}

// GET /api/status
void handleStatus() {
  String json = "{";
  json += "\"power\":" + String(greeAc.getPower() ? "true" : "false") + ",";
  json += "\"temperature\":" + String(greeAc.getTemp()) + ",";
  json += "\"fan\":\"" + String(greeAc.getFan()) + "\",";
  json += "\"sleep\":" + String(greeAc.getSleep() ? "true" : "false") + ",";
  json += "\"turbo\":" + String(greeAc.getTurbo() ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void setupWebServer() {
  server.on("/api/button/send", HTTP_GET, handleButtonSend);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.begin();
  Serial.println("Web server started");
}
```

### Usage Examples

```bash
# Get current status
curl http://192.168.1.60/api/status

# Send power button
curl "http://192.168.1.60/api/button/send?buttonId=power"

# Increase temperature
curl "http://192.168.1.60/api/button/send?buttonId=temp_up"

# Change mode
curl "http://192.168.1.60/api/button/send?buttonId=mode"
```

### Expected Response

```json
{
  "success": true,
  "message": "Power toggled",
  "power": false,
  "temperature": 24
}
```

---

## 4. IR Signal Capture & Debug

### What It Does

Captures IR signals from your existing remote for debugging.

### Code

```cpp
#include <IRrecv.h>

const uint16_t kRecvPin = 15;  // GPIO15 = IR Receiver
const uint16_t kBufferSize = 1024;

IRrecv irrecv(kRecvPin, kBufferSize, 50, true);
decode_results results;

struct DebugCapture {
  String protocol;
  uint16_t bits;
  String hex_value;
  uint16_t raw_length;
  unsigned long timestamp;
};

DebugCapture lastCapture;

void initIRReceiver() {
  irrecv.enableIRIn();
  Serial.println("IR Receiver enabled on GPIO15");
}

void handleIRCapture() {
  if (irrecv.decode(&results)) {
    // Store the capture
    lastCapture.protocol = typeToString(results.decode_type);
    lastCapture.bits = results.bits;
    lastCapture.hex_value = resultToHexidecimal(&results);
    lastCapture.raw_length = results.rawlen;
    lastCapture.timestamp = millis();
    
    // Print to serial
    Serial.println("\n=== IR SIGNAL CAPTURED ===");
    Serial.print("Protocol: ");
    Serial.println(lastCapture.protocol);
    Serial.print("Bits: ");
    Serial.println(lastCapture.bits);
    Serial.print("Hex: ");
    Serial.println(lastCapture.hex_value);
    Serial.print("Raw Length: ");
    Serial.println(lastCapture.raw_length);
    Serial.println("========================\n");
    
    irrecv.resume();  // Ready for next signal
  }
}

// JSON endpoint for capture data
void handleDebugCapture() {
  String json = "{";
  json += "\"protocol\":\"" + lastCapture.protocol + "\",";
  json += "\"bits\":" + String(lastCapture.bits) + ",";
  json += "\"hex\":\"" + lastCapture.hex_value + "\",";
  json += "\"raw_length\":" + String(lastCapture.raw_length);
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  initIRReceiver();
}

void loop() {
  handleIRCapture();
  delay(50);
}
```

### Using the Captured Data

```
1. Point your original remote at the ESP32
2. Press a button on the remote
3. The capture data appears in serial monitor
4. Use hex value to verify the command was received
```

---

## 5. State Persistence (Save to Flash)

### What It Does

Saves and restores AC state even after power loss.

### Code

```cpp
#include <Preferences.h>
#include <ir_Gree.h>

const uint8_t kGreeStateLength = 14;  // YAW1F state is 14 bytes
Preferences preferences;
IRGreeAC greeAc(12, gree_ac_remote_model_t::YAW1F);

void saveACState() {
  const uint8_t* rawState = greeAc.getRaw();
  
  size_t written = preferences.putBytes(
    "ac_state",           // Key
    rawState,             // Data to save
    kGreeStateLength      // Length
  );
  
  if (written == kGreeStateLength) {
    Serial.println("✓ AC state saved to flash");
  } else {
    Serial.println("✗ Failed to save AC state");
  }
}

bool loadACState() {
  if (preferences.getBytesLength("ac_state") != kGreeStateLength) {
    Serial.println("No saved state found");
    return false;
  }
  
  uint8_t rawState[kGreeStateLength];
  preferences.getBytes("ac_state", rawState, kGreeStateLength);
  
  // Verify checksum
  if (!IRGreeAC::validChecksum(rawState, kGreeStateLength)) {
    Serial.println("✗ Saved state is corrupted");
    return false;
  }
  
  // Restore the state
  greeAc.setRaw(rawState);
  Serial.println("✓ AC state restored from flash");
  return true;
}

void setup() {
  Serial.begin(115200);
  preferences.begin("gree-remote", false);
  
  greeAc.begin();
  
  // Try to restore saved state
  if (!loadACState()) {
    // Set defaults if no saved state
    greeAc.stateReset();
    greeAc.setPower(false);
    greeAc.setTemp(24);
    greeAc.setMode(kGreeAuto);
    Serial.println("Using default AC state");
  }
}

// Every time you change AC state:
void sendButtonCommand(const String& buttonId) {
  if (buttonId == "power") {
    greeAc.setPower(!greeAc.getPower());
  } else if (buttonId == "temp_up") {
    greeAc.setTemp(greeAc.getTemp() + 1);
  }
  // ... other buttons ...
  
  greeAc.send();
  saveACState();  // Save after every command
}
```

### How It Works

```
Power ON → Load saved state → Display matches last state
   ↓
Send command → Update state → Save to flash
   ↓
Power OFF → (state saved in flash)
   ↓
Power ON → Load saved state → Display matches
```

---

## 6. mDNS Setup (irremote.local)

### What It Does

Makes your device accessible at `http://irremote.local` instead of remembering IP addresses.

### Code

```cpp
#include <ESPmDNS.h>
#include <WiFi.h>

const char* hostname = "irremote";  // Will become irremote.local

void setupMDNS() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping mDNS");
    return;
  }
  
  // Start mDNS responder
  if (!MDNS.begin(hostname)) {
    Serial.println("Error setting up mDNS responder");
    return;
  }
  
  // Add HTTP service
  MDNS.addService("http", "tcp", 80);
  
  Serial.print("✓ mDNS started: http://");
  Serial.print(hostname);
  Serial.println(".local/");
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi first
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  
  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Setup mDNS
    setupMDNS();
  }
}

void loop() {
  // mDNS needs to be updated periodically
  MDNS.update();
  delay(1000);
}
```

### How to Use

```
Instead of:   http://192.168.1.60/
Use:          http://irremote.local/

Much easier to remember and works even if IP changes!
```

---

## 7. Complete Minimal Example

### What It Does

Combines everything into one file for beginners.

### Code

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <IRsend.h>
#include <ir_Gree.h>

// ===== CONFIGURATION =====
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const uint16_t irLedPin = 12;

// ===== OBJECTS =====
IRGreeAC greeAc(irLedPin, gree_ac_remote_model_t::YAW1F);
WebServer server(80);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(200);
  
  // IR transmitter
  greeAc.begin();
  greeAc.stateReset();
  greeAc.setPower(false);
  greeAc.setTemp(24);
  
  // WiFi
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);
  
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.println(WiFi.localIP());
    
    // mDNS
    MDNS.begin("irremote");
    MDNS.addService("http", "tcp", 80);
    Serial.println("✓ Access at: http://irremote.local");
  }
  
  // Web server
  server.on("/api/power", handlePower);
  server.on("/api/temp_up", handleTempUp);
  server.on("/api/temp_down", handleTempDown);
  server.on("/api/status", handleStatus);
  server.begin();
}

// ===== WEB HANDLERS =====
void handlePower() {
  greeAc.setPower(!greeAc.getPower());
  greeAc.send();
  server.send(200, "application/json", "{\"success\":true}");
}

void handleTempUp() {
  if (greeAc.getTemp() < 30) {
    greeAc.setTemp(greeAc.getTemp() + 1);
    greeAc.setPower(true);
    greeAc.send();
  }
  server.send(200, "application/json", "{\"success\":true}");
}

void handleTempDown() {
  if (greeAc.getTemp() > 16) {
    greeAc.setTemp(greeAc.getTemp() - 1);
    greeAc.send();
  }
  server.send(200, "application/json", "{\"success\":true}");
}

void handleStatus() {
  String json = "{\"power\":" + String(greeAc.getPower() ? "true" : "false") + 
                ",\"temperature\":" + String(greeAc.getTemp()) + "}";
  server.send(200, "application/json", json);
}

// ===== MAIN LOOP =====
void loop() {
  server.handleClient();
  MDNS.update();
  delay(10);
}
```

### API Endpoints

```bash
http://irremote.local/api/power          # Toggle power
http://irremote.local/api/temp_up        # Increase temp
http://irremote.local/api/temp_down      # Decrease temp
http://irremote.local/api/status         # Get status
```

---

## Usage Tips for Blog

1. **Show in context:** Always explain what each line does
2. **Highlight important parts:** Use bold or quotes
3. **Include output:** Show what serial monitor will display
4. **Provide examples:** Before/after code comparisons
5. **Link libraries:** Point to GitHub repos
6. **Add warnings:** GPIO conflicts, power consumption, etc.

---

## Common Copy-Paste Issues

### Issue: WiFi credentials not connecting

**Check:**
```cpp
// Make sure these match EXACTLY (case-sensitive):
const char* ssid = "MyWiFi";           // ← Your exact network name
const char* password = "MyPassword123"; // ← Your exact password
```

### Issue: IR LED not working

**Check:**
```cpp
// GPIO12 must be free (not used by anything else)
const uint16_t irLedPin = 12;  // Try 13 or 14 if conflicts

// Power supply must be adequate
// IR LED needs 5V through resistor
// Check transistor connections
```

### Issue: IRremoteESP8266 library not found

**Solution:**
```
Sketch → Include Library → Manage Libraries
Search: "IRremoteESP8266"
Install version 2.8.6 or higher
```

---

