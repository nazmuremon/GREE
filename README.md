# ESP32 Universal Gree Remote

This repository is split so the ESP32 flash project is separate from
documentation and web files.

## Folder Layout

- `esp-firmware/` - the only PlatformIO project to build and flash to ESP32
- `web/` - standalone browser remote UI source
- `index.html` - GitHub Pages entry point copied from `web/index.html`
- `docs/` - documentation, guides, diagrams, and reference pages
- `github-pages-site/` - separate GitHub Pages checkout/mirror

Do not run PlatformIO upload from the repository root. Run it from
`esp-firmware/` so only firmware code is part of the build.

## Flash ESP32

```bash
cd esp-firmware
platformio run
platformio run --target upload --upload-port COM7
platformio device monitor --port COM7 --baud 115200
```

Use your actual ESP32 COM port if it is not `COM7`.

## MQTT Settings

- Broker: `128.199.20.163`
- ESP32 TCP port: `1883`
- Username: `amiuser`
- Password: `password`
- Command topic: `ir`
- Status topic: `ir/status`

The ESP32 accepts JSON payloads like `{"button":"power"}` and plain button
payloads like `power`.

## GitHub Pages MQTT

Browsers cannot open raw MQTT TCP connections to port `1883`. GitHub Pages is
HTTPS, so the browser UI must use MQTT over secure WebSocket:

```javascript
const MQTT_BROKER_HOST = '128.199.20.163';
const MQTT_BROKER_PORT = 8084;
const MQTT_USE_SSL = true;
const MQTT_TOPIC_COMMAND = 'ir';
const MQTT_TOPIC_STATUS = 'ir/status';
```

The broker must expose a TLS WebSocket listener such as
`wss://128.199.20.163:8084/` before the hosted page can connect.


