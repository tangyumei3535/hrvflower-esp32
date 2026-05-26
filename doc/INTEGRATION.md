# End-to-end integration guide

[中文](./INTEGRATION_CN.md)

iPhone (Apple Watch recommended) → **Bemfa HTTP** → **MQTT** → ESP32 display.  
Sender and receiver do not need the same LAN.

Firmware capabilities: [FIRMWARE.md](./FIRMWARE.md) · Project overview: [README.md](../README.md)

---

## 1. Data flow

### 1.1 Overview

```
iPhone (+ Apple Watch)
  Health HRV sample → Shortcut builds JSON → HTTPS POST to Bemfa
        ↓
Bemfa MQTT broker (bemfa.com:9501)
        ↓
ESP32: Wi-Fi → subscribe topic → parse JSON → LVGL mood flower
```

### 1.2 Single payload path

| Step | Where | Content |
|------|-------|---------|
| 1 | Apple Watch | HRV written to iPhone Health |
| 2 | Health app | Type: heart rate variability (ms) |
| 3 | Shortcut | Latest sample → int `hrv` |
| 4 | Shortcut | Date → `time`; weather → `weather`/`temp`; location → `city` |
| 5 | Shortcut | Build JSON (§1.3) |
| 6 | Bemfa HTTP | POST to topic |
| 7 | `mqtt_hrv.cpp` | `hrv_parse_status_json()` |
| 8 | `hrv_ui.cpp` | `drawInterface()` |

### 1.3 JSON schema

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

| Field | Type | Notes |
|-------|------|-------|
| `hrv` | int | **Required**; drives flower stage |
| `time` | string | Send time, max 31 chars |
| `temp` | int | Optional, °C integer |
| `weather` | string | Optional |
| `city` | string | Optional |

### 1.4 HRV → flower stage

| HRV (ms) | Mood |
|----------|------|
| &lt; 20 | Stressed (gray bud) |
| 20–30 | Normal |
| 31–39 | Mildly happy |
| 40–49 | Happy |
| ≥ 50 | Very happy (gold + green glow) |

Thresholds in `main/hrv_ui.cpp` → `hrv_to_stage()`.

### 1.5 iPhone notes

- **No Apple Watch**: few HRV samples in Health; Shortcut may read empty/stale values.
- **With Apple Watch**: higher sample rate; much better experience.
- First run: allow HRV access under **Settings → Privacy & Security → Health → Shortcuts**.

---

## 2. Setup checklists (by role)

### 2.1 Firmware / hardware (before flash)

- [ ] `idf.py menuconfig` → **HRV Emotion Flower**: Wi-Fi, BEMFA_PRIVATE_KEY, BEMFA_MQTT_TOPIC
- [ ] Multiple boards online: enable **AppID + secretKey** ([Bemfa userSecretKey API](https://cloud.bemfa.com/docs/src/api_user.html))
- [ ] `idf.py gen-bmgr-config -c boards/<vendor> -b <board>`
- [ ] `idf.py build flash monitor`
- [ ] **2.4 GHz** Wi-Fi; network reaches `bemfa.com:9501`

### 2.2 Bemfa Cloud

- [ ] Register at [cloud.bemfa.com](https://cloud.bemfa.com)
- [ ] Create topic (matches `BEMFA_MQTT_TOPIC`, e.g. `hrv001`)
- [ ] Flash again after UID is set in menuconfig
- [ ] Read [MQTT docs](https://cloud.bemfa.com/docs/src/mqtt.html) and HTTP POST fields

### 2.3 iPhone Shortcut

- [ ] Health: allow heart rate variability
- [ ] Find Health Samples → HRV → latest → ms integer
- [ ] Date, weather, location → variables
- [ ] Build JSON (§1.3)
- [ ] **POST** `https://apis.bemfa.com/va/postJsonMsg` (Dictionary: `uid`, `topic`, `type`=1, `msg`=JSON)
- [ ] Run once manually; confirm display updates

### 2.4 Automation & Apple Watch

- [ ] Shortcut automation on a schedule (e.g. every 30 min)
- [ ] Wear Watch daily; confirm HRV curve in Health
- [ ] Policy when no new HRV (skip / repeat / default) — firmware does not handle empty pushes

### 2.5 Receiver deployment

- [ ] Stable power; Wi-Fi matches menuconfig
- [ ] Serial log: `MQTT connected, subscribe <topic>`

---

## 3. Recommended setup order

### Phase 0: Display + cloud (~30 min)

1. Flash per §2.1; `monitor` until Wi-Fi has IP
2. Log: `MQTT connected, subscribe <topic>`
3. **Manually push** §1.3 JSON from Bemfa console
4. If UI unchanged: check topic name; serial for `Ignored payload`

### Phase 1: One HTTP POST

1. Complete §2.2
2. POST JSON once via curl / Postman per official docs
3. Board UI or serial should react

### Phase 2: iPhone Shortcut

1. New shortcut; chain actions per §1.2
2. POST to Bemfa (§2.3)
3. Run manually; confirm display update

### Phase 3: Automation + Watch

1. Scheduled automation runs shortcut
2. Confirm Watch + Health have continuous HRV
3. Check timezone and time display

### Phase 4: Long-term

1. Power/network recovery → MQTT reconnect (observe 24h)
2. iOS background automation limits
3. Bemfa free-tier limits

---

## 4. Minimal Shortcut steps

> HTTP fields per [Bemfa documentation](https://cloud.bemfa.com/docs/src/mqtt.html).

1. **Find All Health Samples** → Heart Rate Variability → latest 1
2. **Get Numbers from Input** → `hrv`
3. **Current Date** → **Format Date** → `time`
4. **Get Current Weather** → `weather`, `temp`
5. **Get Current Location** → city → `city`
6. **Text** JSON:  
   `{"type":"status","hrv":{hrv},"time":"{time}","temp":{temp},"weather":"{weather}","city":"{city}"}`
7. **Dictionary** (`uid`, `topic`, `type`=1, `msg`=payload) → **Get Contents of URL** POST `https://apis.bemfa.com/va/postJsonMsg`

---

## Revision history

| Date | Notes |
|------|-------|
| 2026-05-26 | Restructured from legacy integration TODO doc |
