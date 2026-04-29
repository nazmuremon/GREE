# WiFi Provisioning Implementation Guide

## Technical Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 Firmware                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────┐      ┌──────────────────────┐    │
│  │   WiFi Module        │      │   NVS Storage        │    │
│  ├──────────────────────┤      ├──────────────────────┤    │
│  │ - STA Mode           │◄────►│ - SSID               │    │
│  │ - AP Mode            │      │ - Password           │    │
│  │ - Provisioning       │      │ - AC State           │    │
│  └──────────────────────┘      └──────────────────────┘    │
│           ▲                              ▲                   │
│           │                              │                   │
│  ┌────────┴──────────────────────────────┴─────────┐        │
│  │         Button Handler & State Machine           │        │
│  ├────────────────────────────────────────────────┤        │
│  │ - GPIO0 Input Monitoring                       │        │
│  │ - Press Duration Measurement                   │        │
│  │ - Mode Transitions (5s, 10s)                   │        │
│  └────────────────────────────────────────────────┘        │
│           ▲                              ▲                   │
│           │                              │                   │
│  ┌────────┴──────────────────────────────┴─────────┐        │
│  │      Web Server & mDNS                           │        │
│  ├────────────────────────────────────────────────┤        │
│  │ - Main Dashboard (irremote.local)              │        │
│  │ - Provisioning Portal (192.168.4.1)            │        │
│  │ - DNS Server (Captive Portal)                  │        │
│  │ - REST API Endpoints                           │        │
│  └────────────────────────────────────────────────┘        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Code Structure

### New Global Variables

```cpp
// WiFi provisioning state
bool isProvisioningMode = false;
uint32_t buttonPressStartTime = 0;
bool buttonPressed = false;
String savedWifiSsid = "";
String savedWifiPassword = "";

// DNS server for captive portal
DNSServer dnsServer;
```

### New Functions Added

#### WiFi Credential Management
```cpp
void saveWifiCredentials(const String& ssid, const String& password)
void loadWifiCredentials()
void clearWifiCredentials()
```

**Purpose**: Handle persistent storage of WiFi credentials in NVS

**NVS Keys**:
- `"wifi_ssid"` - Network name
- `"wifi_pass"` - Network password

#### WiFi Provisioning
```cpp
void startProvisioning()
```

**Purpose**: Initialize provisioning mode
- Switch to AP mode
- Create hotspot with credentials
- Start DNS server
- Reconfigure web server for provisioning UI

**Flow**:
```
startProvisioning()
├─ WiFi.mode(WIFI_AP)
├─ WiFi.softAP(kProvisioningApSsid, kProvisioningApPassword)
├─ dnsServer.start(53, "*", WiFi.softAPIP())
├─ server.reset()
├─ server.on("/", handleProvisioningRoot)
├─ server.on("/api/scan-networks", handleScanNetworks)
└─ server.on("/api/connect-wifi", handleConnectWifi)
```

#### Button Handling
```cpp
void handleResetButton()
```

**Purpose**: Debounce and measure button press duration

**Logic**:
- Monitor GPIO0 for LOW signal
- Measure press duration
- Trigger action based on duration:
  - `5000-9999ms`: Start provisioning
  - `≥10000ms`: Clear credentials & restart

#### Provisioning API Handlers
```cpp
void handleScanNetworks()
void handleConnectWifi()
void handleProvisioningRoot()
```

**Endpoints**:
- `GET /api/scan-networks` - Return list of available networks
- `POST /api/connect-wifi` - Connect to selected network and save credentials

#### Provisioning UI
```cpp
String getProvisioningPage()
```

**Features**:
- Mobile-responsive design
- Real-time network scanning
- Visual feedback during connection
- Error handling and retry
- CSS animations and modern styling

### Modified Functions

#### `connectToWifi()`
**Changes**:
1. Added credential loading from NVS
2. Attempts saved credentials first
3. Falls back to hardcoded credentials
4. Enters provisioning mode if connection fails

**New Flow**:
```
connectToWifi()
├─ Load saved credentials from NVS
├─ If saved credentials exist
│  └─ Attempt connection to saved WiFi
├─ Else if hardcoded credentials exist
│  └─ Attempt connection to hardcoded WiFi
└─ Else
   └─ Start provisioning mode
```

