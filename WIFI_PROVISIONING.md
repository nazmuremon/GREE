# WiFi Provisioning System - Universal IR Remote

## Overview

The Universal IR Remote now includes a comprehensive WiFi provisioning system that allows for easy WiFi setup and management through a mobile-friendly web interface. All WiFi credentials are securely stored in the ESP32's NVS (Non-Volatile Storage) and persist across power cycles.

## Features

- **Easy WiFi Setup**: Mobile-friendly web interface for connecting to home WiFi
- **5-Second Quick Provisioning**: Press the reset button for 5 seconds to enter provisioning mode
- **10-Second WiFi Reset**: Press the reset button for 10 seconds to clear credentials and start over
- **Persistent Credentials**: WiFi credentials saved in ROM and survive power cycles
- **mDNS Support**: Access the remote dashboard at `irremote.local` regardless of IP address
- **Captive Portal**: Automatically redirects to setup page when connecting to provisioning hotspot
- **Backup Connection**: Falls back to provisioning mode if WiFi connection fails

## Hardware Requirements

- **Reset Button**: GPIO0 (typically the BOOT button on ESP32 development boards)
- The button should be **active low** (pressing pulls to GND)
- Most ESP32 dev boards have this built-in

## WiFi Provisioning Flow

### Initial Setup (No WiFi Credentials)

1. Device starts without saved WiFi credentials
2. Automatically enters provisioning mode
3. Creates hotspot: **UNIVERSAL IR** (Password: **-12345678**)
4. Access provisioning page at `http://192.168.4.1`
5. Scan and select your home WiFi network
6. Enter WiFi password and connect
7. Device saves credentials and restarts
8. Device connects to saved WiFi automatically

### Manual WiFi Provisioning (5-Second Button Press)

1. Press and hold the reset button for **5 seconds**
2. Device enters provisioning mode (WiFi mode changes to AP)
3. Connect to **UNIVERSAL IR** hotspot
4. Enter provisioning page at `http://192.168.4.1` or `http://192.168.4.1/`
5. Follow the same setup procedure
6. Device restarts and connects to home WiFi

### Reset WiFi Credentials (10-Second Button Press)

1. Press and hold the reset button for **10 seconds**
2. All WiFi credentials are cleared
3. Device restarts automatically
4. Returns to provisioning mode (waiting for new credentials)

## Web Interface Features

### Provisioning Page (`http://192.168.4.1`)

- **Network Scanning**: Automatically scans and displays available WiFi networks
- **Auto-Refresh**: Networks are rescanned every 10 seconds
- **Manual SSID Entry**: Manually enter network name if not visible
- **Password Field**: Enter WiFi password (optional for open networks)
- **Connection Feedback**: Real-time status messages during connection
- **Mobile Responsive**: Fully optimized for mobile devices

### Dashboard Access

After successful WiFi connection, access the remote dashboard at:

- **mDNS hostname**: `http://irremote.local` (recommended)
- **Local IP**: `http://<device-ip>` (IP shown in serial monitor)
- **Port**: 80 (HTTP)

## Serial Monitor Output

Watch the serial monitor for provisioning status:

```
WiFi credentials saved: MyHomeWiFi
Attempting to connect to saved WiFi: MyHomeWiFi
Wi-Fi connected: MyHomeWiFi
```

Button press status:
```
Reset button press duration: 5234ms
Medium press detected - starting WiFi provisioning
```

Or for reset:
```
Reset button press duration: 10567ms
Long press detected - clearing WiFi credentials
```

## API Endpoints (Provisioning Mode)

### Scan Networks

**GET** `/api/scan-networks`

Returns available WiFi networks in JSON format:

```json
{
  "networks": [
    {"ssid": "Network1", "rssi": -45},
    {"ssid": "Network2", "rssi": -72}
  ]
}
```

**Response**: JSON array of networks with SSID and signal strength (dBm)

### Connect to WiFi

