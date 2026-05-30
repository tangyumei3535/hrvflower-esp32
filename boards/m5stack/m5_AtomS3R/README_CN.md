# M5Stack AtomS3R（`m5_AtomS3R`）

[English](./README.md)

SKU C126 · ESP32-S3-PICO-1-N8R8 · 8MB Flash · 8MB Octal PSRAM

0.85" GC9107 128×128，LP5562 I2C 背光。两个按键：**屏幕背后 USER_BUT**、**侧面复位键**。

官方资料：[M5Stack AtomS3R](https://docs.m5stack.com/zh_CN/core/AtomS3R)

## 引脚

| 功能 | GPIO | 备注 |
|------|------|------|
| I2C SDA | 45 | LP5562 `0x30`、BMI270 `0x68` |
| I2C SCL | 0 | |
| BMI270 INT1 | **16**（待原理图确认） | RTC GPIO；官方 PinMap **未标 INT**；勿用 **GPIO3**（32K 晶振脚） |
| SPI MOSI / SCK / CS | 21 / 15 / 14 | |
| LCD DC / RST | 42 / 48 | gc9a01 驱动 + GC9107 init |
| USER_BUT | 41 | 低电平有效 — 屏幕背后；短按切换 **系统页**（固件版本 + 电量） |
| 电池底座 BAT ADC | **8** | M5 [Atomic Battery Base](https://docs.m5stack.com/zh_CN/atom/Atomic%20Battery%20Base)（AtomS3R 行）；Atom-Lite 为 GPIO33；电量 % 按底座 LED 四档电压分段（满电约 3780mV ADC） |

背光：`display_backlight`（LP5562，reg `0x0E`），在 `display_lcd` 之前初始化。

## 按键与睡眠

| 按键 | 硬件 | 能否从 deep sleep 唤醒 |
|------|------|------------------------|
| **侧面复位键** | 接 EN / 复位电路 | **能** — 整机复位，固件重新启动 |
| **背后 USER_BUT（GPIO41）** | 屏下按键 | **不能** — ESP32-S3 上非 RTC 脚，无法 EXT0 唤醒 |

开启 **Low power** + **BMI270 any-motion wake**（默认开）：**有效 MQTT 或背后按键** 后重置空闲计时，**90 秒**无操作再 deep sleep；**晃动**（INT → **GPIO16** EXT1）或 **侧面复位** 唤醒。

下载模式：长按 **复位键** 约 2 秒至内部绿灯亮，再烧录。

## 编译烧录

```bash
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R
idf.py build flash monitor
```

## 板级文件

| 文件 | 作用 |
|------|------|
| `board_peripherals.yaml` | I2C / SPI |
| `board_devices.yaml` | LP5562 + GC9107 LCD |
| `lp5562_backlight.c` | 背光 |
| `setup_device.c` | GC9107 面板工厂，`set_gap(0, 32)` |
| `sdkconfig.defaults.board` | Octal PSRAM 等 |
