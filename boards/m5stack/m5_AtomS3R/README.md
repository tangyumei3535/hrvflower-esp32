# M5Stack AtomS3R (`m5_AtomS3R`)

[中文](./README_CN.md)

SKU C126 · ESP32-S3-PICO-1-N8R8 · 8MB Flash · 8MB Octal PSRAM

0.85" GC9107 128×128, LP5562 I2C backlight, button GPIO41. Compact UI (scrolling top/bottom bars).

Docs: [M5Stack AtomS3R](https://docs.m5stack.com/en/core/AtomS3R)

## Pins

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA | 45 | LP5562 `0x30`, BMI270 `0x68` |
| I2C SCL | 0 | |
| SPI MOSI / SCK / CS | 21 / 15 / 14 | |
| LCD DC / RST | 42 / 48 | gc9a01 driver + GC9107 init |
| Button | 41 | active low |

Backlight: `display_backlight` (LP5562, reg `0x0E`), initialized before `display_lcd`.

## Build and flash

```bash
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R
idf.py build flash monitor
```

Download mode: hold reset ~2 s until the internal green LED turns on, then flash.

## Board files

| File | Purpose |
|------|---------|
| `board_peripherals.yaml` | I2C / SPI |
| `board_devices.yaml` | LP5562 + GC9107 LCD |
| `lp5562_backlight.c` | Backlight custom device |
| `setup_device.c` | GC9107 panel factory, `set_gap(0, 32)` |
| `sdkconfig.defaults.board` | Octal PSRAM, etc. |
