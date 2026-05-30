# 固件现状与路线图

[English](./FIRMWARE.md) · [日本語](./FIRMWARE_JA.md) · [Italiano](./FIRMWARE_IT.md)

本文档说明 **当前固件能做什么、不能做什么**，以及 **后续开发计划**。  
端到端集成（巴法云、iPhone 快捷指令）见 [INTEGRATION_CN.md](./INTEGRATION_CN.md)。

相关：[README_CN.md](./README_CN.md) · [main/Kconfig.projbuild](../main/Kconfig.projbuild)

---

## 1. 未来开发 TODO（固件路线图）

### 1.1 Wi-Fi 配网界面 ✅

- [x] **问题**：未联网时屏幕花屏，用户体验差
- [x] **目标**：on-device Wi-Fi 设置（SSID/密码或 SoftAP 配网）
- [x] **预期**：凭证写入 NVS，重启自动连接；未联网时显示引导页
- [x] **涉及**：`main/wifi_connect.cpp`、`components/esp-wifi-connect/`、LVGL 配网 UI（`hrv_ui_provisioning_*`）、NVS `wifi` 命名空间

**已实现（2026-05-27）**：NVS 凭据优先 STA（60s 超时）→ 失败则 `HRVFlower-XXXX` SoftAP + `http://192.168.4.1` 网页配网；屏显热点名与呼吸动画；menuconfig SSID 仅在 NVS 为空时一次性写入。

### 1.2 低功耗 + 定时 NVS 持久化

- [ ] **目标**：降低待机功耗，约 **每小时唤醒一次**
- [ ] **行为**：唤醒后将最新 HRV 状态写入 NVS，再休眠或短暂连网
- [ ] **涉及**：ESP-IDF sleep API、RTC 定时唤醒、LCD/MQTT 与休眠衔接

### 1.3 ESP32 端主动请求

- [ ] **目标**：接收端不再仅被动等 MQTT 推送，由 **ESP32 主动发起** 数据同步或推送触发
- [ ] **场景示例**：
  - 按键 / 触摸：用户按板载键后立即请求最新 HRV
  - 低功耗唤醒：从 sleep 醒来后的首轮主动同步，再决定是否休眠
- [ ] **涉及**：`esp_http_client` / MQTT publish、请求-响应状态机、与 `hrv_ui` / NVS / 低功耗策略配合；可选 UI「正在请求…」提示

### 1.4 OTA 固件升级

- [ ] **目标**：Over-The-Air 更新，免 USB 烧录
- [ ] **方案**：`esp_https_ota`；分区表预留 `ota_0` / `ota_1`
- [ ] **涉及**：`partitions.csv`、版本号、可选 MQTT 触发升级 URL

### 1.5 IMU 自动旋转屏幕

- [ ] **目标**：根据姿态自动旋转 LVGL 显示（0° / 90° / 180° / 270°）
- [ ] **硬件**：如 AtomS3R 板载 BMI270（I2C `0x68`）
- [ ] **涉及**：`esp_lcd_panel_mirror` / `lv_disp_set_rotation`、`main/display.cpp`

### 1.6 无 Wi-Fi 时的 BLE 模式

- [ ] **目标**：Wi-Fi 不可用或未配置时，通过 **BLE** 接收 HRV JSON 并刷新 UI
- [ ] **场景**：无路由器、临时近场同步，或 Wi-Fi 配网前的数据通路
- [ ] **涉及**：NimBLE / GATT 服务、手机端 Companion 或快捷指令 BLE 写入、复用 `hrv_parse_status_json()`、与 Wi-Fi/MQTT 状态机切换

### 1.7 其它可选增强

- [ ] 同时订阅 `{topic}` 与 `{topic}/set`
- [ ] UI：MQTT / 上次更新时间状态指示
- [ ] 启动页：「等待首次数据」
- [ ] 天气图标（LVGL 矢量）
- [ ] MQTT over TLS（9503）
- [ ] 处理 MQTT Retain，重启后显示最后一帧

---

## 2. 现在直接烧录：已有 / 没有的功能

### 2.1 已有（menuconfig 正确 + LCD 就绪）

| 能力 | 说明 |
|------|------|
| STA 连接 Wi-Fi | NVS 凭据 + 开机 captive portal；运行中断线由 `esp-wifi-connect` 后台重连 |
| 巴法云 MQTT | 默认 `mqtt://bemfa.com:9501`；单设备 UID 鉴权或 AppID 多设备 |
| 订阅主题 | 默认 `hrv001`（`BEMFA_MQTT_TOPIC` 可改） |
| JSON 解析与 UI | 有效 MQTT payload 写入 NVS（`hrv`/`status_json`）并刷新；断电/深睡唤醒/联网后自动显示缓存（首次无缓存为占位） |
| 五阶段情绪花 | HRV 阈值 → 灰苞 / 暗红 / 橙 / 粉 / 金红+绿晕 |
| 顶栏天气 + 底栏 HRV | 含温度、城市、更新时间 |
| MQTT 断线重连 | ESP-MQTT 内部处理 |

### 2.2 没有 / 未实现

| 能力 | 说明 |
|------|------|
| iPhone 快捷指令 / 健康读取 | 不在本仓库，见 [INTEGRATION_CN.md](./INTEGRATION_CN.md) |
| 巴法云账号与主题 | 需自行注册 |
| 触摸 / IMU / 语音 | 本示例未使用；IMU 旋转见 §1.5 |
| 离线 / MQTT 状态 UI | 看不出是否在等待推送 |
| 启动后 MQTT Retain | 未用；改用 **NVS 最后一帧 JSON** |
| MQTT TLS（9503） | 当前仅明文 9501 |
| OTA / ESP32 主动请求 / BLE | 见 §1 路线图 |

### 2.3 烧录后典型现象

- **UID 未填**：MQTT 启动失败，屏保持占位（`--:--`、`HRV: 0ms`）。
- **UID 已填、发送端未配**：串口 `MQTT connected, subscribe hrv001`，花不变。
- **全流程打通**：每次 HTTP POST 成功，屏刷新并写入 NVS；下次上电先显示 NVS 缓存，新推送再覆盖。

### 2.4 串口监视

```bash
idf.py -p <PORT> monitor
```

`<PORT>` 如 macOS `/dev/cu.usbmodem*`，Linux `/dev/ttyUSB*`。

---

## 3. 源码索引

| 文件 | 职责 |
|------|------|
| [main/main.cpp](../main/main.cpp) | 启动：板级、Wi-Fi、显示、MQTT |
| [main/mqtt_hrv.cpp](../main/mqtt_hrv.cpp) | 订阅、解析、触发 UI |
| [main/hrv_ui.cpp](../main/hrv_ui.cpp) | UI 与五阶段花 |
| [main/wifi_connect.cpp](../main/wifi_connect.cpp) | Wi-Fi：NVS STA、SoftAP 配网、配网屏 UI |
| [main/display.cpp](../main/display.cpp) | LCD + LVGL |
| [main/Kconfig.projbuild](../main/Kconfig.projbuild) | Wi-Fi / 巴法云配置 |

---

## 修订记录

| 日期 | 说明 |
|------|------|
| 2026-05-27 | §1.1 Wi-Fi 配网（captive portal + LVGL 引导）已实现 |
| 2026-05-26 | 从 INTEGRATION_TODO 拆出固件现状与路线图 |
