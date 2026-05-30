---
name: hrvflower-dev
description: >-
  hrvflower-esp32 ESP-IDF: boards, Bemfa MQTT, LVGL, Wi-Fi provisioning, AtomS3R deep sleep
  + BMI270 EXT1 wake, git/Cursor workflow. Use for hrvflower, Bemfa, low power, board builds.
---

# hrvflower-esp32

## Always apply

| Topic | Rule |
|-------|------|
| Replies | **Simplified Chinese** unless user uses another language |
| Code | Logs/comments **English only** |
| Commits/push | **Only when user asks** — **pre-commit cleanup** then commit; see [git-workflow.md](./git-workflow.md) |
| Author footer | `Author: tangyumei <tangyumei@espressif.com>` |
| Branches | `feat/<topic>` → PR `main` |
| Diffs | Minimal; SPDX header per [file-header.md](./file-header.md) on touched `main/` / `boards/**` firmware |

Workspace: `~/Workspace/.cursor/rules/git-commits.mdc`

## Product & docs

ESP32 mood flower: Shortcut → Bemfa → **MQTT** → LVGL + NVS cache.

| Doc | Path |
|-----|------|
| Integration | `doc/INTEGRATION_CN.md` |
| Roadmap | `doc/FIRMWARE_CN.md` |
| AtomS3R pins / low power | `boards/m5stack/m5_AtomS3R/README.md` |

## Layout & build

```
main/              # sources; headers in main/include/
boards/            # esp_board_manager YAML + setup_device.c
components/esp-wifi-connect/   # vendored; not registry
```

| Board | SoC |
|-------|-----|
| `esp_SensairShuttle` | C5 |
| `m5_AtomS3R` | S3, 128×128 compact UI |

```bash
./scripts/build_board.sh m5_AtomS3R boards/m5stack
idf.py -p /dev/tty.usbmodem* flash monitor
```

`sdkconfig.defaults`: `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096`. Per-board: `boards/*/sdkconfig.defaults.board`.

## Wi-Fi provisioning

`wifi_connect.cpp` + captive portal: NVS STA → 60s timeout → SoftAP `HRVFlower-XXXX` → `192.168.4.1`.

**LVGL only from `wifi_ui` task** (queue + `display_lock()`). Never call LVGL from Wi-Fi event handlers.

## MQTT / UI

- `mqtt_hrv.cpp` — Bemfa; `hrv_ui_apply_payload()` + `hrv_status_store.cpp` (NVS `hrv` / `status_json`, 512 B max).
- Boot: `hrv_status_store_init()` → `hrv_ui_init()` **before** `wifi_connect_init()`; do not redraw on `Connected` (white flash).

## Low power (AtomS3R)

Kconfig: `HRV_LOW_POWER_ENABLE`, `HRV_ACTIVE_WINDOW_SEC` (default **90**), `HRV_IMU_WAKE_ENABLE`, `HRV_IMU_INT_GPIO` (**16**, not GPIO3 — 32K crystal).

### Boot → sleep sequence (`main/main.cpp`)

1. `hrv_imu_wake_on_boot()` · `hrv_power_log_wakeup_reason()`
2. `esp_board_manager_init()` → **`hrv_imu_wake_prepare()`** (BMI270 any-motion on **hardware `I2C_NUM_0`**)
3. display / Wi-Fi / MQTT start
4. **`mqtt_hrv_active_window(sec×1000)`** — idle after **last valid** payload
5. `hrv_power_display_off()` → `hrv_imu_wake_arm_for_deep_sleep()` → `esp_deep_sleep_start()`

### Idle before sleep (`mqtt_hrv.cpp`)

- Each successful `hrv_ui_apply_payload()` or USER button click calls `mqtt_hrv_kick_idle()`.
- Window entry also seeds time → **no MQTT for 90s still sleeps** (from window start).
- **Wrong**: EventGroup / wait that returns on **first** message → sleeps ~seconds after Bemfa push.

### Sleep wake config (`hrv_imu_wake.c`)

Software wake **only** IMU:

```c
esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_HRV_IMU_INT_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH);
gpio_hold_en(IMU_INT_GPIO);
```

- **No RTC timer wake.** Side **EN** reset still boots (`ESP_SLEEP_WAKEUP_UNDEFINED`).
- **Wrong**: `esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER)` when timer never enabled → `Incorrect wakeup source (4)` (`4` = `ESP_SLEEP_WAKEUP_TIMER`).
- **Wrong**: disable EXT0/GPIO/touch/ULP individually without prior enable.

### IMU pitfalls

| Issue | Cause | Fix |
|-------|--------|-----|
| `IMU wake not armed` / I2C NACK at sleep | SW I2C or late init after board owns bus | `hrv_imu_wake_prepare()` **right after** board init, HW I2C |
| No deep sleep at all | `CONFIG_HRV_LOW_POWER_ENABLE` off in sdkconfig | board `sdkconfig.defaults.board` + rebuild |
| Sleep on first MQTT | `wait_activity` exits on message bit | use **idle-after-last** `mqtt_hrv_active_window` |

Dep: `espressif/bmi270_sensor` in `main/idf_component.yml`.

## Pitfalls (general)

| Symptom | Fix |
|---------|-----|
| `sys_evt` stack overflow | LVGL off Wi-Fi callbacks → `wifi_ui` queue |
| `cursoragent` on GitHub | `commit-tree`, Attribution off |
| Missing board headers | `idf.py gen-bmgr-config` / `build_board.sh` |

## Roadmap (don’t mark ✅ unless done)

| § | Item |
|---|------|
| 1.1 | Wi-Fi captive portal | ✅ |
| 1.2 | Deep sleep + NVS + IMU EXT1 | ✅ on AtomS3R; light sleep GPIO41 / Wi-Fi-in-sleep open |
| 1.3–1.6 | Active request, OTA, IMU rotate, BLE | open |

## Related

- [git-workflow.md](./git-workflow.md) — commits, `gh`, Attribution
- [SKILL_CN.md](./SKILL_CN.md) — 中文速查
- [file-header.md](./file-header.md) — SPDX block
