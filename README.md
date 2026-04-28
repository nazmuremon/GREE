# ESP32 Universal Gree Remote - MQTT Edition

Universal WiFi-based Gree air conditioner remote using MQTT for cloud connectivity. Control your AC from any device globally via a web browser.

**🌐 Live Demo**: [Universal Gree Remote Web Control](https://YOUR_USERNAME.github.io/esp32-gree-remote-web)
*(Replace `YOUR_USERNAME` with your GitHub username after setup)*

## Architecture

- **ESP32 Device**: WiFi provisioning + IR control + MQTT client
- **MQTT Broker**: Cloud message hub (128.199.20.163:1883)
- **Web UI**: Hosted on GitHub Pages (no installation needed)
- **Communication**: MQTT messages for universal access

## What This Project Does

- 🌍 **Universal Control**: Access your AC from anywhere globally
- 📱 **No App**: Works in any web browser
- 🔐 **Secure Setup**: WiFi provisioning hotspot
- 💾 **Persistent Config**: Saves WiFi credentials in NVS
- 🔗 **Cloud Connected**: Real-time MQTT communication
- 🎨 **Realistic UI**: Remote replica interface
- ⚡ **Lightweight Device**: WiFi + MQTT only (no web server)

## Quick Start

### 1. Flash ESP32

```bash
cd esp32-gree-remote
platformio run --target upload --upload-port COM7
```

### 2. WiFi Provisioning (5-second button press)

- Device creates **UNIVERSAL IR** hotspot (password: `12345678`)
- Open browser to `http://192.168.4.1`
- Select your WiFi and enter password
- Device connects and restarts

### 3. Access Web UI

Visit: **[Universal Gree Remote](https://YOUR_USERNAME.github.io/esp32-gree-remote-web)**

### 4. Control Your AC

Click buttons on the web interface → MQTT message sent to ESP32 → IR command transmitted to AC unit

## Setup Steps

### Step A: Deploy Web UI to GitHub Pages

1. **Fork or create repository**: `esp32-gree-remote-web`
2. **Copy web files**: Use files from `github-pages-site/`
3. **Enable GitHub Pages**: Settings → Pages → Deploy from branch
4. **Access at**: `https://YOUR_USERNAME.github.io/esp32-gree-remote-web`

**See [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md) for detailed instructions**

### Step B: Flash ESP32 with MQTT Support

```bash
platformio run --target upload --upload-port COM7
pio device monitor
```

### Step C: Connect to WiFi

Press RESET button for 5 seconds:
- Enters provisioning mode
- Creates **UNIVERSAL IR** hotspot
- Navigate to `http://192.168.4.1`
- Select home WiFi and enter password

### Step D: Control via Web UI

Open your web UI and start controlling!

## MQTT Configuration

| Setting | Value |
|---------|-------|
| Broker | 128.199.20.163 |
| Port | 1883 |
| Username | amiuser |
| Password | password |
| Command Topic | `remote/command` |
| Status Topic | `remote/status` |

**See [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md) for detailed documentation**

## Hardware Requirements

- ESP32 development board
- IR LED + 2N2222 transistor (transmit)
- IR receiver module (optional, for debugging)
- 5V USB power supply

**See [CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md) for wiring**

## Supported AC Models

- Gree units using `YAW1F` protocol
- Tested on common Gree AC units
- Protocol: Native Gree infrared remote commands

## Files

- **[MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md)** - Complete technical architecture
- **[GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md)** - Deploying web UI to GitHub Pages
- **[WIFI_PROVISIONING.md](WIFI_PROVISIONING.md)** - WiFi setup and troubleshooting
- **[CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md)** - Hardware wiring
- **[USER_MANUAL.md](USER_MANUAL.md)** - Operation guide
- **[github-pages-site/](github-pages-site/)** - Web UI source code

## Features

### Device Features
- ✅ WiFi provisioning (5-second button press)
- ✅ MQTT client for cloud connectivity
- ✅ IR transmission (GPIO 12)
- ✅ IR reception/debug (GPIO 15)
- ✅ Persistent state saving (NVS)
- ✅ Button press: 10 seconds clears WiFi

### Web UI Features
- ✅ Realistic remote interface
- ✅ Real-time AC status display
- ✅ Mobile responsive
- ✅ No installation required
- ✅ Works offline with cached UI
- ✅ MQTT status indicator

## Web UI Commands

All buttons on the remote interface:

```
POWER       MODE        FAN
TEMP UP     SWING V     SWING H
TEMP DOWN   TURBO       SLEEP
TIMER       I FEEL      DISPLAY
WIFI        ECO         LIGHT
AUTO        COOL        DRY        HEAT
```

## Serial Monitor Output

```
ESP32 Universal Gree Remote
WiFi credentials saved: MyNetwork
Attempting to connect to saved WiFi: MyNetwork
Wi-Fi connected: MyNetwork
Connecting to MQTT broker... Connected!
MQTT Command received: {"button":"power"}
Sent IR: GREE Power Toggle
```

## Troubleshooting

### Device won't connect to MQTT
- Verify WiFi is connected (check serial monitor)
- Ping broker: `ping 128.199.20.163`
- Check MQTT credentials correct

### Web UI shows "Disconnected"
- Verify ESP32 is powered and WiFi connected
- Check browser console (F12) for errors
- Ensure MQTT broker is reachable

### WiFi provisioning won't appear
- Press reset button for exactly 5 seconds
- Check serial monitor for "Starting WiFi Provisioning"
- Try 10-second press to clear, then reprogram

### IR commands not working
- Ensure IR LED is on GPIO 12
- Point LED at AC unit
- Check AC responds to physical remote

**Full troubleshooting guide: See [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md)**

## Building and Customization

### Add More Button Commands

Edit `src/main.cpp` `isSupportedButton()` and `sendNativeGreeButtonCode()` functions.

### Change MQTT Broker

Edit `src/main.cpp`:
```cpp
constexpr char kMqttBrokerHost[] = "your.broker.com";
constexpr uint16_t kMqttBrokerPort = 1883;
```

### Customize Web UI

Edit `github-pages-site/index.html`:
```javascript
const MQTT_BROKER_HOST = 'your.broker.com';
const MQTT_USERNAME = 'your_user';
const MQTT_PASSWORD = 'your_pass';
```

## Project Structure

```
esp32-gree-remote/
├── platformio.ini                    # Build config (MQTT + IR libs)
├── src/
│   ├── main.cpp                     # WiFi + MQTT + IR firmware
│   └── remote_library.cpp           # AC state management
├── include/
│   ├── remote_library.h
│   └── wifi_config.example.h
├── github-pages-site/                # Web UI (GitHub Pages ready)
│   ├── index.html                   # Remote interface
│   ├── README.md                    # Web UI documentation
│   └── .github/workflows/pages.yml  # Auto-deployment config
├── MQTT_ARCHITECTURE.md             # Technical details
├── GITHUB_PAGES_SETUP.md            # Deployment guide
└── [other documentation files]
```

## Dependencies

**ESP32 Firmware:**
- `crankyoldgit/IRremoteESP8266` @ ^2.8.6
- `knolleary/PubSubClient` @ ^2.8.0

**Web UI:**
- Paho MQTT JS (CDN)
- HTML5/CSS3/JavaScript

## License

MIT License - See LICENSE file

## Contributing

Pull requests welcome! Test on real hardware before submitting.

## Support

- **GitHub Issues**: Report bugs or feature requests
- **Discussions**: Ask questions
- **Wiki**: Community tips and tricks

## Author

Universal Gree remote with MQTT cloud connectivity for global AC control.

---

**Next Steps:**
1. Follow [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md) to deploy web UI
2. Follow [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md) for detailed setup
3. See [WIFI_PROVISIONING.md](WIFI_PROVISIONING.md) for WiFi configuration

## API Endpoints

- `GET /` web UI
- `GET /api/status` current Wi-Fi, AC, and debug state
- `GET /api/button/send?buttonId=power&count=1` send a remote button
- `GET /api/read-ir` last captured IR value
- `GET /api/debug/capture` IR debug payload
- `GET /api/remote/clear` reset the saved AC state

## Repository Guide

- [src/main.cpp](src/main.cpp) firmware, web UI, Wi-Fi handling, API routes, IR send/receive logic
- [src/remote_library.cpp](src/remote_library.cpp) remote model and button metadata
- [include/wifi_config.example.h](include/wifi_config.example.h) Wi-Fi and network config template
- [USER_MANUAL.md](USER_MANUAL.md) setup, usage, and troubleshooting notes
- [CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md) wiring guide for the IR LED and receiver

## Limitations

- This project targets Gree `YAW1F` protocol devices, not every air conditioner brand
- The ESP32 can send commands and mirror the last saved state, but it cannot read live indoor or outdoor temperature back from the AC
- Public internet access still requires your own tunnel, reverse proxy, DDNS, or port forwarding

## Suggested GitHub About Section

Description:
ESP32 web-based IR remote for Gree air conditioners using the YAW1F protocol, with a browser UI, Wi-Fi access, flash state saving, and IR debug capture.

Topics:
`esp32`, `arduino`, `platformio`, `infrared`, `ir-remote`, `air-conditioner`, `gree`, `gree-ac`, `wifi`, `webserver`, `smart-home`, `irremoteesp8266`
#   g r e e  
 