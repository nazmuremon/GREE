# Build Your Own Smart WiFi Gree Air Conditioner Remote with ESP32

## Transform Your AC Control: DIY WiFi Remote for Gree Air Conditioners

![ESP32 Universal Gree Remote - Web UI Preview]

### What You'll Learn

In this comprehensive guide, I'll show you how to build a **smart WiFi remote for your Gree air conditioner** using an ESP32 microcontroller. By the end of this project, you'll have a web-based remote accessible from your phone, tablet, or laptop—no cloud services required, no subscriptions needed, just pure local WiFi control.

**Difficulty Level:** Intermediate  
**Time Required:** 2-4 hours  
**Skills Required:** Basic electronics, Arduino programming, WiFi networking

---

## Introduction: Why Build This?

Have you ever wanted to control your air conditioner from your phone, but didn't trust a cloud service with your home network? Or maybe the original remote is lost, and a replacement is expensive? This project solves both problems.

The **ESP32 Universal Gree Remote** is a DIY smart home device that:
- ✅ Turns your ESP32 into a **complete AC remote** 
- ✅ Works entirely on **local WiFi** (no internet required)
- ✅ Remembers your AC settings after power loss
- ✅ Provides a modern, mobile-friendly web interface
- ✅ Lets you capture and debug IR signals
- ✅ Easy WiFi setup with a 5-second button press

**Best of all?** The entire project uses open-source libraries and costs less than $15 in components.

---

## What You'll Need