#### `setup()`
**Changes**:
1. Initialize GPIO0 as INPUT_PULLUP
2. Skip mDNS initialization during provisioning
3. Add provisioning status check

#### `loop()`
**Changes**:
1. Add DNS server handling during provisioning
2. Add button press monitoring
3. Maintain existing IR capture and serial handling

#### `printHelp()`
**Changes**:
1. Added WiFi provisioning button timing documentation

## Configuration Constants

### New Constants Added

```cpp
// Hardware
constexpr uint16_t kResetButtonPin = 0;

// Provisioning
constexpr char kProvisioningApSsid[] = "UNIVERSAL IR";
constexpr char kProvisioningApPassword[] = "-12345678";

// Hostname (changed from "gree" to "irremote")
constexpr char kDeviceHostname[] = "irremote";

// NVS Keys
constexpr char kPreferencesWifiSsidKey[] = "wifi_ssid";
constexpr char kPreferencesWifiPasswordKey[] = "wifi_pass";

// Timing (milliseconds)
constexpr uint32_t kResetButtonProvisioningTime = 5000;
constexpr uint32_t kResetButtonWifiResetTime = 10000;
constexpr uint32_t kButtonDebounceMs = 50;
```

### Modifiable Configuration

#### Button Pin
```cpp
// Change to different GPIO if using different pin
constexpr uint16_t kResetButtonPin = 0;  // BOOT button on most ESP32 dev boards
```

#### Provisioning Hotspot Name
```cpp
// Customize provisioning AP credentials
constexpr char kProvisioningApSsid[] = "UNIVERSAL IR";
constexpr char kProvisioningApPassword[] = "-12345678";
```

#### mDNS Hostname
```cpp
// Change to different hostname (e.g., "gree", "ac", "remote")
constexpr char kDeviceHostname[] = "irremote";
// Access at: http://irremote.local
```

#### Button Timing
```cpp
// Adjust button press duration thresholds (in milliseconds)
constexpr uint32_t kResetButtonProvisioningTime = 5000;   // 5 seconds
constexpr uint32_t kResetButtonWifiResetTime = 10000;     // 10 seconds
```

## WiFi State Machine

```
┌─────────────────────────────────────┐
│   Power On / Cold Start              │
└──────────────┬──────────────────────┘
               │
               ▼
        ┌──────────────┐
        │ Load NVS     │
        │ Credentials  │
        └──────┬───────┘
               │
         ┌─────┴──────────────────────┐
         │                            │
    Credentials?                  Credentials?
    │                             │
    No                            Yes
    │                             │
    ▼                             ▼
    ┌──────────────┐        ┌──────────────┐
    │ Hardcoded    │        │ Use Saved    │
    │ Credentials? │        │ Credentials  │
    └──────┬───────┘        └──────┬───────┘
           │                       │
      Yes  │  No            ┌──────┴────────────┐
      │    │                │                   │
      ▼    ▼                ▼                   ▼
   ┌──────────────┐     ┌─────────────┐  ┌──────────────┐
   │ Try Connect  │     │ Try Connect │  │ Connection OK│
   │ Hardcoded    │     │ Saved WiFi  │  │ STA Mode     │
   └──────┬───────┘     └──────┬──────┘  │ mDNS Start   │
          │                    │         │ Dashboard    │
          ▼                    ▼         └──────────────┘
      ┌─────────────────────────────┐
      │   Connection Failed         │
      │ OR Timeout After 15s        │
      └──────┬──────────────────────┘
             │
             ▼
      ┌──────────────────────────┐
      │ Start Provisioning Mode  │
      │ AP: UNIVERSAL IR         │
      │ Password: -12345678      │
      │ URL: 192.168.4.1         │
      └──────────────────────────┘
             │
             ├─────────────────┐
             │                 │
        User Input         Wait for
        (WiFi Creds)       10s Button
             │               │
             ▼               ▼
          Connect         Clear Creds
          & Save          & Restart
             │               │
             └────────┬──────┘
                      │
                      ▼
              Restart & Return to
              STA Connection Attempt
```

## Button Press State Machine

