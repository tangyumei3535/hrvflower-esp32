# ESP-SensairShuttle (`esp_SensairShuttle`)

[中文](./README_CN.md)

ESP32-C5-WROOM-1-N16R8 · 16MB Flash · 8MB PSRAM

1.83" ILI9341 (284×240 logical, swap_xy), CST816S touch (I2C `0x15`). Standard full-screen UI layout.

Docs: [ESP-SensairShuttle v1.0 user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c5/esp-sensairshuttle/user_guide_v1.0.html)

## Pins

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA / SCL | 2 / 3 | CST816S `0x15`, BS8112 touch keys |
| SPI MOSI / SCK / CS | 23 / 24 / 25 | |
| LCD DC | 26 | ili9341 driver + custom init table |
| PA enable | 1 | default high |

Touch device `lcd_touch` (CST816S); `main/display.cpp` registers LVGL touch when enabled.

## Build and flash

```bash
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle
idf.py build flash monitor
```

`gen-bmgr-config` sets target `esp32c5` and applies `sdkconfig.defaults.board` (QUAD PSRAM).

Board defaults: **primary console UART0**, **secondary USB Serial/JTAG** — flash/monitor over USB without manual download mode; logs on UART0 @ 115200.

## Board files

| File | Purpose |
|------|---------|
| `board_peripherals.yaml` | I2C / SPI / GPIO |
| `board_devices.yaml` | ILI9341 LCD, CST816S, BS8112 |
| `setup_device.c` | LCD / touch / audio custom factories |
| `sdkconfig.defaults.board` | Flash / PSRAM / console |
