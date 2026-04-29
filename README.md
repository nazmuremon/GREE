# Universal Gree Remote Web UI

This folder contains the GitHub Pages remote-control UI.

## MQTT Settings

The firmware uses:

- Broker: `128.199.20.163`
- TCP port: `1883`
- Username: `amiuser`
- Password: `password`
- Command topic: `ir`
- Status topic: `ir/status`

## Browser Requirement

Browsers cannot open raw MQTT TCP connections to port `1883`. GitHub Pages can
only use MQTT if the broker exposes MQTT over WebSocket.

Common broker WebSocket ports to enable:

- `8083` for `ws://`
- `8084` for `wss://`
- `9001` for Mosquitto-style WebSocket setups

For the live GitHub Pages site, enable a TLS WebSocket listener and use:

```javascript
const MQTT_BROKER_HOST = '128.199.20.163';
const MQTT_BROKER_PORT = 8084;
const MQTT_USE_SSL = true;
const MQTT_TOPIC_COMMAND = 'ir';
const MQTT_TOPIC_STATUS = 'ir/status';
```

Plain `ws://` works only from a plain HTTP page. GitHub Pages is HTTPS, so it
requires `wss://`.

## Deploy

Push this folder to the GitHub Pages repository or branch, then enable Pages in
repository Settings.

