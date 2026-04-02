# ESP32 Universal Gree Remote

ESP32 web-based IR remote for Gree air conditioners that use the `YAW1F` protocol. This project turns an ESP32 into a Wi-Fi AC remote with a browser UI, persistent state saving, fallback access point mode, and IR capture/debug support.

If you want to control a Gree AC from your phone, tablet, or laptop without building a full smart home stack, this firmware serves a remote-style web page and sends the matching infrared commands through an IR LED.

## What This Project Does

- Serves a built-in web remote from the ESP32
- Sends native Gree `YAW1F` IR commands on `GPIO12`
- Reads IR signals for debugging on `GPIO15`
- Saves the last AC state in flash and restores it after reboot
- Connects to your Wi-Fi network or starts its own fallback AP
- Supports local IP access, `gree.local`, and an optional public base URL

## Best Fit For

- Gree air conditioner projects
- ESP32 IR remote builds
- DIY Wi-Fi AC controller setups
- PlatformIO + Arduino infrared projects
- Users who want a simple local web interface instead of a cloud app

## Key Features

- Remote-style single-page UI that looks and behaves like a handheld AC remote
- Native control for power, mode, fan, temperature, swing, timer, sleep, turbo, eco, light, Wi-Fi, I Feel, and display options
- Flash-backed state persistence using `Preferences`
- Fallback access point mode when Wi-Fi is missing or fails
- mDNS hostname support with `http://gree.local/`
- Debug IR reader to inspect captured protocol, bit count, raw length, and hex value
- Simple HTTP API for status, button sending, reset, and IR capture

## Supported Hardware And Protocol

- ESP32 development board
- Gree AC units that respond to the `YAW1F` infrared protocol
- IR LED transmit stage driven by a `2N2222` or similar transistor
- IR receiver module such as `VS1838B` or `TSOP1838`

Wiring details are in [CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md).

## How It Works

The firmware uses `IRremoteESP8266` to build and transmit Gree AC commands. The ESP32 exposes a local web server, renders the remote UI directly from firmware, and keeps the last known AC state in flash memory so the interface stays in sync after restart. If normal Wi-Fi is unavailable, the board starts a fallback hotspot so you can still open the remote from a browser.

## Build And Flash

1. Copy [include/wifi_config.example.h](include/wifi_config.example.h) to `include/wifi_config.h`
2. Add your Wi-Fi SSID and password
3. Optional: keep `WIFI_USE_STATIC_IP` enabled if you want a fixed LAN address
4. Build and upload with PlatformIO

```bash
pio run
pio run --target upload
pio device monitor
```

## Open The Web Remote

On your normal Wi-Fi network, try:

- `http://<device-ip>/`
- `http://gree.local/`

If Wi-Fi is not configured or the connection fails, the ESP32 starts its own access point:

- SSID: `ESP32-IR-Bridge`
- Password: `irbridge123`
- URL: `http://192.168.4.1/`

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
