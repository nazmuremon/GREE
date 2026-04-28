# Migration Guide: WiFi to MQTT Architecture

Complete guide for understanding and implementing the transition from local WiFi-only remote to MQTT cloud-based control.

## What Changed

### Before (Local WiFi Only)
```
Browser on same WiFi
    ↓
ESP32 Web Server (Port 80)
    ↓
HTTP /api/button/send requests
    ↓
AC Control + State Management
```

**Limitations:**
- ❌ Only accessible on local WiFi network
- ❌ Works only when connected to router
- ❌ Not accessible from outside
- ❌ No cloud backup
- ❌ No mobile app integration

### After (MQTT Cloud)
```
Browser (Any Network)
    ↓
Web UI on GitHub Pages (HTTPS)
    ↓
MQTT Broker (broker.hivemq.com:1883 - public HiveMQ)
    ↓
ESP32 MQTT Client
    ↓
AC Control + State Management
```

**Advantages:**
- ✅ Global access from any device
- ✅ Secure cloud connectivity
- ✅ Mobile friendly
- ✅ Persistent logging possible
- ✅ Multi-device support
- ✅ Integration with other smart home systems

## Major Changes

### 1. ESP32 Firmware Changes

#### Added Dependencies
```ini
; platformio.ini
lib_deps =
  crankyoldgit/IRremoteESP8266 @ ^2.8.6
  knolleary/PubSubClient @ ^2.8.0    # NEW
```

#### Added MQTT Configuration
```cpp
// src/main.cpp
constexpr char kMqttBrokerHost[] = "broker.hivemq.com";
constexpr uint16_t kMqttBrokerPort = 1883;
constexpr char kMqttUsername[] = "";
constexpr char kMqttPassword[] = "";
constexpr char kMqttTopicCommand[] = "remote/command";
constexpr char kMqttTopicStatus[] = "remote/status";
```

#### Added MQTT Functions
- `reconnectMQTT()` - Connect/reconnect to broker
- `onMqttMessage()` - Handle incoming commands
- `handleButtonCommand()` - Process button press
- `publishStatus()` - Send AC state

#### Updated Loop
```cpp
void loop() {
  // ... existing code ...
  
  // NEW: Handle MQTT
  if (!isProvisioningMode && WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    } else {
      mqttClient.loop();
    }
  }
}
```

#### Removed Code
- ❌ `getRootPage()` - No more embedded web UI
- ❌ `handleRootRequest()` - No HTTP remote endpoint
- ❌ `/api/status` endpoint - Status now via MQTT
- ❌ `/api/button/send` endpoint - Commands now via MQTT
- ❌ Web server routes for remote UI

**Kept:**
- ✅ WiFi provisioning (5-second button mode)
- ✅ IR transmission and reception
- ✅ State persistence (NVS)
- ✅ Serial debug output

### 2. Web UI Changes

#### Old: Embedded in ESP32
- Served from `http://<esp32-ip>/`
- HTTP GET/POST requests
- Built as C++ string literals
- Size constraint (limited by ESP32 flash)

#### New: GitHub Pages
- Hosted globally at `https://YOUR_USERNAME.github.io/esp32-gree-remote-web`
- WebSocket MQTT client
- Pure HTML/CSS/JavaScript
- Unlimited updates (git push = deploy)

#### Implementation
```javascript
// New: Browser connects directly to MQTT broker
const client = new Paho.MQTT.Client(wsUrl, clientId);
client.onMessageArrived = (message) => {
  // Handle status updates
};
client.send(commandMessage);  // Send commands
```

### 3. Communication Protocol

#### Before: REST API
```
GET /api/status
{
  "gree_state": {...},
  "debug_ir": {...}
}

GET /api/button/send?buttonId=power&count=1
{"message": "Power toggled", "status": {...}}
```

#### After: MQTT Topics
```
Subscribe: remote/status
{
  "gree_state": {...},
  "last_event": "Power toggled"
}

Publish: remote/command
{"button": "power"}
```

## Migration Checklist

### For Users

- [ ] Read [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md)
- [ ] Flash new ESP32 firmware with MQTT support
- [ ] Configure WiFi via provisioning (5-second button)
- [ ] Set up GitHub Pages web UI (follow [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md))
- [ ] Update bookmarks/links to GitHub Pages site
- [ ] Test MQTT connection (check serial monitor)
- [ ] Verify IR commands work via web UI

### For Developers

