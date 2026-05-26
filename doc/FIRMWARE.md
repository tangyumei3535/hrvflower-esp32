# Firmware status & roadmap

[中文](./FIRMWARE_CN.md) · [日本語](./FIRMWARE_JA.md) · [Italiano](./FIRMWARE_IT.md)

What the **current firmware can and cannot do**, plus **planned features**.  
For end-to-end setup (Bemfa, iPhone Shortcuts), see [INTEGRATION.md](./INTEGRATION.md).

Related: [README.md](../README.md) · [main/Kconfig.projbuild](../main/Kconfig.projbuild)

---

## 1. Future development (roadmap)

### 1.1 On-device Wi-Fi provisioning

- [ ] **Issue**: garbled screen when Wi-Fi is down
- [ ] **Goal**: on-device Wi-Fi setup (SSID/password or SoftAP provisioning)
- [ ] **Expected**: credentials in NVS, auto-reconnect on boot; guide screen while offline
- [ ] **Touches**: `main/wifi_connect.cpp`, LVGL provisioning UI, NVS

### 1.2 Low power + periodic NVS persistence

- [ ] **Goal**: lower standby power; wake about **once per hour**
- [ ] **Behavior**: save latest HRV state to NVS after wake, then sleep or brief MQTT sync
- [ ] **Touches**: ESP-IDF sleep API, RTC timer wake, LCD/MQTT vs sleep

### 1.3 ESP32-initiated requests

- [ ] **Goal**: receiver is not MQTT-push-only — **ESP32 actively** triggers sync or a push
- [ ] **Example flows**:
  - Button / touch: on-board key press requests latest HRV immediately
  - Low-power wake: first active sync after sleep, then decide whether to sleep again
- [ ] **Touches**: `esp_http_client` / MQTT publish, request state machine, `hrv_ui` / NVS / sleep policy; optional UI “requesting…” hint

### 1.4 OTA firmware updates

- [ ] **Goal**: over-the-air update without USB flash
- [ ] **Approach**: `esp_https_ota`; `ota_0` / `ota_1` partitions
- [ ] **Touches**: `partitions.csv`, version reporting, optional MQTT-triggered upgrade URL

### 1.5 IMU auto-rotate display

- [ ] **Goal**: auto-rotate LVGL by orientation (0° / 90° / 180° / 270°)
- [ ] **Hardware**: e.g. AtomS3R on-board BMI270 (I2C `0x68`)
- [ ] **Touches**: `esp_lcd_panel_mirror` / `lv_disp_set_rotation`, `main/display.cpp`

### 1.6 BLE mode (no Wi-Fi)

- [ ] **Goal**: when Wi-Fi is unavailable or not configured, receive HRV JSON over **BLE** and refresh UI
- [ ] **Use case**: no router, nearby sync, or data path before Wi-Fi provisioning
- [ ] **Touches**: NimBLE / GATT service, phone companion or Shortcut BLE write, reuse `hrv_parse_status_json()`, Wi-Fi/MQTT vs BLE state machine

### 1.7 Other optional enhancements

- [ ] Subscribe to both `{topic}` and `{topic}/set`
- [ ] UI: MQTT / last-update status indicator
- [ ] Boot screen: “waiting for first payload”
- [ ] Weather icons (LVGL vector)
- [ ] MQTT over TLS (port 9503)
- [ ] Handle MQTT retain — show last frame after reboot

---

## 2. After flash: what works / what does not

### 2.1 Implemented (valid menuconfig + LCD ready)

| Feature | Notes |
|---------|-------|
| Wi-Fi STA | Auto-reconnect (20 retries) |
| Bemfa MQTT | Default `mqtt://bemfa.com:9501`; UID single-device or AppID multi-device |
| Topic subscribe | Default `hrv001` (`BEMFA_MQTT_TOPIC` configurable) |
| JSON → UI | `drawInterface()` **only** on valid MQTT payload |
| Five-stage flower | HRV thresholds → gray bud / dark red / orange / pink / gold+green |
| Top weather + bottom HRV | temp, city, last update |
| MQTT reconnect | handled by ESP-MQTT |

### 2.2 Not in this repo / not implemented

| Feature | Notes |
|---------|-------|
| iPhone Shortcuts / Health | Not in this repo; see [INTEGRATION.md](./INTEGRATION.md) |
| Bemfa account & topic | User must register |
| Touch / IMU / voice | Not used in this demo; IMU rotate → §1.5 |
| Offline / MQTT status UI | No “waiting for push” indicator |
| Garbled screen when offline | Needs provisioning UI (§1.1) |
| Retain last frame on boot | Not handled |
| MQTT TLS (9503) | Plain 9501 only |
| OTA / provisioning / NVS / ESP32-initiated requests / BLE | Roadmap §1 |

### 2.3 Typical behavior after flash

- **No UID**: MQTT fails; placeholder UI (`--:--`, `HRV: 0ms`).
- **UID set, sender not configured**: serial `MQTT connected, subscribe hrv001`; flower unchanged.
- **Full pipeline**: each successful HTTP POST refreshes the display **once**.

### 2.4 Serial monitor

```bash
idf.py -p <PORT> monitor
```

`<PORT>` e.g. macOS `/dev/cu.usbmodem*`, Linux `/dev/ttyUSB*`.

---

## 3. Source index

| File | Role |
|------|------|
| [main/main.cpp](../main/main.cpp) | Boot: board, Wi-Fi, display, MQTT |
| [main/mqtt_hrv.cpp](../main/mqtt_hrv.cpp) | Subscribe, parse, trigger UI |
| [main/hrv_ui.cpp](../main/hrv_ui.cpp) | UI and flower stages |
| [main/wifi_connect.cpp](../main/wifi_connect.cpp) | Wi-Fi STA |
| [main/display.cpp](../main/display.cpp) | LCD + LVGL |
| [main/Kconfig.projbuild](../main/Kconfig.projbuild) | Wi-Fi / Bemfa Kconfig |

---

## Revision history

| Date | Notes |
|------|-------|
| 2026-05-26 | Split from legacy integration doc |
