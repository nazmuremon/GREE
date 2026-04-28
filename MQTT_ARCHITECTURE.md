# ESP32 Universal Gree Remote - MQTT Architecture

A DIY WiFi-enabled remote for Gree air conditioners using MQTT for universal cloud access.

## Architecture

```
                          WiFi Network
                              |
    ┌─────────────────────────┼─────────────────────────┐
    |                         |                         |
ESP32 Device          WiFi Provisioning AP      MQTT Broker
(IR Control)          (Setup Interface)       (broker.hivemq.com)
    |                         |                    |
    |                   Press 5s button ◄──────────┤
    |                                              |
    └──────── Connects to saved WiFi ──────────────┘
    |                                              |
    └──── Publishes IR State ──┐                  |
         Subscribes to Commands┘     Web UI ◄─────┘
         via MQTT                 (GitHub Pages)
                                   Mobile/Desktop
```

## Features

- 🌐 **Web-based Control**: Access remote from any device (phone, tablet, desktop)
- 📱 **No Installation**: Works in any modern web browser
- 🔗 **MQTT Powered**: True cloud connectivity
- 🎨 **Realistic UI**: Replica of physical Gree remote
- 🔐 **Secure WiFi Setup**: Provisioning hotspot for easy setup
- ⚡ **Lightweight ESP32**: Only WiFi provisioning + MQTT on device
- 💾 **Persistent Config**: WiFi credentials saved in NVS
- 📊 **Real-time Status**: AC state synced via MQTT

## Quick Start

### 1. Flash ESP32 Firmware

```bash
platformio run --target upload --upload-port COM7
# (replace COM7 with your port)
```

### 2. WiFi Provisioning (5-second button press)

When powered on:
- **No WiFi saved**: Automatically enters provisioning mode
- **5-second button press**: Manual provisioning
- **10-second button press**: Clear WiFi and restart

Provisioning hotspot:
- **SSID**: `UNIVERSAL IR`
- **Password**: `12345678`
- **URL**: `http://192.168.4.1`

### 3. Control via Web UI

Visit: **[Universal Gree Remote Web Control](https://YourUsername.github.io/esp32-gree-remote-web)**

(See [Web UI Repository](github-pages-site/README.md))

## MQTT Configuration

| Setting | Value |
|---------|-------|
| Broker | broker.hivemq.com |
| Port | 1883 (ESP32) / 8001 (Web UI WebSocket) |
| Username | (not required - public broker) |
| Password | (not required - public broker) |

### Topics

#### `remote/command` (Subscribe - ESP32 receives)
JSON format:
```json
{"button":"power"}
```

Supported buttons:
```
power, mode, fan, temp_up, temp_down,
cool, heat, dry, auto, fan_only,
swing, swing_h, timer, sleep, turbo,
xfan, light, wifi, ifeel, display,
energy
```

#### `remote/status` (Publish - ESP32 sends)
JSON format:
```json
{
  "gree_state": {
    "power": 1,
    "mode_label": "COOL",
    "mode": "cool",
    "fan_label": "AUTO",
    "display_value": 24,
    "timer_label": "OFF",
    "display_source_label": "SET",
    "display_source": "on",
    "display_note": "Operating",
    "sleep": 0,
    "turbo": 0,
    "eco": 0,
    "ifeel": 0,
    "xfan": 0,
    "light": 1,
    "wifi": 1,
    "swing_vertical_label": "AUTO",
    "swing_horizontal_label": "AUTO"
  },
  "last_event": "Power toggled"
}
```

## Hardware Setup

| Component | Pin |
|-----------|-----|
| IR Receiver | GPIO 15 |
| IR Transmitter | GPIO 12 |
| Reset Button | GPIO 0 (BOOT) |
| GND | GND |
| 5V | 5V (via USB) |

## Serial Monitor

```
Monitor at 115200 baud for debug output:
```

Example output:
```
WiFi credentials saved: MyNetwork
Attempting to connect to saved WiFi: MyNetwork
Wi-Fi connected: MyNetwork
Connecting to MQTT broker...✓ Connected!
MQTT Command received: {"button":"power"}
```

## Troubleshooting

### ESP32 won't connect to WiFi
1. Ensure button press duration is correct (5 seconds for provisioning)
2. Check SSID spelling (case-sensitive)
3. Try 10-second press to clear credentials, then reprogram

### MQTT not connecting
1. Verify WiFi is connected (check serial monitor)
2. Ping broker: `ping broker.hivemq.com`
3. No credentials needed (public broker)
4. Try manual MQTT client:
   ```bash
   mosquitto_pub -h broker.hivemq.com \
     -t remote/command -m '{"button":"power"}'
   ```

### IR commands not working
1. Ensure IR LED is pointed at AC unit
2. Verify LED on GPIO 12 lights up when sending
3. Use serial monitor to confirm MQTT messages received
4. Test with physical remote to verify AC unit responds

### Web UI says "Disconnected"
1. Open browser console (F12) to see WebSocket errors
2. Verify MQTT broker is reachable
3. Check firewall rules on MQTT broker
4. Broker may need WebSocket support on port 8883 or 9001

## Project Structure

```
esp32-gree-remote/
├── platformio.ini              # Build configuration with MQTT library
├── src/
│   ├── main.cpp               # Firmware (WiFi provisioning + MQTT + IR control)
│   └── remote_library.cpp     # AC state management
├── include/
│   ├── remote_library.h
│   ├── wifi_config.example.h  # WiFi configuration template
│   └── wifi_config.h          # Auto-generated from provisioning
├── lib/
├── test/
└── github-pages-site/         # Web UI (separate repo)
    ├── index.html            # Remote interface
    ├── README.md             # Web UI documentation
    └── .github/workflows/pages.yml # GitHub Pages deployment
```

## Dependencies

**ESP32 Firmware:**
- IRremoteESP8266 (v2.8.6+): IR protocol library
- PubSubClient (v2.8.0+): MQTT client

**Web UI:**
- Paho MQTT JS: Browser MQTT client
- HTML5/CSS3/JavaScript

## Building from Source

### Firmware
```bash
# Install PlatformIO
pip install platformio

# Build and upload
platformio run --target upload --upload-port COM7
```

### Web UI
```bash
# Install Node (optional, for local testing)
npm install -g http-server

# Serve locally
cd github-pages-site
http-server

# Deploy to GitHub Pages (automatic on commit)
git push origin main
```

## Commands via Serial

```
status   - Show current state
help     - Display available commands  
reset    - Reset AC state to defaults
```

## Configuration

### WiFi (Static IP)
Edit `include/wifi_config.h`:
```cpp
#define WIFI_USE_STATIC_IP 1
#define WIFI_STATIC_IP1 192
#define WIFI_STATIC_IP2 168
#define WIFI_STATIC_IP3 1
#define WIFI_STATIC_IP4 60
```

### Debug IR Reader
Enable IR capture logging in `src/main.cpp`:
```cpp
constexpr bool kEnableDebugIrReader = true;  // Show captured IR codes
```

## Power Usage

- **At Rest**: ~100mA (WiFi + IR ready)
- **Sending Command**: ~500mA peak
- **Provisioning Mode**: ~200mA

Recommended: 2A USB power supply

## License

MIT License - See LICENSE file

## Contributing

Pull requests welcome! Please test on physical hardware.

## Author

Created for universal AC control via MQTT cloud connectivity.

---

**Next Steps:**
- See [Web UI Setup](github-pages-site/README.md) to deploy the remote control interface
- Deploy to GitHub Pages for global access
