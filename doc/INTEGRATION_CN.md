# 端到端集成指南

[English](./INTEGRATION.md)

iPhone（建议 Apple Watch）→ **巴法云 HTTP** → **MQTT** → ESP32 显示屏。  
发送端与接收端可不在同一局域网。

固件能力与路线图见 [FIRMWARE_CN.md](./FIRMWARE_CN.md) · 工程概览 [README_CN.md](./README_CN.md)

---

## 1. 数据流

### 1.1 总览

```
iPhone（+ Apple Watch）
  健康 HRV 样本 → 快捷指令拼 JSON → HTTPS POST 巴法云
        ↓
巴法云 MQTT Broker (bemfa.com:9501)
        ↓
ESP32：Wi-Fi → 订阅主题 → 解析 JSON → LVGL 情绪花
```

### 1.2 单条 payload 路径

| 步骤 | 位置 | 内容 |
|------|------|------|
| 1 | Apple Watch | HRV 写入 iPhone「健康」 |
| 2 | 健康 App | 类型：心率变异性 (ms) |
| 3 | 快捷指令 | 最新 1 条 → 整数 `hrv` |
| 4 | 快捷指令 | 日期 → `time`；天气 → `weather`/`temp`；定位 → `city` |
| 5 | 快捷指令 | 组装 JSON（见 §1.3） |
| 6 | 巴法云 HTTP | POST 到主题 |
| 7 | `mqtt_hrv.cpp` | `hrv_parse_status_json()` |
| 8 | `hrv_ui.cpp` | `drawInterface()` |

### 1.3 JSON Schema

```json
{
  "type": "status",
  "hrv": 26,
  "time": "2026/5/25, 20:38",
  "temp": 26,
  "weather": "Mostly Cloudy",
  "city": "Example City"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `hrv` | int | **必填**；决定花朵阶段 |
| `time` | string | 发送时刻，最长 31 字符 |
| `temp` | int | 可选，摄氏度 |
| `weather` | string | 可选 |
| `city` | string | 可选 |

### 1.4 HRV → 花朵

| HRV (ms) | 状态 |
|----------|------|
| &lt; 20 | 有点压力（灰苞） |
| 20–30 | 正常 |
| 31–39 | 有点开心 |
| 40–49 | 开心 |
| ≥ 50 | 超级开心（金红+绿晕） |

阈值见 `main/hrv_ui.cpp` 的 `hrv_to_stage()`。

### 1.5 iPhone 说明

- **无 Apple Watch**：健康内 HRV 样本少，快捷指令常读到空/旧值。
- **有 Apple Watch**：样本频率高，体验完整。
- 首次需在 **设置 → 隐私与安全性 → 健康 → 快捷指令** 允许读取 HRV。

---

## 2. 配置清单（按负责方）

### 2.1 固件 / 硬件（烧录前）

- [ ] `idf.py menuconfig` → **HRV Emotion Flower**：Wi-Fi、BEMFA_PRIVATE_KEY、BEMFA_MQTT_TOPIC
- [ ] 多设备同时在线：选 **AppID + secretKey**（见 [巴法云 userSecretKey API](https://cloud.bemfa.com/docs/src/api_user.html)）
- [ ] `idf.py gen-bmgr-config -c boards/<vendor> -b <board>`
- [ ] `idf.py build flash monitor`
- [ ] 路由器 **2.4GHz**；网络可访问 `bemfa.com:9501`

### 2.2 巴法云

- [ ] 注册 [cloud.bemfa.com](https://cloud.bemfa.com)
- [ ] 创建主题（与 `BEMFA_MQTT_TOPIC` 一致，如 `hrv001`）
- [ ] UID 写入 menuconfig 后重新烧录
- [ ] 阅读 [MQTT 接入](https://cloud.bemfa.com/docs/src/mqtt.html) 与 HTTP POST 参数

### 2.3 iPhone 快捷指令

- [ ] 健康：允许读取「心率变异性」
- [ ] 查找健康样本 → HRV → 最新 1 条 → ms 整数
- [ ] 日期、天气、定位 → 变量
- [ ] 拼 JSON（§1.3）
- [ ] **POST** `https://apis.bemfa.com/va/postJsonMsg`（Dictionary：`uid`、`topic`、`type`=1、`msg`=JSON）
- [ ] 手动运行一次，确认显示屏更新

### 2.4 自动化与 Apple Watch

- [ ] 快捷指令「自动化」：定时（如每 30 分钟）运行
- [ ] Watch 日常佩戴，健康 App 内有 HRV 曲线
- [ ] 无新 HRV 时的策略（跳过 / 重复上次 / 默认值）— 固件不处理空推送

### 2.5 接收端部署

- [ ] 稳定供电；Wi-Fi 与 menuconfig 一致
- [ ] 串口确认 `MQTT connected, subscribe <topic>`

---

## 3. 推荐配置顺序

### 阶段 0：验证屏 + 云（~30 分钟）

1. 完成 §2.1 烧录，`monitor` 确认 Wi-Fi 拿 IP
2. 日志：`MQTT connected, subscribe <topic>`
3. 巴法云控制台 **手动推送** §1.3 JSON
4. 若屏不变：核对主题名；查串口 `Ignored payload`

### 阶段 1：HTTP 单次打通

1. 完成 §2.2
2. curl / Postman 按官方文档 POST 一次 JSON
3. 板子 UI 或串口应变化

### 阶段 2：iPhone 快捷指令

1. 新建快捷指令，按 §1.2 串联动作
2. POST 巴法云（§2.3）
3. 手动运行，确认显示屏更新

### 阶段 3：自动化 + Watch

1. 定时自动化运行快捷指令
2. 确认 Watch + 健康有连续 HRV
3. 核对时区与时间显示

### 阶段 4：长期运行

1. 断电/断网恢复后 MQTT 重连（观察 24h）
2. iOS 后台自动化是否被系统限制
3. 巴法云免费额度是否够用

---

## 4. 快捷指令最小步骤

> HTTP 参数以 [巴法云文档](https://cloud.bemfa.com/docs/src/mqtt.html) 为准。

1. **查找所有健康样本** → 心率变异性 → 最新 1 条
2. **从输入获取数字** → `hrv`
3. **当前日期** → **格式化** → `time`
4. **获取当前天气** → `weather`、`temp`
5. **获取当前位置** → 城市 → `city`
6. **文本**拼 JSON：  
   `{"type":"status","hrv":{hrv},"time":"{time}","temp":{temp},"weather":"{weather}","city":"{city}"}`
7. **Dictionary**（`uid`、`topic`、`type`=1、`msg`=payload）→ **获取 URL 内容** POST `https://apis.bemfa.com/va/postJsonMsg`

---

## 修订记录

| 日期 | 说明 |
|------|------|
| 2026-05-26 | 从 INTEGRATION_TODO 重构为集成指南 |
