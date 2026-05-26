# M5Stack AtomS3R（`m5_AtomS3R`）

[English](./README.md)

SKU C126 · ESP32-S3-PICO-1-N8R8 · 8MB Flash · 8MB Octal PSRAM

0.85" GC9107 128×128，LP5562 I2C 背光，按键 GPIO41。UI 使用紧凑布局（顶/底滚动文字）。

官方资料：[M5Stack AtomS3R](https://docs.m5stack.com/zh_CN/core/AtomS3R)

## 引脚

| 功能 | GPIO | 备注 |
|------|------|------|
| I2C SDA | 45 | LP5562 `0x30`、BMI270 `0x68` |
| I2C SCL | 0 | |
| SPI MOSI / SCK / CS | 21 / 15 / 14 | |
| LCD DC / RST | 42 / 48 | gc9a01 驱动 + GC9107 init |
| 按键 | 41 | 低电平有效 |

背光：`display_backlight`（LP5562，reg `0x0E`），在 `display_lcd` 之前初始化。

## 编译烧录

```bash
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R
idf.py build flash monitor
```

下载模式：长按复位约 2 秒至内部绿灯亮，再烧录。

## 板级文件

| 文件 | 作用 |
|------|------|
| `board_peripherals.yaml` | I2C / SPI |
| `board_devices.yaml` | LP5562 + GC9107 LCD |
| `lp5562_backlight.c` | 背光 custom device |
| `setup_device.c` | GC9107 panel factory，`set_gap(0, 32)` |
| `sdkconfig.defaults.board` | Octal PSRAM 等 |
