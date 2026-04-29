# Circuit Diagram

This is the wiring used by the firmware:

- `GPIO12` = IR transmit
- `GPIO15` = IR receive

## Parts

- ESP32 board
- IR LED
- IR receiver module
- `2N2222` or `2N222` NPN transistor
- `1k` resistor for the transistor base
- `68 ohm` to `100 ohm` resistor for the IR LED from `5V`

## Important note about the transistor

Check the datasheet for your exact part.

`2N2222`, `PN2222`, and similar parts do not always have the same pin order. Make sure you know which leg is:

- base
- collector
- emitter

## IR transmit stage

Use the transistor as a low-side switch.

Connections:

- `GPIO12` -> `1k` resistor -> transistor base
- transistor emitter -> `GND`
- transistor collector -> IR LED cathode
- IR LED anode -> `68R` to `100R` resistor -> `+5V`
- ESP32 ground -> common ground

```text
ESP32 GPIO12 ----[1k]----B
                          |
                         |\
                         | \
                         |  \
                         |   \    2N2222 / 2N222
                         |   /
                         |  /
                         | /
                         |/
                          C---------------- IR LED cathode (-)
                          |
                          E---------------- GND

+5V ----[68R to 100R]---- IR LED anode (+)

ESP32 GND -------------------------------- GND
```

## IR receive stage

Use a normal demodulating IR receiver module such as `VS1838B` or `TSOP1838`.

Connections:

- receiver `OUT` -> `GPIO15`
- receiver `VCC` -> `3.3V`
- receiver `GND` -> `GND`

```text
Receiver VCC ---- ESP32 3.3V
Receiver GND ---- ESP32 GND
Receiver OUT ---- ESP32 GPIO15
```

## Combined view

```text
                    ESP32
          +-----------------------+
          | GPIO12 ----[1k]---- Base
          | GPIO15 <--------- Receiver OUT
          | 3.3V  ----------> Receiver VCC
          | GND   ----------> Receiver GND
          | GND   ------------+------------------+
          +-------------------|------------------+
                              |                  |
                           Emitter             5V GND
                              |
                           2N2222
                              |
                           Collector
                              |
                       IR LED cathode (-)
                              |
                +5V --[68R to 100R]-- IR LED anode (+)
```

## Extra notes

- keep grounds common
- do not drive the IR LED straight from the ESP32 pin
- keep the IR LED pointed at the AC receiver window
- if range is poor, check transistor pinout first

## Quick checks

- `GPIO12` really goes to the base resistor
- `GPIO15` really goes to the receiver output
- IR LED polarity is correct
- transistor pinout matches its datasheet
- no `5V` is fed into an ESP32 GPIO