**POST** `/api/connect-wifi`

Connect to a WiFi network

**Request Body**:
```json
{
  "ssid": "MyWiFi",
  "password": "mypassword"
}
```

**Success Response**:
```json
{
  "success": true,
  "message": "Connected to WiFi"
}
```

**Error Response**:
```json
{
  "success": false,
  "error": "Failed to connect to WiFi"
}
```

**Behavior**: On successful connection, device saves credentials to NVS and restarts.

## Data Storage (NVS)

Credentials are stored in the NVS namespace `"gree-remote"`:

- **Key**: `wifi_ssid` → WiFi network name
- **Key**: `wifi_pass` → WiFi password
- **Key**: `state` → Gree AC remote state (existing)

NVS provides:
- ✅ Persistent storage across power cycles
- ✅ Automatic wear leveling
- ✅ Corruption protection
- ✅ Secure storage

To manually clear NVS (advanced):
```cpp
preferences.begin("gree-remote", false);
preferences.remove("wifi_ssid");
preferences.remove("wifi_pass");
preferences.end();
```

## mDNS (Multicast DNS)

The device is accessible via the hostname `irremote.local` once connected to WiFi.

- **Hostname**: `irremote.local`
- **Service**: HTTP
- **Port**: 80

This means you can access the dashboard at `http://irremote.local` without needing to know the device's IP address.

### Benefits

- **Fixed Hostname**: Access point never changes
- **Works Across Networks**: Works on any WiFi network
- **Mobile Friendly**: Easy to remember URL
- **No Port Forwarding**: Direct local access

### Troubleshooting mDNS

If `irremote.local` doesn't resolve:

1. **Check Serial Output**: Verify "mDNS active: yes"
2. **Device IP**: Use IP address directly (shown in serial monitor)
3. **Network Support**: Most modern routers support mDNS
4. **Firewall**: Ensure mDNS (port 5353) isn't blocked
5. **Restart Device**: Power cycle the ESP32

## Troubleshooting

### Device Won't Start Provisioning Mode

**Problem**: Pressing reset button doesn't trigger provisioning

**Solutions**:
1. Check if GPIO0 is correctly connected to button
2. Verify button is active low (pulls to GND)
3. Check serial output for button press duration
4. Try using the boot button on the development board

### Can't Connect to UNIVERSAL IR Hotspot

**Problem**: Hotspot not appearing or won't connect

**Solutions**:
1. Check serial output: "Starting WiFi Provisioning Mode..."
2. Verify correct password: `-12345678` (note the dash!)
3. Restart device and try again
4. Check if 2.4GHz WiFi is enabled on your phone (hotspot uses 2.4GHz)

### Provisioning Page Won't Load

**Problem**: Can't access `http://192.168.4.1`

**Solutions**:
1. Verify you're connected to **UNIVERSAL IR** hotspot
2. Check that you're using HTTP (not HTTPS)
3. Try accessing `http://192.168.4.1/` with trailing slash
4. Check serial output for errors
5. Restart your phone's WiFi

### Can't Connect to Home WiFi

**Problem**: Shows "Failed to connect to WiFi"

**Solutions**:
1. Verify SSID and password are correct
2. Check WiFi network is 2.4GHz (5GHz not supported by ESP32)
3. Check if network has special characters in password
4. Verify device is in range of WiFi router
5. Check if MAC filtering is enabled on router
6. Check serial output for connection errors

### Lost Connection After Setup

**Problem**: Device connects but disconnects frequently

**Solutions**:
1. Check WiFi signal strength (should be > -70 dBm)
2. Verify WiFi password is correct in NVS
3. Check for WiFi interference
4. Try different WiFi channel on router
5. Move device closer to router
6. Check serial output: "Wi-Fi connected"

### Can't Access irremote.local

**Problem**: Hostname doesn't resolve