```
┌────────────────────────┐
│   GPIO0 Monitoring     │
│   (INPUT_PULLUP)       │
└────────────────┬───────┘
                 │
        ┌────────┴──────────┐
        │                   │
      HIGH              LOW (Button Pressed)
        │                   │
        ▼                   ▼
   ┌─────────┐         ┌──────────────────┐
   │ Idle    │         │ buttonPressed=1  │
   └─────────┘         │ recordTime=now() │
                       └────────┬─────────┘
                                │
                    ┌───────────┴──────────────┐
                    │ Wait for Release        │
                    │ Monitor Duration        │
                    └───────────┬──────────────┘
                                │
                               ▼
                    ┌─────────────────────┐
                    │ Release Detected    │
                    │ (GPIO0 = HIGH)      │
                    └─────────┬───────────┘
                              │
                   ┌──────────┴──────────┐
                   │                    │
              Duration?            Duration?
              │                    │
         < 5000ms            >= 5000ms
              │                    │
              ▼                    ▼
         ┌─────────┐         ┌──────────┐
         │ Ignore  │         │ Check    │
         │ (Noise) │         │ Again    │
         └─────────┘         └────┬─────┘
                                  │
                        ┌─────────┴──────────┐
                        │                   │
                  5000-9999ms         >= 10000ms
                        │                   │
                        ▼                   ▼
                  ┌──────────────┐  ┌──────────────┐
                  │ Provisioning │  │ Reset WiFi   │
                  │ Mode         │  │ & Restart    │
                  └──────────────┘  └──────────────┘
```

## NVS (Non-Volatile Storage) Layout

```
Namespace: "gree-remote"
├─ state (bytes, 14)
│  └─ Gree AC remote state (existing)
├─ wifi_ssid (string)
│  └─ Saved WiFi network name
└─ wifi_pass (string)
   └─ Saved WiFi password
```

**Size Considerations**:
- SSID: max 32 bytes
- Password: max 64 bytes
- State: 14 bytes
- **Total**: ~112 bytes (well within available NVS)

**Wear Leveling**: NVS automatically handles wear leveling, making frequent writes safe

## Provisioning Portal Flow

```
User Phone                    ESP32 Device
    │                              │
    ├─ Connect to UNIVERSAL IR ──►│ WiFi.softAP()
    │                              │
    ├─ Open http://192.168.4.1 ──►│ handleProvisioningRoot()
    │                              │
    │◄─── Provisioning Page ───────┤ getProvisioningPage()
    │                              │
    ├─ GET /api/scan-networks ──►│ WiFi.scanNetworks()
    │                              │
    │◄─── Network List (JSON) ─────┤ handleScanNetworks()
    │                              │
    ├─ POST /api/connect-wifi ──►│ handleConnectWifi()
    │ (ssid, password)             │
    │                              │ saveWifiCredentials()
    │                              │ WiFi.begin(ssid, pass)
    │                              │ Wait for connection...
    │◄─── Connection Status ───────┤
    │                              │ saveWifiCredentials()
    │                              │ ESP.restart()
    │                              │
    │                        (Device Restarts)
    │                              │
    │                              ├─ Load saved credentials
    │                              ├─ Connect to home WiFi
    │                              ├─ Start mDNS
    │                              │
    ├─ Connect to home WiFi ──────┤ (same network now)
    │                              │
    ├─ Open http://irremote.local►│ handleRootRequest()
    │                              │
    │◄─── Remote Dashboard ────────┤ getRootPage()
    │                              │
```

## API Response Examples

### GET /api/scan-networks

**Response**:
```json
{
  "networks": [
    {"ssid": "HomeWiFi", "rssi": -45},
    {"ssid": "NeighborWiFi", "rssi": -72},
    {"ssid": "GuestNetwork", "rssi": -58}
  ]
}
```

**RSSI**: Signal strength in dBm (Received Signal Strength Indicator)
- -30 to -50: Excellent
- -50 to -60: Good
- -60 to -70: Fair
- Below -70: Weak

### POST /api/connect-wifi

**Request**:
```json
{
  "ssid": "MyHomeWiFi",
  "password": "securepassword123"
}
```

**Success Response**:
```json
{
  "success": true,
  "message": "Connected to WiFi"
}
```

**Failure Response**:
```json
{
  "success": false,
  "error": "Failed to connect to WiFi"
}
```

## Error Handling

### Button Press Errors

| Error | Cause | Solution |
|-------|-------|----------|
| No button press detected | GPIO0 not connected | Check wiring |
| Incorrect timing | GPIO voltage floating | Add pull-up resistor |
| Multiple triggers | Contact bounce | Add debouncing |

