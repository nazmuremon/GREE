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

After enabling WebSockets, update these constants in `index.html`:

```javascript
const MQTT_BROKER_HOST = '128.199.20.163';
const MQTT_BROKER_PORT = 8083;
const MQTT_USE_SSL = false;
const MQTT_TOPIC_COMMAND = 'ir';
const MQTT_TOPIC_STATUS = 'ir/status';
```

Use `MQTT_USE_SSL = true` only when the broker has a valid TLS WebSocket
listener, usually on `8084`.

## Deploy

Push this folder to the GitHub Pages repository or branch, then enable Pages in
repository Settings.