### Electronics Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| **ESP32 Development Board** | 1 | Any variant (ESP32-DevKit-C recommended) |
| **IR LED** | 1 | 850-950nm wavelength (standard remote LED) |
| **IR Receiver Module** | 1 | VS1838B, TSOP1838, or similar |
| **2N2222 NPN Transistor** | 1 | Or 2N222, PN2222 (check datasheet for pinout) |
| **1kΩ Resistor** | 1 | For transistor base |
| **68-100Ω Resistor** | 1 | For IR LED current limiting |
| **Breadboard** | 1 | For prototyping |
| **Jumper Wires** | 20+ | Male-to-male connectors |
| **USB Cable** | 1 | For ESP32 programming (Micro-USB) |
| **5V Power Supply** | 1 | For IR LED (optional, can use ESP32's 5V) |

**Optional but Recommended:**
- Multimeter for testing connections
- ESP32 reset button (or solder-wire) on GPIO0

### Software Requirements

- **Arduino IDE** (or PlatformIO for VS Code)
- **ESP32 Board Support Package**
- **IRremoteESP8266 Library** (v2.8.6+)
- **WiFi & WebServer Libraries** (built-in)

---

## Understanding Gree AC Protocol

Before we build, let's understand what we're working with.

Gree air conditioners use the **YAW1F infrared protocol**. This is a proprietary format that encodes:
- **Power state** (on/off)
- **Temperature** (16-30°C)
- **Mode** (Cool, Heat, Dry, Auto, Fan)
- **Fan speed** (Auto, Low, Medium, High)
- **Swing** (Vertical, Horizontal, Off)
- **Timer** settings
- **Special modes** (Sleep, Turbo, Eco, I Feel, etc.)

The IR receiver captures these signals, while the IR LED transmits them using the `IRremoteESP8266` library. The ESP32 stores the last known state in flash memory, so your settings persist even after a power outage.

---

## Part 1: Building the Circuit

### Step 1: Understand Your Transistor Pinout

**IMPORTANT:** Transistor pinouts vary! Before connecting anything, check your 2N2222 datasheet.

The three legs are:
1. **Base (B)** - Controls the switch
2. **Collector (C)** - Where current flows out (to IR LED)
3. **Emitter (E)** - Where current returns to ground

For TO-92 package (most common):
```
        |
      B | C | E
        | | |
```

### Step 2: Prepare the Breadboard

1. Insert the ESP32 into the breadboard vertically
2. Ensure there's space on both sides for other components
3. Leave room for power buses on the sides

### Step 3: Build the IR Transmitter Circuit

**Goal:** Use the transistor as a low-side switch controlled by GPIO12

**Connections:**

```
ESP32 GPIO12  ──[1kΩ resistor]──┐
                                │
                            Base of 2N2222
                                │
                          [Collector]
                                │
                         IR LED cathode (-)
                                │
5V Rail ──[68-100Ω resistor]──[IR LED anode (+)]
                                
2N2222 Emitter ────────────────[GND]
ESP32 GND ──────────────────────[GND]
```

**Step-by-step:**
1. Connect ESP32 GND to the negative rail of your breadboard
2. Connect a 5V source (can be from ESP32's 5V pin) to the positive rail
3. Connect the 1kΩ resistor from **GPIO12** to the transistor **Base**
4. Connect the transistor **Emitter** to **GND**
5. Connect the transistor **Collector** to the **cathode (short leg)** of the IR LED
6. Connect the **anode (long leg)** of the IR LED to the positive rail through the 68-100Ω resistor
7. Add a **capacitor (optional, 100µF)** across the 5V and GND for noise filtering

### Step 4: Build the IR Receiver Circuit

**Goal:** Capture IR signals on GPIO15

**Connections:**

```
VS1838B/TSOP1838
├─ VCC (pin 1) ───────→ ESP32 3.3V
├─ GND (pin 2) ───────→ ESP32 GND
└─ OUT (pin 3) ───────→ ESP32 GPIO15
```

**Simple and straightforward!** The receiver module handles all demodulation internally.

### Step 5: Add a Reset Button (Optional)

The ESP32 has a built-in BOOT button on GPIO0. This serves as:
- **5-second press** → Enter WiFi provisioning mode
- **10-second press** → Clear WiFi credentials

If your board doesn't have an accessible button, solder a wire to GPIO0 and wire it through a push-button to GND.

### Verification Checklist

Before powering on:
- ✅ All GND connections are common
- ✅ IR LED polarity is correct (anode to +5V, cathode to transistor)
- ✅ GPIO12 connects through 1kΩ to transistor base
- ✅ GPIO15 connects directly to IR receiver OUT
- ✅ 3.3V connects to receiver VCC
- ✅ No short circuits between power rails
- ✅ Transistor Base, Collector, Emitter are in correct positions

---

## Part 2: Software Setup

### Step 1: Install Arduino IDE & ESP32 Support

1. Download **Arduino IDE** from https://www.arduino.cc/en/software
2. Open Arduino IDE
3. Go to **File → Preferences**
4. Add this URL to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
5. Go to **Tools → Board Manager**
6. Search for "ESP32" and install "esp32 by Espressif Systems"

### Step 2: Install Required Libraries

1. Go to **Sketch → Include Library → Manage Libraries**
2. Search for and install:
   - **IRremoteESP8266** (v2.8.6 or higher)
   - This includes all IR protocol support including Gree YAW1F

### Step 3: Configure Your Board

1. Select **Tools → Board → esp32 → ESP32 Dev Module**
2. Select **Tools → Port → COM[X]** (your ESP32's port)
3. Set **Tools → Upload Speed → 921600**

### Step 4: Create the Configuration File

1. Create a new file: `include/wifi_config.h`
2. Add your WiFi credentials:

```cpp
#pragma once

// Your home WiFi SSID and password
constexpr char kWifiSsid[] = "YOUR_WIFI_NETWORK";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

// Optional: Static IP for consistent access
#define WIFI_USE_STATIC_IP 1
#define WIFI_STATIC_IP1 192
#define WIFI_STATIC_IP2 168
#define WIFI_STATIC_IP3 1
#define WIFI_STATIC_IP4 60
#define WIFI_GATEWAY_IP1 192
#define WIFI_GATEWAY_IP2 168
#define WIFI_GATEWAY_IP3 1
#define WIFI_GATEWAY_IP4 1
#define WIFI_SUBNET_MASK1 255
#define WIFI_SUBNET_MASK2 255
#define WIFI_SUBNET_MASK3 255
#define WIFI_SUBNET_MASK4 0

// Optional: Public base URL (for remote access)
#define WIFI_PUBLIC_BASE_URL ""
```

---

## Part 3: Understanding the Code

### The Main Loop

Here's how the firmware works:

```cpp
void setup() {
  Serial.begin(115200);
  
  // Initialize the IR transmitter on GPIO12
  greeAc.begin();
  
  // Initialize the IR receiver on GPIO15
  irrecv.enableIRIn();
  
  // Load saved AC state from flash
  if (!loadSavedGreeState()) {
    resetNativeGreeState();  // Default state if none saved
  }
  
  // Connect to WiFi (or start provisioning mode)
  connectToWifi();
  
  // Setup mDNS for irremote.local
  if (WiFi.status() == WL_CONNECTED) {
    MDNS.begin("irremote");
  }
  
  // Start web server
  startWebServer();
}

void loop() {
  server.handleClient();  // Handle web requests
  handleResetButton();    // Check for provisioning button press
  handleSerial();         // Listen for serial commands
  handleDebugCapture();   // Capture IR signals for debugging
}
```

### Sending an IR Command

When you press a button on the web interface, here's what happens:

```cpp
void sendNativeGreeButtonCode(const String& buttonId) {
  // Example: Power button
  if (buttonId == "power") {
    greeAc.setPower(!greeAc.getPower());  // Toggle power
  }
  
  // Example: Temperature up
  else if (buttonId == "temp_up") {
    uint8_t temp = greeAc.getTemp();
    if (temp < 30) greeAc.setTemp(temp + 1);
    greeAc.setPower(true);  // Turn on if off
  }
  
  // Send the IR command
  greeAc.send();
  
  // Save the new state to flash
  saveCurrentGreeState();
}
```

### WiFi Provisioning with Button Press

The most innovative feature is WiFi provisioning without hardcoding credentials:

```cpp
void handleResetButton() {
  bool pressed = (digitalRead(GPIO0) == LOW);  // Button is active-low
  
  if (pressed && !buttonPressed) {
    buttonPressed = true;
    buttonPressStartTime = millis();
  } 
  else if (!pressed && buttonPressed) {
    buttonPressed = false;
    uint32_t pressDuration = millis() - buttonPressStartTime;
    
    // 5 seconds: Enter provisioning mode
    if (pressDuration >= 5000 && pressDuration < 10000) {
      startProvisioning();
    }
    // 10 seconds: Clear WiFi credentials
    else if (pressDuration >= 10000) {
      clearWifiCredentials();
      ESP.restart();
    }
  }
}
```

### Web API Endpoint Example

The firmware serves a REST API for automation:

```cpp
// Get current AC state as JSON
void handleStatusRequest() {
  String json = "{";
  json += "\"power\":" + String(greeAc.getPower() ? "true" : "false") + ",";
  json += "\"temperature\":" + String(greeAc.getTemp()) + ",";
  json += "\"mode\":\"" + String(getGreeModeCode(greeAc.getMode())) + "\",";
  json += "\"fan\":\"" + String(getGreeFanLabel(greeAc.getFan())) + "\"";
  json += "}";
  
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}
```

---

## Part 4: WiFi Setup (Two Methods)

### Method 1: Traditional Setup (Easy)

If you want to keep WiFi credentials in code:

1. Edit `wifi_config.h` with your SSID and password
2. Flash the firmware
3. Device connects to WiFi on boot
4. Access at `http://irremote.local` or the IP shown in serial monitor

### Method 2: Provisioning Mode (Recommended)

No hardcoded credentials needed!

#### First Time Setup

1. **Flash the firmware** (leave WiFi settings empty in `wifi_config.h`)
2. **Device detects no WiFi** → automatically creates hotspot
3. **Hotspot appears:** SSID `"UNIVERSAL IR"` (password: `-12345678`)
4. **From your phone:**
   - Go to WiFi settings
   - Select **"UNIVERSAL IR"** hotspot
   - Enter password: **`-12345678`**
5. **Open browser** and go to `http://192.168.4.1`
   
   You'll see the **WiFi Setup Interface:**
   - **Available Networks** - List of all WiFi networks your phone can see
   - **Select Network** - Click on your home WiFi SSID
   - **Enter Password** - Type your WiFi password
   - **Connect Button** - Submits and connects device
   
6. **Wait for connection** - Device connects to your WiFi and restarts
7. **Access the remote** at `http://irremote.local` or the IP shown in serial monitor

#### Change WiFi Later

1. **Press reset button for 5 seconds**
   - Device beeps (if you added a buzzer) or prints to serial
   - Returns to provisioning mode
2. **Repeat the provisioning steps above** to connect to a different network

#### Emergency Reset (Clear All Settings)

1. **Press reset button for 10 seconds**
   - Device clears all stored WiFi credentials
   - All AC state is reset to defaults
   - Device restarts into provisioning mode
2. **Restart from "First Time Setup"** above

**Pro Tip:** During provisioning, you can check the serial monitor (115200 baud) to see real-time connection status:
```
Starting WiFi Provisioning Mode...
Connect to SSID: UNIVERSAL IR
Password: -12345678
Then open browser to: http://192.168.4.1
```

---

## Part 5: Using Your New Smart Remote

### Access the Web Interface

Once connected to WiFi, open your browser:

```
http://irremote.local/
```

Or use the IP address shown in the serial monitor:

```
http://192.168.1.60/  (example)
```

### The Remote Interface

You'll see a realistic remote LCD screen with:

**Left Section:**
- Power button (large orange)
- Mode selector (Auto, Cool, Heat, Dry, Fan)
- Temperature controls (+/-)

**Top LCD Display:**
- Current temperature
- Fan speed
- Power state
- Mode label

**Bottom Controls:**
- Swing (vertical and horizontal)
- Timer
- Sleep, Turbo, Eco modes
- WiFi, I Feel, Display, X-Fan toggles

**Debug Section:**
- IR signal capture and analysis
- Protocol information
- Raw hex values (great for reverse-engineering!)

### Serial Monitor Commands

Connect your ESP32 with USB and open the Serial Monitor (115200 baud):

```
help     - Show all commands
status   - Display device status and WiFi info
reset    - Reset AC state to defaults
```

---

## Part 6: Code Structure Overview

### Main Files

**src/main.cpp** (~1700 lines)
```cpp
// WiFi provisioning system
void startProvisioning();
void handleResetButton();

// IR transmission
void sendNativeGreeButtonCode(const String& buttonId);

// Web server
void handleRootRequest();      // Serves the remote UI
void handleStatusRequest();    // JSON API
void handleButtonSendRequest(); // Send IR commands

// Web UI HTML
String getRootPage();          // ~2000 lines of embedded HTML/CSS/JS
String getProvisioningPage();  // WiFi setup interface
```

**src/remote_library.cpp** (~150 lines)
```cpp
// Button metadata
const RemoteButton kGreeAcButtons[] = {
  {"power", "Power", "top", "power", 1},
  {"mode", "Mode", "core", "accent", 1},
  // ... all 20 buttons
};

// JSON serialization helpers
String getLibraryJson();
String escapeJson(const String& input);
```

**include/remote_library.h**
```cpp
// Data structures
struct RemoteButton { /* button properties */ };
struct RemoteModel { /* remote definition */ };

// Query functions
const RemoteModel* findRemoteModel(const String& id);
const RemoteButton* findRemoteButton(const RemoteModel& remote, const String& buttonId);
```

---

## Advanced Features

### 1. IR Signal Capture & Debug

Press any button on your **actual Gree remote** while pointing it at the ESP32:

1. Open `http://irremote.local`
2. Scroll down to "IR Debug Reader"
3. Point your remote at the IR receiver
4. Captured signals appear:
   - Protocol: YAW1F
   - Bits: 48
   - Hex value for further analysis

### 2. API for Automation

Control your AC from other devices:

```bash
# Get current state
curl http://irremote.local/api/status

# Send button command
curl "http://irremote.local/api/button/send?buttonId=power"

# Set temperature
curl "http://irremote.local/api/button/send?buttonId=temp_up&count=5"

# Reset to defaults
curl http://irremote.local/api/remote/clear
```

### 3. State Persistence

Your AC settings are automatically saved to ESP32 flash memory. After a power outage:
- ✅ Remote displays the same mode as before
- ✅ Temperature stays the same
- ✅ All toggles (Sleep, Turbo, etc.) are preserved
- ✅ AC commands will match the display

### 4. mDNS for Stable Access

Instead of remembering an IP address, use:
```
http://irremote.local
```

This works as long as your device is on the same WiFi network, even if the IP changes!

---

## Troubleshooting

### "Remote doesn't connect to WiFi"

**Solution:**
1. Check your SSID and password in `wifi_config.h`
2. Verify ESP32 is close to router
3. Check serial output for error messages
4. Try pressing reset button for 5 seconds to enter provisioning mode

### "IR commands not working"

**Checklist:**
- ✅ Is the IR LED receiving power? (Check with multimeter)
- ✅ Is GPIO12 connected properly?
- ✅ Is the transistor in the correct orientation?
- ✅ Try capturing from another remote first to verify receiver works

### "Can't access irremote.local"

**Try:**
1. Use IP address directly (shown in serial monitor)
2. Check if your WiFi router supports mDNS
3. Restart the ESP32
4. Some networks block mDNS on port 5353

### "ESP32 keeps rebooting"

**Likely causes:**
- Power supply is inadequate (use 5V/2A minimum)
- GPIO conflict (check pin assignments)
- Memory issue (factory reset the board)

### "Provisioning hotspot won't appear"

**Solution:**
1. Check GPIO0 (BOOT button) is connected properly
2. Ensure button press is held for **full 5 seconds** (watch serial monitor)
3. Flash with empty WiFi credentials: `kWifiSsid = ""`
4. Reset button might be already in use - try GPIO4 instead
5. Check `kProvisioningApSsid` constant is correct in code

### "Can't connect to UNIVERSAL IR hotspot"

**Troubleshooting:**
1. Make sure ESP32 is powered on (LED indicator should be lit)
2. Wait 5 seconds after pressing button before searching for hotspot
3. Try **manually entering password:** `-12345678` (with hyphen)
4. Move your phone closer to the ESP32
5. Restart WiFi on your phone
6. Check Serial Monitor to confirm provisioning mode is active

### "Page at 192.168.4.1 won't load"

**Try:**
1. Wait 10 seconds - web server needs time to start
2. Ensure you're connected to **"UNIVERSAL IR"** hotspot (check WiFi settings)
3. Some phones auto-disconnect from provisioning hotspots - manually reconnect
4. Try in incognito/private mode (clears browser cache)
5. Use WiFi IP: Open phone WiFi settings, find UNIVERSAL IR, check the gateway IP shown

### "Settings not saving after provisioning"

**This means device rebooted correctly. Check:**
1. Open Serial Monitor and watch for connection messages
2. After restart, device should show: `Attempting to connect to saved WiFi: YOUR_SSID`
3. Press reset button for 5 seconds to re-enter provisioning if it failed

---

## Customization Ideas

### Add More AC Models

Modify `src/remote_library.cpp` to support other air conditioner protocols:

```cpp
const RemoteButton kDaikinButtons[] = {
  {"power", "Power", "top", "power", 1},
  // ... Daikin-specific buttons
};

const RemoteModel kRemoteLibrary[] = {
  { /* Gree model */ },
  { /* Daikin model */ },
  // ... add more models
};
```

### Create a Mobile App

The REST API is perfect for mobile app development:

```javascript
// JavaScript/React example
fetch('http://irremote.local/api/status')
  .then(r => r.json())
  .then(status => {
    console.log('AC Power:', status.gree_state.power);
    console.log('Temperature:', status.gree_state.temperature);
  });
```

### Add Temperature Sensor

Wire a DHT22 or similar to monitor room temperature:

```cpp
#include "DHT.h"
DHT dht(GPIO_PIN, DHT22);

void loop() {
  float temp = dht.readTemperature();
  // Send to web UI or API
}
```

### Cloud Integration

For remote access, set up a secure tunnel:

```cpp
// Example with ngrok or cloudflare tunnel
#define WIFI_PUBLIC_BASE_URL "https://myremote.tunnels.me"
```

---

## Final Thoughts

You've just built a **fully functional WiFi-enabled AC remote** using just $15 worth of components and open-source software. No cloud, no subscriptions, no privacy concerns—just you and your AC on your own local network.

### What You've Learned:

- ✅ IR protocol basics and transistor switching
- ✅ ESP32 WiFi and web server setup
- ✅ State persistence with NVS flash storage
- ✅ Building a responsive web UI
- ✅ REST API design
- ✅ IoT device provisioning

### Next Steps:

1. **Expand to multiple ACs** - Wire multiple IR LEDs to different GPIO pins
2. **Add voice control** - Integrate with Home Assistant or Alexa
3. **Create a mobile app** - Use React Native or Flutter
4. **Monitor energy usage** - Log AC usage patterns
5. **Implement scheduling** - Turn AC on at specific times

### Share Your Build!

Have you completed this project? Share your experience in the comments:
- What worked well?
- What challenges did you face?
- Any cool customizations?

---

## Resources & References

- **IRremoteESP8266 Library:** https://github.com/crankyoldgit/IRremoteESP8266
- **ESP32 Documentation:** https://docs.espressif.com/projects/esp-idf/
- **Gree AC Protocol:** Analysis in [WIFI_PROVISIONING_IMPLEMENTATION.md]
- **Arduino IDE:** https://www.arduino.cc/en/software
- **VS1838B Datasheet:** Standard IR demodulator module

---

## Questions?

If you have questions or run into issues:
1. Check the serial monitor output first
2. Review the troubleshooting section above
3. Check GPIO pin assignments in code
4. Verify circuit connections with multimeter

---

**Happy building! 🎉**

*Last Updated: April 2026*

