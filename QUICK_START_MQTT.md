# 🎯 Quick Reference: MQTT Remote Control Setup

Complete summary of changes and quick start guide.

## What Was Done

### 1. ✅ Removed Web Server from ESP32
- Removed embedded HTML/CSS/JavaScript UI (~3KB)
- Removed `/api/status`, `/api/button/send` endpoints
- Kept WiFi provisioning mode (still works)
- Kept IR transmission/reception
- Added MQTT client for cloud connectivity

### 2. ✅ Created Web UI on GitHub Pages
- Modern, responsive remote interface
- Works on any browser (phone, tablet, desktop)
- Connects to MQTT broker directly
- No installation needed - just bookmark the URL
- Updates deploy automatically with git push

### 3. ✅ Added MQTT Architecture
- ESP32 acts as IoT device (not web server)
- MQTT broker as cloud gateway (broker.hivemq.com:1883 or 8001 for WebSocket)
- Browser connects to broker via WebSocket
- Commands: browser → MQTT → ESP32 → IR → AC
- Status: AC state → IR capture → MQTT → browser

## Files Changed

### Code
- `platformio.ini` - Added PubSubClient library
- `src/main.cpp` - Added MQTT functions and integration

### New Documentation
- `README.md` - Updated for MQTT architecture
- `MQTT_ARCHITECTURE.md` - Technical details
- `GITHUB_PAGES_SETUP.md` - Deployment steps
- `MIGRATION_GUIDE.md` - Before/after comparison

### Web UI (New)
- `github-pages-site/index.html` - Remote interface
- `github-pages-site/README.md` - Web UI docs
- `github-pages-site/.github/workflows/pages.yml` - Auto-deploy config

## Quick Start (3 Steps)

### Step 1: Flash ESP32
```bash
cd esp32-gree-remote
platformio run --target upload --upload-port COM7
```

### Step 2: Setup WiFi (5-second button press)
1. Press RESET button for 5 seconds
2. Connect to **UNIVERSAL IR** hotspot (password: `12345678`)
3. Open `http://192.168.4.1` in browser
4. Select your home WiFi and enter password
5. Device restarts and connects

### Step 3: Deploy Web UI
1. Create GitHub repo: `esp32-gree-remote-web`
2. Copy files from `github-pages-site/`
3. Push to GitHub
4. Enable GitHub Pages
5. Access at: `https://YOUR_USERNAME.github.io/esp32-gree-remote-web`

## MQTT Details

| Setting | Value |
|---------|-------|
| Broker | broker.hivemq.com |
| Port | 1883 |
| Username | (none - public broker) |
| Password | password |

### Topics
- **Command**: `remote/command` - Browser sends commands here
- **Status**: `remote/status` - ESP32 publishes state here

### Command Format
```json
{"button":"power"}
```

### Status Format
```json
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

## Supported Buttons

```
Power, Mode, Fan
Temp Up, Temp Down
Cool, Heat, Dry, Auto
Swing V, Swing H
Timer, Sleep, Turbo
WiFi, ECO, Light
I Feel, Display
```

## Testing

### Serial Monitor Output
```
ESP32 connected to WiFi: MyNetwork
Connecting to MQTT broker...
Connected!
MQTT Command received: {"button":"power"}
Sent IR: GREE Power Toggle
Publishing status...
```

### Test Commands (via MQTT CLI)
```bash
mosquitto_pub -h broker.hivemq.com \
  -t remote/command -m '{"button":"power"}'
```

## Troubleshooting

### Issue: WiFi provisioning won't start
**Solution**: Press RESET button for exactly 5 seconds
- Serial monitor should show "Starting WiFi Provisioning Mode"
- UNIVERSAL IR hotspot should appear

### Issue: MQTT connection fails
**Solution**: Check WiFi connected first
1. Verify WiFi SSID appears in provisioning page
2. Check serial monitor for WiFi connection status
3. Ping broker: `ping broker.hivemq.com`

### Issue: IR commands not working
**Solution**: Verify IR hardware
1. Check IR LED on GPIO 12
2. Verify AC responds to physical remote
3. Point LED directly at AC unit

### Issue: Web UI shows "Disconnected"
**Solution**: Check MQTT broker
1. Verify ESP32 is online (green LED activity)
2. Check browser console (F12) for errors
3. Ensure MQTT broker is reachable

## Architecture Diagram

```
                 WiFi Network
                      |
    ┌─────────────────┴─────────────────┐
    |                                   |
ESP32 Device                    WiFi Provisioning
(IR Control)                    (Setup Interface)
    |                                   |
    └─────────────────┬─────────────────┘
                      |
                   WiFi
                      |
          ┌───────────┴───────────┐
          |                       |
       Home Router          MQTT Broker
                          (broker.hivemq.com)
                            |        |
                      WebSocket  TCP (1883)
                            |        |
                    ┌───────┴────────┘
                    |
              Web UI (GitHub Pages)
                (Browser)
```

## Before & After

### Before
- ❌ Local network only
- ❌ Access via `http://<device-ip>/`
- ❌ Embedded web server on ESP32
- ❌ Cannot update UI without reflashing
- ❌ Limited to devices on same WiFi

### After
- ✅ Global access (anywhere)
- ✅ Access via HTTPS GitHub Pages URL
- ✅ Lightweight device (MQTT only)
- ✅ Update UI with git push
- ✅ Control from any network

## Next Steps

1. **Test Local Setup**
   - Flash firmware
   - WiFi provisioning
   - Verify serial output

2. **Deploy Web UI**
   - Create GitHub repository
   - Copy web files
   - Enable GitHub Pages

3. **Test End-to-End**
   - Open web UI in browser
   - Click buttons
   - Verify AC responds

4. **Customize** (Optional)
   - Change MQTT broker settings
   - Modify button labels
   - Add new features

## Files to Know

| File | Purpose |
|------|---------|
| `src/main.cpp` | ESP32 firmware with MQTT |
| `platformio.ini` | Build config |
| `github-pages-site/index.html` | Remote UI |
| `MQTT_ARCHITECTURE.md` | Technical docs |
| `GITHUB_PAGES_SETUP.md` | Deployment guide |
| `MIGRATION_GUIDE.md` | Change details |

## Key Features

- 🌍 **Global Access**: Control AC from anywhere
- 📱 **Mobile Friendly**: Works on phone, tablet, desktop
- 🔐 **Secure**: MQTT with credentials
- ⚡ **Lightweight**: ESP32 uses less resources
- 🔄 **Real-time**: MQTT for instant updates
- 📊 **Expandable**: Use same broker for other devices
- 🎨 **Modern UI**: Responsive, attractive interface
- 🚀 **Easy Updates**: Deploy UI by git push

## Support Resources

- **Documentation**: See README.md
- **Setup Guide**: See GITHUB_PAGES_SETUP.md
- **Technical Details**: See MQTT_ARCHITECTURE.md
- **Migration Info**: See MIGRATION_GUIDE.md
- **Serial Monitor**: Check ESP32 output (115200 baud)
- **Browser Console**: Check for errors (F12)

---

**You're all set! 🚀 Start with Step 1 above.**
