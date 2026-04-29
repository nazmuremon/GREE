# ESP Firmware

This folder is the ESP32 PlatformIO project. Build and upload from here only.

```bash
platformio run
platformio run --target upload --upload-port COM7
platformio device monitor --port COM7 --baud 115200
```

Only these folders are used by PlatformIO for the firmware:

- `src/`
- `include/`
- `lib/`
- `test/`
- `platformio.ini`

Documentation and web UI files live outside this folder and are not flashed to
the ESP32.


