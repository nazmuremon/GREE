# 🎯 Your GitHub Pages Web UI - Reference Card

## 📌 Key Information

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

## 🔗 Key URLs

| Resource | URL |
|----------|-----|
| **Remote Control** | `https://YOUR_USERNAME.github.io/esp32-gree-remote-web` |
| **Repository** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web` |
| **Deployment Status** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web/actions` |
| **GitHub Settings** | `https://github.com/YOUR_USERNAME/esp32-gree-remote-web/settings/pages` |

---

## 📋 GitHub Repository Details

**Repository Name**: `esp32-gree-remote-web`
**Visibility**: Public
**Branch**: main
**Deploy From**: Root folder (/)

---

## ⚙️ ESP32 Connection Details

### WiFi Provisioning
```
Button Press Duration:  5 seconds
Hotspot Name:          UNIVERSAL IR
Hotspot Password:      12345678
Setup URL:             http://192.168.4.1
```

### MQTT Configuration
```
Broker Host:    broker.hivemq.com
Broker Port:    1883 (TCP) or 8001 (WebSocket)
Username:       (not required)
Password:       (not required)
Command Topic:  remote/command
Status Topic:   remote/status
```

---

## 📱 Access Your Remote

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

## 🚀 Quick Deployment (Copy & Paste)

### Step 1: Create Repository on GitHub
```
Name: esp32-gree-remote-web
Visibility: Public
✓ Add a README file
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
- Go to: Settings → Pages
- Branch: main
- Folder: / (root)
- Click Save

### Step 4: Done!
Your site will be live in 1-2 minutes at:
```
https://YOUR_USERNAME.github.io/esp32-gree-remote-web
```

---

## 📲 Share with Others

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

## 🔧 Update Your Web UI

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

## 📊 Status Check

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

## 🆘 Troubleshooting

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

## 📚 Documentation Quick Links

| Need Help With | Document |
|----------------|----------|
| Getting started | [QUICK_START_MQTT.md](QUICK_START_MQTT.md) |
| Setup steps | [SETUP_CHECKLIST.md](SETUP_CHECKLIST.md) |
| GitHub Pages | [GITHUB_PAGES_SETUP.md](GITHUB_PAGES_SETUP.md) |
| Technical details | [MQTT_ARCHITECTURE.md](MQTT_ARCHITECTURE.md) |
| Migration info | [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) |
| Troubleshooting | Each guide has section |

---

## 🎮 Supported Buttons

All AC controls:
- **Power**: On/Off
- **Mode**: Cool, Heat, Dry, Auto, Fan
- **Temperature**: Up/Down
- **Fan**: Speed control
- **Swing**: Vertical/Horizontal
- **Timer**: Turn on/off
- **Features**: Sleep, Turbo, ECO, I Feel, Light, Display, WiFi

---

## ⏰ Important: WiFi Provisioning

**Only accessible during provisioning mode:**

1. Press RESET button for **exactly 5 seconds**
2. Device creates **UNIVERSAL IR** hotspot (password: **12345678**)
3. Connect and open **http://192.168.4.1**
4. Select WiFi and enter password
5. Device restarts with new WiFi

---

## 🔒 Security Notes

- ✅ MQTT: Public HiveMQ broker (no credentials needed)
- ✅ Provisioning hotspot password: `12345678`
- ✅ WiFi credentials stored securely in ESP32 NVS
- ✅ MQTT uses standard TCP protocol
- ⚠️ For public access, consider using VPN or firewall rules

---

## 📞 One-Page Checklist

```
□ Create GitHub repository (esp32-gree-remote-web)
□ Clone repository locally
□ Copy files from github-pages-site/
□ Commit and push to GitHub
□ Enable GitHub Pages (Settings → Pages)
□ Wait for deployment to complete
□ Note your web UI URL
□ Flash ESP32 firmware
□ WiFi provisioning (5-sec button)
□ Test web UI access
□ Click buttons and verify AC responds
□ Save bookmark: https://YOUR_USERNAME.github.io/esp32-gree-remote-web
□ Share URL with others if desired
```

---

## 🎉 Success!

Once everything works:
- ✅ You have global AC control
- ✅ Your web UI is live
- ✅ ESP32 is connected to MQTT
- ✅ Ready for daily use

**Enjoy your universal remote! 🚀**

---

## 📝 Your Setup Notes

```
GitHub Username:        [                        ]
Repository Name:        esp32-gree-remote-web
Your Web UI URL:        https://_________________.github.io/esp32-gree-remote-web
ESP32 Port:             [                        ]
WiFi SSID:              [                        ]
WiFi Password:          [                        ]
Backup URL:             http://192.168.4.1 (provisioning)
MQTT Broker:            broker.hivemq.com
MQTT Port:              1883 (ESP32) / 8001 (Web UI)
```

Print this page or save as PDF for future reference!

---

**Last Updated**: April 28, 2026  
**Version**: MQTT Architecture  
**Status**: Production Ready