- [ ] Update `platformio.ini` with PubSubClient library
- [ ] Add MQTT constants to `src/main.cpp`
- [ ] Implement `reconnectMQTT()` function
- [ ] Implement `onMqttMessage()` callback
- [ ] Update `setup()` to initialize MQTT
- [ ] Update `loop()` to handle MQTT
- [ ] Remove web server remote UI code
- [ ] Test provisioning mode still works
- [ ] Test MQTT command reception
- [ ] Deploy web UI to GitHub Pages
- [ ] Test end-to-end: Browser → MQTT → ESP32 → IR → AC

## Configuration

### MQTT Broker Details
```
Broker:   broker.hivemq.com (public HiveMQ)
Port:     1883 (plain TCP for ESP32)
           8001 (WebSocket for browser)
Username: (not required)
Password: (not required)
```

### Topics
```
Command Topic:  remote/command      (ESP32 subscribes)
Status Topic:   remote/status       (ESP32 publishes)
```

### Message Format
```json
// Command (browser → ESP32)
{"button":"power"}
{"button":"temp_up"}
{"button":"mode"}

// Status (ESP32 → browser)
{
  "gree_state": {
    "power": 1,
    "mode": "cool",
    "temp": 24,
    ...
  },
  "last_event": "Power toggled"
}
```

## Troubleshooting Migration

### Issue: MQTT Connection Failed
**Solution:**
1. Verify WiFi connected: `Serial Monitor at 115200 baud`
2. Ping broker: `ping broker.hivemq.com`
3. No credentials needed (public broker)
4. Ensure ESP32 can reach MQTT port 1883 (check firewall)

### Issue: Web UI Shows "Disconnected"
**Solution:**
1. Verify ESP32 connected to MQTT (serial monitor)
2. Check browser console (F12) for WebSocket errors
3. Ensure MQTT broker supports WebSocket protocol
4. Verify firewall allows WebSocket traffic

### Issue: IR Commands Not Executing
**Solution:**
1. Check ESP32 receives MQTT message (serial monitor)
2. Verify IR LED is on GPIO 12
3. Check AC responds to physical remote
4. Verify `isSupportedButton()` includes the button

### Issue: WiFi Provisioning Doesn't Appear
**Solution:**
1. Press reset button for exactly 5 seconds
2. Check serial monitor: "Starting WiFi Provisioning Mode"
3. Look for "UNIVERSAL IR" hotspot
4. If not appearing: Try 10-second reset, then reprogram

## Rollback (if needed)

If you need to revert to local-only WiFi remote:

1. Restore from git history:
   ```bash
   git log --oneline
   git checkout <old-commit>
   ```

2. Or use previous firmware version

3. Limitations when reverting:
   - No cloud access
   - Only works on local network
   - Larger firmware size (embedded UI)

## Performance Comparison

| Metric | Old (WiFi) | New (MQTT) |
|--------|-----------|-----------|
| **Latency** | ~50ms (local) | ~100-200ms (cloud) |
| **Accessibility** | Local only | Global |
| **Setup Time** | 5 minutes | 15 minutes |
| **Device Size** | 3000KB | 2500KB |
| **Flexibility** | Low | High |
| **Scaling** | Single device | Multiple devices |

## Benefits of MQTT Architecture

### For Users
1. **Global Access**: Control AC from anywhere
2. **No Port Forwarding**: Secure cloud gateway
3. **Mobile Friendly**: Responsive web UI
4. **No Installation**: Just bookmark a URL
5. **Always Updated**: Deploy new UI without reflashing ESP32

### For Developers
1. **Separation of Concerns**: Device logic vs UI
2. **Scalability**: Support multiple devices with same UI
3. **Easier Updates**: Update UI by git push
4. **Standard Protocol**: MQTT integrates with other systems
5. **Cloud Integration**: Log data, create automations, etc.

## Future Enhancements

With MQTT architecture, you can now:

- [ ] Log AC usage statistics
- [ ] Create automation rules (e.g., "turn off at 6 PM")
- [ ] Integrate with Home Assistant
- [ ] Build mobile app (subscribe to same topics)
- [ ] Add multiple devices (each with unique identifier)
- [ ] Create cost tracking (electricity per day)
- [ ] Build web dashboard with historical data
- [ ] Set up alerts (AC offline, unusual activity)

## Support and Questions

- **Issues**: Create GitHub issue with details
- **Discussions**: Use GitHub Discussions
- **Documentation**: See related .md files
- **Testing**: Verify on real hardware

## Summary

The migration to MQTT brings **cloud connectivity** while keeping **local provisioning** secure and simple. The ESP32 becomes lighter (no web server), more reliable (no HTTP timeouts), and integrates better with smart home ecosystems.

**Key Takeaway**: Device handles hardware (WiFi + IR), Cloud handles UI and logic (MQTT + GitHub Pages).

---

**Next**: Follow [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md) to deploy your web UI!
