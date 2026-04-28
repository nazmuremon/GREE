# Quick Start Guide - WiFi Provisioning

## For First-Time Setup

### What You'll Need
- ESP32 development board
- USB cable for power and programming
- Mobile phone or computer with WiFi
- Your home WiFi network name and password

### Step-by-Step Setup

#### 1. **Flash the Firmware**
```bash
cd portable_lightburn_converter/ESP32\ Universal\ Gree\ Remote/
pio run -t upload
```

#### 2. **Connect to Provisioning Hotspot**
- Device automatically creates a WiFi hotspot named **UNIVERSAL IR**
- Password: **-12345678** (note the dash!)
- Connect from your phone or computer

#### 3. **Open Provisioning Page**
- After connecting to the hotspot, a setup page should appear automatically
- If not, open browser and go to: `http://192.168.4.1`

#### 4. **Select Your WiFi**
- Tap on your home WiFi network from the list
- Or manually type the network name
- Enter your WiFi password

#### 5. **Connect**
- Tap "Connect to WiFi" button
- Wait for connection confirmation
- Device will restart and connect to your WiFi

#### 6. **Access the Dashboard**
After connection, access the remote dashboard at:
```
http://irremote.local
```

That's it! Your remote is now connected and accessible.

---

## Changing WiFi Network

### Method 1: Button Press (5 seconds)
1. Press and hold the **Reset** button for 5 seconds
2. Device enters provisioning mode again
3. Repeat setup steps above with new WiFi

### Method 2: Serial Command
Connect via serial monitor and send:
```
reset
```

---

## If Something Goes Wrong

### Forgot WiFi Password?
Press reset button for **10 seconds** to clear everything and start fresh.

### Lost Connection?
1. Check if WiFi router is on
2. Verify password is correct
3. Try moving device closer to router
4. Press reset for 10 seconds and set up again

### Can't Access Provisioning Page?
1. Make sure you're connected to **UNIVERSAL IR** hotspot
2. Try opening `http://192.168.4.1/` (with trailing slash)
3. Check serial monitor for error messages

---

## Useful Shortcuts

| Action | Duration | Result |
|--------|----------|--------|
| Button Press | 5 seconds | Enter WiFi setup mode |
| Button Press | 10 seconds | Clear WiFi & restart |
| Power cycle | - | Auto-connect to saved WiFi |

---

## Technical Details

- **Provisioning Hotspot**: UNIVERSAL IR (WPA2-PSK)
- **Password**: -12345678
- **Setup IP**: 192.168.4.1
- **Dashboard Hostname**: irremote.local
- **Credentials Storage**: ESP32 NVS (survives power cycles)
- **Reset Button**: GPIO0 (BOOT button on dev boards)

---

For detailed troubleshooting, see [WIFI_PROVISIONING.md](WIFI_PROVISIONING.md)
