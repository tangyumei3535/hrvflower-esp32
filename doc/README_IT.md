# HRV Emotion Flower (hrvflower-esp32)

[English](../README.md) · [中文](./README_CN.md) · [日本語](./README_JA.md)

Firmware ESP-IDF per un display da scrivania a fiore emotivo HRV. Il dispositivo si connette via Wi-Fi, sottoscrive **Bemfa Cloud MQTT** e mostra su LVGL HRV, meteo, città e ora dell’ultimo aggiornamento. Un’Automazione iPhone (es. con Apple Watch) può inviare JSON via HTTP Bemfa—non serve la stessa LAN.

## Documentazione

| Argomento | Link |
|-----------|------|
| Integrazione end-to-end (Bemfa, Scorciatoie iOS) | [INTEGRATION.md](./INTEGRATION.md) |
| Stato firmware e roadmap | [FIRMWARE_IT.md](./FIRMWARE_IT.md) |
| Definizioni board e nuove target | [boards/README.md](./boards/README.md) |

## Avvio rapido

```bash
git clone https://github.com/tangyumei3535/hrvflower-esp32.git
cd hrvflower-esp32
. $HOME/esp/esp-idf/export.sh

# Scegli una board (il cambio cancella sdkconfig e applica i defaults)
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle   # ESP32-C5, 1.83"
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R             # ESP32-S3, 128×128

idf.py menuconfig   # HRV Emotion Flower: Wi-Fi, chiave Bemfa, topic MQTT
idf.py build flash monitor
```

## Board supportate

| Board | Percorso | SoC | UI |
|-------|----------|-----|-----|
| ESP-SensairShuttle | [boards/espressif/esp_SensairShuttle/](../boards/espressif/esp_SensairShuttle/) | ESP32-C5 | Layout completo |
| M5Stack AtomS3R | [boards/m5stack/m5_AtomS3R/](../boards/m5stack/m5_AtomS3R/) | ESP32-S3 | Compatto 128×128 |

## Licenza

[Apache License 2.0](../LICENSE)
