# HRV 情绪花（hrvflower-esp32）

[English](../README.md) · [日本語](./README_JA.md) · [Italiano](./README_IT.md)

ESP-IDF 固件：HRV 情绪花桌面摆件。设备 Wi-Fi 连网、订阅 **巴法云 MQTT**，在 LVGL 上显示 HRV、天气、城市与更新时间。iPhone 快捷指令（可配合 Apple Watch）经巴法云 HTTP 推送 JSON，发送端与设备无需同一局域网。

## 文档

| 内容 | 链接 |
|------|------|
| 端到端集成（巴法云、快捷指令、配置顺序） | [INTEGRATION_CN.md](./INTEGRATION_CN.md) |
| 固件现状与路线图 | [FIRMWARE_CN.md](./FIRMWARE_CN.md) |
| 板级定义 | [boards/README_CN.md](./boards/README_CN.md) |

## 快速开始

```bash
git clone https://github.com/tangyumei3535/hrvflower-esp32.git
cd hrvflower-esp32
. $HOME/esp/esp-idf/export.sh

# 选板（换板会清除 sdkconfig 并应用板级 defaults）
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle   # ESP32-C5，1.83"
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R             # ESP32-S3，128×128

idf.py menuconfig   # HRV Emotion Flower：Wi-Fi、BEMFA_PRIVATE_KEY、BEMFA_MQTT_TOPIC
idf.py build flash monitor
```

## 支持板型

| 板型 | 路径 | 芯片 | UI |
|------|------|------|-----|
| ESP-SensairShuttle | [boards/espressif/esp_SensairShuttle/](../boards/espressif/esp_SensairShuttle/) | ESP32-C5 | 大屏标准布局 |
| M5Stack AtomS3R | [boards/m5stack/m5_AtomS3R/](../boards/m5stack/m5_AtomS3R/) | ESP32-S3 | 128×128 紧凑布局 |

## License

[Apache License 2.0](../LICENSE)