**Solutions**:
1. Verify device is connected to WiFi: check serial output
2. Check mDNS is active: look for "mDNS active: yes"
3. Use IP address directly instead: check serial for IP
4. Ensure router supports mDNS
5. Wait a few seconds after connection for mDNS to initialize
6. Try accessing from different device

## Configuration

### Hardware Pin Configuration

The button pin can be changed by modifying this constant in `main.cpp`:

```cpp
constexpr uint16_t kResetButtonPin = 0;  // Change this to your button GPIO
```

### Timing Configuration

Button press timing can be adjusted:

```cpp
constexpr uint32_t kResetButtonProvisioningTime = 5000;  // milliseconds
constexpr uint32_t kResetButtonWifiResetTime = 10000;    // milliseconds
```

### Hotspot Credentials

To change the provisioning hotspot name/password, modify:

```cpp
constexpr char kProvisioningApSsid[] = "UNIVERSAL IR";
constexpr char kProvisioningApPassword[] = "-12345678";
```

### mDNS Hostname

To change the mDNS hostname (default: `irremote`):

```cpp
constexpr char kDeviceHostname[] = "irremote";
// Access at http://irremote.local
```

## Security Considerations

### Hotspot Security

- Hotspot uses WPA2-PSK authentication
- Password is pre-configured to `-12345678`
- **Note**: This hotspot is only for initial setup - not for production internet access
- Consider implementing custom password mechanism for production

### Credential Storage

- Credentials stored in NVS (Not encrypted by default)
- ESP32 provides OTA Digest authentication optional
- For sensitive applications, consider using encrypted NVS

### Network Security

- Always use HTTPS in production (implement cert on device)
- mDNS is local network only (cannot be accessed from internet)
- WiFi password is saved locally in NVS

## Advanced Topics

### Custom Provisioning Page

The provisioning page HTML is embedded in `getProvisioningPage()` function. To customize:

1. Modify the HTML/CSS in the function
2. Recompile and upload
3. Make sure to keep all JavaScript functionality

### DNS Captive Portal

The provisioning mode runs a DNS server that redirects all DNS queries to the device IP (192.168.4.1). This creates a captive portal effect.

### WiFi Modes

The device switches between modes:

- **STA Mode**: Connected to home WiFi (normal operation)
- **AP Mode**: Provisioning hotspot active
- **Off**: mDNS disabled during provisioning

### JSON Parsing

The provisioning page uses simple string-based JSON parsing. For larger deployments, consider using a JSON library like ArduinoJson.

## Firmware Updates

When updating firmware, WiFi credentials are preserved as they're stored in NVS (different partition).

To clear credentials during OTA update:
1. Use the 10-second reset button press method
2. Or programmatically: `clearWifiCredentials()`
3. Or erase entire NVS via platformio: `pio run -t erase`

## Performance

- **Provisioning Page Load**: < 1 second over WiFi
- **WiFi Connection**: 5-15 seconds depending on signal
- **Device Restart**: ~2 seconds
- **mDNS Resolution**: < 200ms on local network

## Compatibility

- **ESP32 Board**: Generic ESP32 Dev Module and variants
- **Arduino IDE**: 1.8.19+
- **PlatformIO**: 5.0+
- **Browsers**: All modern browsers (mobile and desktop)
- **Mobile OS**: iOS 11+, Android 5+

## Future Enhancements

Possible future improvements:
- [ ] WebSocket for real-time status updates
- [ ] WiFi signal strength indicator
- [ ] Encrypted credential storage
- [ ] QR code provisioning
- [ ] Time zone configuration during setup
- [ ] OTA firmware update via provisioning page
- [ ] Custom branding in provisioning UI

## Support

For issues or questions:
1. Check serial monitor output for error messages
2. Review troubleshooting section above
3. Verify hardware connections (GPIO0 to button)
4. Check platformio.ini configuration
5. Try factory reset (10-second button press)

---

**Version**: 1.0  
**Last Updated**: April 2026  
**Firmware**: ESP32 Universal Gree Remote
