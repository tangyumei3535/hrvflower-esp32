# HRV Emotion Flower (hrvflower-esp32)

[中文](./doc/README_CN.md) · [日本語](./doc/README_JA.md) · [Italiano](./doc/README_IT.md)

ESP-IDF firmware for an HRV emotion flower desk display. The device connects via Wi-Fi, subscribes to **Bemfa Cloud MQTT**, and shows HRV, weather, city, and last-update time on LVGL. An iPhone Shortcut (e.g. with Apple Watch) can push JSON over Bemfa HTTP—no shared LAN required.

## Docs

| Topic | Link |
|-------|------|
| Integration (Bemfa, iOS Shortcuts, setup order) | [doc/INTEGRATION.md](./doc/INTEGRATION.md) |
| Firmware status & roadmap | [doc/FIRMWARE.md](./doc/FIRMWARE.md) |
| Board definitions | [doc/boards/README.md](./doc/boards/README.md) |

## Quick start

```bash
git clone https://github.com/tangyumei3535/hrvflower-esp32.git
cd hrvflower-esp32
. $HOME/esp/esp-idf/export.sh

# Pick one board (switching clears sdkconfig and applies board defaults)
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle   # ESP32-C5, 1.83"
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R             # ESP32-S3, 128×128

idf.py menuconfig   # HRV Emotion Flower: Wi-Fi, Bemfa key, MQTT topic
idf.py build flash monitor
```

## Supported boards

| Board | Path | SoC | UI |
|-------|------|-----|-----|
| ESP-SensairShuttle | [boards/espressif/esp_SensairShuttle/](./boards/espressif/esp_SensairShuttle/) | ESP32-C5 | Full layout |
| M5Stack AtomS3R | [boards/m5stack/m5_AtomS3R/](./boards/m5stack/m5_AtomS3R/) | ESP32-S3 | Compact 128×128 |

## License

[Apache License 2.0](./LICENSE)
