# M5Stack AtomS3R (`m5_AtomS3R`)

[中文](./README_CN.md)

SKU C126 · ESP32-S3-PICO-1-N8R8 · 8MB Flash · 8MB Octal PSRAM

0.85" GC9107 128×128, LP5562 I2C backlight. Two buttons: **USER_BUT** under the display, **reset** on the side.

Docs: [M5Stack AtomS3R](https://docs.m5stack.com/en/core/AtomS3R)

## Pins

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA | 45 | LP5562 `0x30`, BMI270 `0x68` |
| I2C SCL | 0 | |
| BMI270 INT1 | **16** (verify schematic) | RTC GPIO; M5 PinMap shows I2C only — do **not** use GPIO3 (32K) |
| SPI MOSI / SCK / CS | 21 / 15 / 14 | |
| LCD DC / RST | 42 / 48 | gc9a01 driver + GC9107 init |
| USER_BUT | 41 | Active low — under the round display |

Backlight: `display_backlight` (LP5562, reg `0x0E`), initialized before `display_lcd`.

## Buttons and sleep

| Control | Hardware | Deep sleep wake |
|---------|----------|-----------------|
| **Side reset** | EN / reset circuit | **Yes** — full chip reset, reboots firmware |
| **USER_BUT (GPIO41)** | Under display | **No** — not an RTC GPIO on ESP32-S3 (cannot EXT0/EXT1) |

With **Low power** + **BMI270 any-motion wake** (default on): run **30 s**, then deep sleep. Wake on **shake** (INT → **GPIO16** EXT1) or **side reset**. Logs `INT GPIOx level=…` before sleep; do not use GPIO3 (32K).

Download mode: hold **reset** ~2 s until the internal green LED lights up, then flash.

## Build and flash

```bash
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R
idf.py build flash monitor
```

## Board files

| File | Purpose |
|------|---------|
| `board_peripherals.yaml` | I2C / SPI |
| `board_devices.yaml` | LP5562 + GC9107 LCD |
| `lp5562_backlight.c` | Backlight custom device |
| `setup_device.c` | GC9107 panel factory, `set_gap(0, 32)` |
| `sdkconfig.defaults.board` | Octal PSRAM, etc. |
