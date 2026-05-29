# ファームウェア現状とロードマップ

[English](./FIRMWARE.md) · [中文](./FIRMWARE_CN.md) · [Italiano](./FIRMWARE_IT.md)

**現在のファームウェアのできること・できないこと**と**今後の開発計画**を説明します。  
エンドツーエンド統合（Bemfa、iPhone ショートカット）は [INTEGRATION.md](./INTEGRATION.md) を参照。

関連：[README_JA.md](./README_JA.md) · [main/Kconfig.projbuild](../main/Kconfig.projbuild)

---

## 1. 今後の開発 TODO（ロードマップ）

### 1.1 Wi-Fi プロビジョニング UI ✅

- [x] **課題**：未接続時に画面が乱れる
- [x] **目標**：オンデバイス Wi-Fi 設定（SSID/パスワードまたは SoftAP）
- [x] **期待**：認証情報を NVS に保存、再起動後自動接続；未接続時はガイド画面
- [x] **関連**：`main/wifi_connect.cpp`、`components/esp-wifi-connect/`、LVGL UI（`hrv_ui_provisioning_*`）、NVS `wifi`

**実装済み（2026-05-27）**：NVS STA 優先（60s タイムアウト）→ 失敗時 `HRVFlower-XXXX` SoftAP + `http://192.168.4.1`；画面にホットスポット名と呼吸アニメ。

### 1.2 低消費電力 + 定期 NVS 永続化

- [ ] **目標**：待機電力を低減、約 **1 時間ごとに wake**
- [ ] **動作**：wake 後に最新 HRV 状態を NVS に保存し、sleep または短時間 MQTT 同期
- [ ] **関連**：ESP-IDF sleep API、RTC タイマー wake、LCD/MQTT と sleep の連携

### 1.3 ESP32 側からの能動リクエスト

- [ ] **目標**：MQTT 受動待ちだけでなく、**ESP32 が能動的に**同期またはプッシュをトリガー
- [ ] **例**：
  - ボタン / タッチ：オンボードキーで最新 HRV を即時リクエスト
  - 低消費電力 wake：sleep 復帰後の初回能動同期、その後 sleep 可否を判断
- [ ] **関連**：`esp_http_client` / MQTT publish、リクエスト状態機械、`hrv_ui` / NVS / 低消費電力方針；任意 UI「リクエスト中…」

### 1.4 OTA ファームウェア更新

- [ ] **目標**：USB なしの Over-The-Air 更新
- [ ] **方式**：`esp_https_ota`；`ota_0` / `ota_1` パーティション
- [ ] **関連**：`partitions.csv`、バージョン報告、任意 MQTT トリガー URL

### 1.5 IMU 自動画面回転

- [ ] **目標**：姿勢に応じ LVGL を回転（0° / 90° / 180° / 270°）
- [ ] **ハードウェア**：例 AtomS3R 搭載 BMI270（I2C `0x68`）
- [ ] **関連**：`esp_lcd_panel_mirror` / `lv_disp_set_rotation`、`main/display.cpp`

### 1.6 Wi-Fi なし時の BLE モード

- [ ] **目標**：Wi-Fi 不可または未設定時、**BLE** で HRV JSON を受信して UI 更新
- [ ] **用途**：ルーターなし、近距離同期、Wi-Fi プロビジョニング前のデータ経路
- [ ] **関連**：NimBLE / GATT、Companion またはショートカット BLE 書き込み、`hrv_parse_status_json()` 再利用、Wi-Fi/MQTT と BLE の切替

### 1.7 その他の任意強化

- [ ] `{topic}` と `{topic}/set` の両方を subscribe
- [ ] UI：MQTT / 最終更新時刻の状態表示
- [ ] 起動画面：「初回データ待ち」
- [ ] 天気アイコン（LVGL ベクター）
- [ ] MQTT over TLS（9503）
- [ ] MQTT Retain 処理、再起動後に最終フレーム表示

---

## 2. 書き込み直後：ある機能 / ない機能

### 2.1 実装済み（menuconfig 正 + LCD 準備完了）

| 機能 | 説明 |
|------|------|
| Wi-Fi STA | NVS 認証 + 起動時 captive portal；実行中切断はバックグラウンド再接続 |
| Bemfa MQTT | 既定 `mqtt://bemfa.com:9501`；UID 単体または AppID 複数端末 |
| トピック subscribe | 既定 `hrv001`（`BEMFA_MQTT_TOPIC` 変更可） |
| JSON → UI | 有効 MQTT payload 受信時**のみ** `drawInterface()` |
| 5 段階エモーションフラワー | HRV 閾値 → 灰つぼみ / 暗赤 / 橙 / 桃 / 金赤+緑晕 |
| 上部天気 + 下部 HRV | 気温、都市、更新時刻 |
| MQTT 再接続 | ESP-MQTT が処理 |

### 2.2 未実装 / 本リポジトリ外

| 機能 | 説明 |
|------|------|
| iPhone ショートカット / 健康 | 本リポジトリ外 → [INTEGRATION.md](./INTEGRATION.md) |
| Bemfa アカウントとトピック | ユーザー登録が必要 |
| タッチ / IMU / 音声 | 本デモ未使用；IMU 回転 → §1.5 |
| オフライン / MQTT 状態 UI | プッシュ待ちが分からない |
| 起動後 Retain 最終フレーム | 未対応 |
| MQTT TLS（9503） | 現在は平文 9501 のみ |
| OTA / ESP32 能動リクエスト / BLE | ロードマップ §1 |

### 2.3 典型的な挙動

- **UID 未設定**：MQTT 失敗、プレースホルダ UI（`--:--`、`HRV: 0ms`）。
- **UID 設定済み、送信側未設定**：シリアル `MQTT connected, subscribe hrv001`、花は変化なし。
- **全経路接続**：HTTP POST 成功の**その瞬間**に画面更新。

### 2.4 シリアルモニタ

```bash
idf.py -p <PORT> monitor
```

`<PORT>` 例：macOS `/dev/cu.usbmodem*`、Linux `/dev/ttyUSB*`。

---

## 3. ソース索引

| ファイル | 役割 |
|----------|------|
| [main/main.cpp](../main/main.cpp) | 起動：ボード、Wi-Fi、表示、MQTT |
| [main/mqtt_hrv.cpp](../main/mqtt_hrv.cpp) | subscribe、解析、UI トリガー |
| [main/hrv_ui.cpp](../main/hrv_ui.cpp) | UI と 5 段階フラワー |
| [main/wifi_connect.cpp](../main/wifi_connect.cpp) | Wi-Fi：NVS STA、SoftAP プロビジョニング、ガイド画面 |
| [main/display.cpp](../main/display.cpp) | LCD + LVGL |
| [main/Kconfig.projbuild](../main/Kconfig.projbuild) | Wi-Fi / Bemfa 設定 |

---

## 改訂履歴

| 日付 | 内容 |
|------|------|
| 2026-05-27 | §1.1 Wi-Fi プロビジョニング実装 |
| 2026-05-26 | 旧統合 TODO から分離 |
