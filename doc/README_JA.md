# HRV エモーションフラワー（hrvflower-esp32）

[English](../README.md) · [中文](./README_CN.md) · [Italiano](./README_IT.md)

ESP-IDF 向け HRV エモーションフラワー卓上ディスプレイのファームウェア。Wi-Fi で **Bemfa Cloud MQTT** に接続し、LVGL 上に HRV・天気・都市・最終更新時刻を表示します。iPhone ショートカット（Apple Watch 連携可）から Bemfa HTTP 経由で JSON を送信でき、送信端末とデバイスは同一 LAN である必要はありません。

## ドキュメント

| 内容 | リンク |
|------|------|
| エンドツーエンド統合（Bemfa、iOS ショートカット） | [INTEGRATION.md](./INTEGRATION.md) |
| ファームウェア現状・ロードマップ | [FIRMWARE_JA.md](./FIRMWARE_JA.md) |
| ボード定義と新規ボード追加 | [boards/README.md](./boards/README.md) |

## クイックスタート

```bash
git clone https://github.com/tangyumei3535/hrvflower-esp32.git
cd hrvflower-esp32
. $HOME/esp/esp-idf/export.sh

# ボードを選択（切替時は sdkconfig をクリアし board defaults を適用）
idf.py gen-bmgr-config -c boards/espressif -b esp_SensairShuttle   # ESP32-C5, 1.83"
idf.py gen-bmgr-config -c boards/m5stack -b m5_AtomS3R             # ESP32-S3, 128×128

idf.py menuconfig   # HRV Emotion Flower: Wi-Fi, Bemfa key, MQTT topic
idf.py build flash monitor
```

## 対応ボード

| ボード | パス | SoC | UI |
|--------|------|-----|-----|
| ESP-SensairShuttle | [boards/espressif/esp_SensairShuttle/](../boards/espressif/esp_SensairShuttle/) | ESP32-C5 | フルレイアウト |
| M5Stack AtomS3R | [boards/m5stack/m5_AtomS3R/](../boards/m5stack/m5_AtomS3R/) | ESP32-S3 | コンパクト 128×128 |

## ライセンス

[Apache License 2.0](../LICENSE)
