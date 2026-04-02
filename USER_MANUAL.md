# User Manual

## Basic idea

This project turns an ESP32 into a browser-based Gree remote.

Open the page, tap the buttons, and the ESP32 sends the matching IR command through the LED. The last state is saved in flash, so after reboot the page comes back with the same mode, temperature, and options.

## What you need

- ESP32 board
- IR LED on `GPIO12`
- IR receiver on `GPIO15`
- `2N2222` or `2N222` transistor for the IR LED driver
- Wi-Fi, unless you want to use the fallback AP mode

Wiring is in [CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md).

## Setup

1. Copy [include/wifi_config.example.h](include/wifi_config.example.h) to `include/wifi_config.h`
2. Put in your Wi-Fi name and password
3. If you want the same IP every time, keep `WIFI_USE_STATIC_IP` enabled and set the IP values for your LAN
4. Flash the board

```bash
pio run
pio run --target upload
```

## Opening the page

Try:

- `http://<device-ip>/`
- `http://gree.local/`

If Wi-Fi is missing or fails, the board starts an access point:

- SSID: `ESP32-IR-Bridge`
- Password: `irbridge123`

Then open:

- `http://192.168.4.1/`

## Buttons

- `POWER` toggles the AC
- `MODE` cycles modes
- `FAN` cycles fan speed
- `TEMP UP` and `TEMP DN` change temperature and force `Cool`
- `SWING V` steps vertical swing
- `SWING H` changes horizontal swing
- `TIMER` adds 30 minutes each press
- `SLEEP`, `TURBO`, `I FEEL`, `WIFI`, `ECO`, `LIGHT`, `DISPLAY` toggle those options
- `AUTO`, `COOL`, `DRY`, `HEAT` jump straight to that mode

## Saved state

The board saves the current Gree state in flash. If it reboots, it loads that state again.

## Reading another remote

The IR reader is on by default.

You can see the last captured value in:

- the `IR Debug Reader` section on the page
- `GET /api/read-ir`
- `GET /api/debug/capture`

It shows:

- protocol
- bit count
- raw length
- hex value

## Serial commands

Open the serial monitor at `115200`.

Commands:

- `help`
- `status`
- `reset`

## Same bookmark on local Wi-Fi

Use `include/wifi_config.h` and keep `WIFI_USE_STATIC_IP` enabled. Set the IP to an unused address on your router subnet, for example:

- `192.168.1.60`

Then bookmark:

- `http://192.168.1.60/`

## Outside access

If you want to open it from another network, you still need your own network setup:

- tunnel
- DDNS
- reverse proxy
- port forwarding

The ESP32 cannot do that part by itself.

## Troubleshooting

If the page does not open:

- check power
- check serial output for the IP
- try the fallback AP

If `gree.local` does not open:

- use the IP directly
- or keep the fixed IP setting enabled

If IR receive is not showing values:

- make sure the receiver module is on `GPIO15`
- point the remote straight at it
- test from close range

If transmit range is poor:

- check the transistor wiring
- check IR LED polarity
- see [CIRCUIT_DIAGRAM.md](CIRCUIT_DIAGRAM.md)
