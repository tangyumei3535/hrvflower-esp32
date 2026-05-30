---
name: hrvflower-dev-cn
description: >-
  hrvflower-esp32 中文速查：低功耗、MQTT 空闲、IMU 唤醒、构建与 Git。与 SKILL.md 配套。
---

# hrvflower-esp32 中文速查

英文完整版：[SKILL.md](./SKILL.md)

## 协作（必守）

- 回复：**简体中文**；源码日志/注释：**仅英文**
- **仅用户明确要求**时 commit / push；`commit-tree`，禁止 `Co-authored-by: Cursor`
- 分支 `feat/<topic>`；提交 `feat(scope): …` + 末行 `Author: tangyumei <tangyumei@espressif.com>`
- 改动最小；新改固件用 [file-header.md](./file-header.md) SPDX 块

## 低功耗（AtomS3R，实战经验）

### 流程

上电 → board init 后立刻 **`hrv_imu_wake_prepare()`**（硬件 I2C）→ Wi-Fi/MQTT → **`mqtt_hrv_active_window(90s)`** → 关背光 → **仅 EXT1 GPIO16** → deep sleep。

### 入睡前空闲

- **最后一次有效 MQTT 后再空闲 90s** 才睡；每条有效 JSON **重置**计时。
- 若一直无 MQTT：从进入 `active_window` 起算满 90s 也会睡。
- **错误**：收到第一条 Bemfa 消息就立刻睡（旧 `wait_activity` + EventGroup 置位即返回）。

### 唤醒源

| 项 | 说明 |
|----|------|
| 软件配置 | 仅 **EXT1 + GPIO16**（BMI270 INT1 高电平） |
| 不用 | RTC 定时唤醒 |
| 硬件 | 侧面 **EN 复位** 仍可开机（非 `esp_sleep` 配置项） |
| 禁 GPIO3 | ESP32-S3-PICO 的 32K 晶振脚 |

入睡前：`ESP_SLEEP_WAKEUP_ALL` 清空 → 只 `enable_ext1_wakeup(GPIO16)`。

**勿**对未 enable 的源调用 `disable`：日志 `Incorrect wakeup source (4)` 中 **4 = TIMER**。

### IMU

- 入睡前再用软件 I2C 配 BMI270 → **NACK / IMU wake not armed**
- 修复：board init 后尽早 `hrv_imu_wake_prepare()`，走 **`I2C_NUM_0`**

Kconfig：`HRV_LOW_POWER_ENABLE`、`HRV_ACTIVE_WINDOW_SEC=90`、`HRV_IMU_INT_GPIO=16`。

## 其它要点

| 主题 | 说明 |
|------|------|
| 构建 | `./scripts/build_board.sh m5_AtomS3R boards/m5stack` |
| 配网 | STA 60s → SoftAP；LVGL 只在 **wifi_ui 任务** |
| NVS UI | `hrv` / `status_json`，复位后先显示缓存 |
| 组件 | `components/esp-wifi-connect/` 本地 vendored |

## 文档

`doc/INTEGRATION_CN.md` · `doc/FIRMWARE_CN.md`（§1.2 低功耗已标 ✅）

Git / Cursor：[git-workflow.md](./git-workflow.md)
