# ðŸŽ¯ Your GitHub Pages Web UI - Reference Card

## ðŸ“Œ Key Information

### Your Remote Control URL
After deploying to GitHub Pages, your remote will be at:

```
https://YOUR_USERNAME.github.io/esp32-gree-remote-web
```

**Example**: If your GitHub username is `john-doe`, your URL will be:
```
https://john-doe.github.io/esp32-gree-remote-web
```

---

## ðŸ”— Key URLs

| Resource | URL |
|----------|-----|
| **Remote Control** | `https://YOUR_USERNAME.github.io/esp32-gree-remote-web` |
| **Repository** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web` |
| **Deployment Status** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web/actions` |
| **GitHub Settings** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web/settings/pages` |

---

## ðŸ“‹ GitHub Repository Details

**Repository Name**: `esp32-gree-remote-web`
**Visibility**: Public
**Branch**: main
**Deploy From**: Root folder (/)

---

## âš™ï¸ ESP32 Connection Details

### WiFi Provisioning
```
Button Press Duration:  5 seconds
Hotspot Name:          UNIVERSAL IR
Hotspot Password:      12345678
Setup URL:             http://192.168.4.1
```

### MQTT Configuration
```
Broker Host:    128.199.20.163
Broker Port:    1883 (ESP32 TCP); browser needs a WebSocket port
Username:       amiuser
Password:       password
Command Topic:  ir
Status Topic:   ir/status
```

---

## ðŸ“± Access Your Remote

### From Desktop Browser
1. Open: `https://YOUR_USERNAME.github.io/esp32-gree-remote-web`
2. Bookmark it for quick access
3. Start controlling your AC

### From Mobile Browser
1. Use same URL as desktop
2. Fully responsive design
3. Add to home screen for app-like experience

### From Any Device
- Works on Mac, Windows, Linux
- Works on iPhone, Android
- Works on tablets
- Works offline (UI cached)

---

## ðŸš€ Quick Deployment (Copy & Paste)

### Step 1: Create Repository on GitHub
```
Name: esp32-gree-remote-web
Visibility: Public
âœ“ Add a README file
```

### Step 2: Clone and Add Files
```bash
git clone https://github.com/YOUR_USERNAME/esp32-gree-remote-web.git
cd esp32-gree-remote-web
cp -r /path/to/github-pages-site/* .
git add .
git commit -m "Initial: Universal Gree Remote"
git push origin main
```

### Step 3: Enable GitHub Pages
- Go to: Settings â†’ Pages
- Branch: main
- Folder: / (root)
- Click Save

### Step 4: Done!
Your site will be live in 1-2 minutes at:
```
https://YOUR_USERNAME.github.io/esp32-gree-remote-web
```

---

## ðŸ“² Share with Others

Share this link with anyone who wants to control your AC:

```
https://YOUR_USERNAME.github.io/esp32-gree-remote-web
```

They just need:
- Any web browser
- Internet connection
- Your MQTT broker running (should already be up)
- Your ESP32 connected to WiFi

---

## ðŸ”§ Update Your Web UI

To update the web UI later:

```bash
cd esp32-gree-remote-web

# Make changes to index.html or other files

git add .
git commit -m "Update remote UI"
git push origin main

# GitHub Pages will auto-deploy in 1-2 minutes
```

---

## ðŸ“Š Status Check

### Is Everything Working?

Open your web UI and check:
- [ ] Page loads in browser
- [ ] Green "Connected" indicator shows
- [ ] All buttons visible
- [ ] AC status displays
- [ ] Can click buttons
- [ ] Commands send to ESP32
- [ ] AC responds

---

## ðŸ†˜ Troubleshooting

### Web UI won't load
- Check URL spelling: `https://YOUR_USERNAME.github.io/esp32-gree-remote-web`
- Wait 2-3 minutes after first push
- Check Actions tab for deployment status

### Shows "Disconnected"
- Verify ESP32 is powered on
- Check ESP32 connected to WiFi (serial monitor)
- Verify MQTT broker reachable
- Check browser console (F12) for errors

### Buttons don't work
- Verify connected status is green
- Check serial monitor shows MQTT connected
- Verify IR LED on GPIO 12
- Check AC responds to physical remote

---

## ðŸ“š Documentation Quick Links

| Need Help With | Document |
|----------------|----------|
| Getting started | [QUICK_START_MQTT.md](QUICK_START_MQTT.md) |
| Setup steps | [SETUP_CHECKLIST.md](SETUP_CHECKLIST.md) |
| GitHub Pages | [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md) |
| Technical details | [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md) |
| Migration info | [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) |
| Troubleshooting | Each guide has section |

---

## ðŸŽ® Supported Buttons

All AC controls:
- **Power**: On/Off
- **Mode**: Cool, Heat, Dry, Auto, Fan
- **Temperature**: Up/Down
- **Fan**: Speed control
- **Swing**: Vertical/Horizontal
- **Timer**: Turn on/off
- **Features**: Sleep, Turbo, ECO, I Feel, Light, Display, WiFi

---

## â° Important: WiFi Provisioning

**Only accessible during provisioning mode:**

1. Press RESET button for **exactly 5 seconds**
2. Device creates **UNIVERSAL IR** hotspot (password: **12345678**)
3. Connect and open **http://192.168.4.1**
4. Select WiFi and enter password
5. Device restarts with new WiFi

---

## ðŸ”’ Security Notes

- âœ… MQTT: Private MQTT broker (no credentials needed)
- âœ… Provisioning hotspot password: `12345678`
- âœ… WiFi credentials stored securely in ESP32 NVS
- âœ… MQTT uses standard TCP protocol
- âš ï¸ For public access, consider using VPN or firewall rules

---

## ðŸ“ž One-Page Checklist

```
â–¡ Create GitHub repository (esp32-gree-remote-web)
â–¡ Clone repository locally
â–¡ Copy files from github-pages-site/
â–¡ Commit and push to GitHub
â–¡ Enable GitHub Pages (Settings â†’ Pages)
â–¡ Wait for deployment to complete
â–¡ Note your web UI URL
â–¡ Flash ESP32 firmware
â–¡ WiFi provisioning (5-sec button)
â–¡ Test web UI access
â–¡ Click buttons and verify AC responds
â–¡ Save bookmark: https://YOUR_USERNAME.github.io/esp32-gree-remote-web
â–¡ Share URL with others if desired
```

---

## ðŸŽ‰ Success!

Once everything works:
- âœ… You have global AC control
- âœ… Your web UI is live
- âœ… ESP32 is connected to MQTT
- âœ… Ready for daily use

**Enjoy your universal remote! ðŸš€**

---

## ðŸ“ Your Setup Notes

```
GitHub Username:        [                        ]
Repository Name:        esp32-gree-remote-web
Your Web UI URL:        https://_________________.github.io/esp32-gree-remote-web
ESP32 Port:             [                        ]
WiFi SSID:              [                        ]
WiFi Password:          [                        ]
Backup URL:             http://192.168.4.1 (provisioning)
MQTT Broker:            128.199.20.163
MQTT Port:              1883 (ESP32 TCP); Web UI needs WebSocket
```

Print this page or save as PDF for future reference!

---

**Last Updated**: April 28, 2026  
**Version**: MQTT Architecture  
**Status**: Production Ready