### WiFi Connection Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "SSID required" | Empty SSID field | Enter WiFi name |
| "Failed to connect" | Wrong password | Verify WiFi password |
| Connection timeout | Out of range | Move closer to router |
| DNS failure | Network issues | Check WiFi network |

### NVS Errors

| Error | Cause | Solution |
|-------|-------|----------|
| Failed to save | NVS full | Clear preferences |
| Failed to load | Corruption | Clear and restart |
| Empty string | Not found | Re-enter credentials |

## Performance Metrics

| Operation | Time |
|-----------|------|
| NVS write | ~50-100ms |
| NVS read | ~20-50ms |
| WiFi scan | 1-3 seconds |
| WiFi connect | 5-15 seconds |
| mDNS resolution | <200ms |
| Device restart | ~2 seconds |
| Provisioning page load | <1 second |

## Memory Usage

```
Code:
- Main firmware: ~500KB
- Provisioning UI: ~8KB
- Web server handlers: ~20KB

Data (RAM):
- Strings (SSID/password): ~100 bytes
- Buffers: ~4KB
- State variables: ~50 bytes
- DNSServer: ~1KB
- WebServer: ~5KB

Storage:
- NVS namespace: ~200 bytes
- Firmware partition: ~512KB
```

## Security Considerations

### Credential Storage

- Credentials stored in **plain text** in NVS
- No encryption applied by default
- Consider implementing secure storage for production:
  ```cpp
  // Example: Simple XOR obfuscation
  String encrypted = xorEncrypt(password, secretKey);
  preferences.putString("wifi_pass", encrypted);
  ```

### Hotspot Security

- Uses **WPA2-PSK** encryption
- Password: `-12345678` (hardcoded, not user-configurable)
- Hotspot only active during provisioning

### Network Security

- WiFi password transmitted over **encrypted** connection (WPA2)
- mDNS is **local network only**
- No internet exposure by default
- Recommend HTTPS for production dashboards

### Best Practices

1. **Change provisioning password** in production:
   ```cpp
   constexpr char kProvisioningApPassword[] = "your-custom-password";
   ```

2. **Add authentication** to API endpoints:
   ```cpp
   if (!isAuthenticated()) {
     server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
     return;
   }
   ```

3. **Validate user input** before processing:
   ```cpp
   if (ssid.length() > 32 || password.length() > 64) {
     return; // Reject invalid input
   }
   ```

## Troubleshooting Development

### Serial Output for Debugging

Monitor serial for provisioning events:

```
Starting WiFi Provisioning Mode...
Connect to SSID: UNIVERSAL IR
Password: -12345678
Then open browser to: http://192.168.4.1

Reset button press duration: 5234ms
Medium press detected - starting WiFi provisioning
```

### Enable Debug Logging

Add serial output to provisioning functions:

```cpp
void handleConnectWifi() {
  Serial.println("handleConnectWifi() called");
  Serial.print("Request body: ");
  Serial.println(server.arg("plain"));
  // ... rest of code ...
}
```

### Test Provisioning Without Button

Simulate button press in code:

```cpp
// In loop(), temporarily add:
if (/* some condition */) {
  startProvisioning();  // Force provisioning mode
}
```

## Future Enhancements

### Potential Improvements

1. **WebSocket for Real-time Updates**
   ```cpp
   // Replace polling with WebSocket
   WebSocketsServer webSocket = WebSocketsServer(81);
   webSocket.begin();
   ```

2. **Encrypted Credentials**
   ```cpp
   // Use mbedTLS for encryption
   #include <mbedtls/aes.h>
   // ... encryption code ...
   ```

3. **WiFi Signal Strength Visualization**
   ```cpp
   // Return RSSI in provisioning page
   // Render signal bars in UI
   ```

4. **QR Code Provisioning**
   ```cpp
   // Generate QR code with WiFi credentials
   // Scan with phone to auto-connect
   ```

5. **OTA Firmware Updates**
   ```cpp
   // Implement within provisioning page
   server.on("/update", handleFirmwareUpdate);
   ```

6. **Multi-language Support**
   ```cpp
   // Localize provisioning UI
   String getProvisioningPage(const String& lang);
   ```

---

**Document Version**: 1.0  
**Last Updated**: April 2026  
**Firmware**: ESP32 Universal Gree Remote v2.0
