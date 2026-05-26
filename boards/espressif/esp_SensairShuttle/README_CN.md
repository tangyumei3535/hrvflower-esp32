# ESP-SensairShuttle（`esp_SensairShuttle`）

[English](./README.md)

ESP32-C5-WROOM-1-N16R8 · 16MB Flash · 8MB PSRAM

1.83" ILI9341（逻辑 284×240，swap_xy），CST816S 触摸（I2C `0x15`）。UI 使用标准大屏布局。

官方资料：[ESP-SensairShuttle v1.0 用户指南](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32c5/esp-sensairshuttle/user_guide_v1.0.html)

## 引脚

| 功能 | GPIO | 备注 |
|------|------|------|
| I2C SDA / SCL | 2 / 3 | CST816S `0x15`、BS8112 触摸按键 |
| SPI MOSI / SCK / CS | 23 / 24 / 25 | |
| LCD DC | 26 | ili9341 驱动 + 自定义 init |
| PA 使能 | 1 | 默认高 |

触摸：`lcd_touch`（CST816S）；`main/display.cpp` 在启用时自动挂载 LVGL touch。

## 编译烧录

```bash
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle
idf.py build flash monitor
```

`gen-bmgr-config` 会设置 `esp32c5` target 并应用板级 `sdkconfig.defaults.board`（含 QUAD PSRAM）。

板级默认 **Primary console = UART0**、**Secondary = USB Serial/JTAG**，可直接 USB 烧录/监视，无需手动进下载模式；日志走 UART0（115200）。

## 板级文件

| 文件 | 作用 |
|------|------|
| `board_peripherals.yaml` | I2C / SPI / GPIO |
| `board_devices.yaml` | ILI9341 LCD、CST816S、BS8112 |
| `setup_device.c` | LCD / touch / audio custom factory |
| `sdkconfig.defaults.board` | Flash / PSRAM / Console |
