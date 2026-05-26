# Stato firmware e roadmap

[English](./FIRMWARE.md) · [中文](./FIRMWARE_CN.md) · [日本語](./FIRMWARE_JA.md)

Cosa il **firmware attuale può e non può fare**, più i **piani di sviluppo**.  
Per l’integrazione end-to-end (Bemfa, Scorciatoie iOS), vedi [INTEGRATION.md](./INTEGRATION.md).

Correlati: [README_IT.md](./README_IT.md) · [main/Kconfig.projbuild](../main/Kconfig.projbuild)

---

## 1. Sviluppi futuri (roadmap)

### 1.1 Interfaccia provisioning Wi-Fi

- [ ] **Problema**: schermo disturbato senza rete
- [ ] **Obiettivo**: configurazione Wi-Fi on-device (SSID/password o SoftAP)
- [ ] **Atteso**: credenziali in NVS, riconnessione automatica; schermata guida offline
- [ ] **Coinvolge**: `main/wifi_connect.cpp`, UI LVGL provisioning, NVS

### 1.2 Basso consumo + persistenza NVS periodica

- [ ] **Obiettivo**: ridurre consumo in standby; wake circa **ogni ora**
- [ ] **Comportamento**: dopo il wake salva HRV in NVS, poi sleep o breve sync MQTT
- [ ] **Coinvolge**: ESP-IDF sleep API, wake RTC, LCD/MQTT vs sleep

### 1.3 Richieste avviate da ESP32

- [ ] **Obiettivo**: il ricevitore non aspetta solo push MQTT — **ESP32 avvia** sync o trigger push
- [ ] **Esempi**:
  - Pulsante / touch: tasto on-board richiede subito l’ultimo HRV
  - Wake low-power: prima sync attiva dopo sleep, poi decide se dormire di nuovo
- [ ] **Coinvolge**: `esp_http_client` / MQTT publish, macchina a stati, `hrv_ui` / NVS / sleep; UI opzionale “richiesta in corso…”

### 1.4 Aggiornamenti OTA

- [ ] **Obiettivo**: aggiornamento Over-The-Air senza flash USB
- [ ] **Approccio**: `esp_https_ota`; partizioni `ota_0` / `ota_1`
- [ ] **Coinvolge**: `partitions.csv`, versione, URL upgrade opzionale via MQTT

### 1.5 Rotazione automatica display (IMU)

- [ ] **Obiettivo**: ruotare LVGL per orientamento (0° / 90° / 180° / 270°)
- [ ] **Hardware**: es. BMI270 su AtomS3R (I2C `0x68`)
- [ ] **Coinvolge**: `esp_lcd_panel_mirror` / `lv_disp_set_rotation`, `main/display.cpp`

### 1.6 Modalità BLE (senza Wi-Fi)

- [ ] **Obiettivo**: senza Wi-Fi, ricevere JSON HRV via **BLE** e aggiornare UI
- [ ] **Caso**: niente router, sync locale, percorso dati prima del provisioning Wi-Fi
- [ ] **Coinvolge**: NimBLE / GATT, companion o Shortcut BLE, riuso `hrv_parse_status_json()`, switch Wi-Fi/MQTT vs BLE

### 1.7 Altri miglioramenti opzionali

- [ ] Subscribe a `{topic}` e `{topic}/set`
- [ ] UI: indicatore MQTT / ultimo aggiornamento
- [ ] Schermata avvio: “in attesa del primo payload”
- [ ] Icone meteo (vettoriali LVGL)
- [ ] MQTT over TLS (9503)
- [ ] Gestione MQTT retain — ultimo frame dopo reboot

---

## 2. Dopo il flash: cosa c’è / cosa manca

### 2.1 Implementato (menuconfig ok + LCD pronto)

| Funzione | Note |
|----------|------|
| Wi-Fi STA | Riconnessione automatica (20 tentativi) |
| Bemfa MQTT | Default `mqtt://bemfa.com:9501`; UID singolo o AppID multi-dispositivo |
| Subscribe topic | Default `hrv001` (`BEMFA_MQTT_TOPIC` configurabile) |
| JSON → UI | `drawInterface()` **solo** su payload MQTT valido |
| Fiore a 5 stadi | Soglie HRV → bocciolo grigio / rosso scuro / arancio / rosa / oro+verde |
| Meteo in alto + HRV in basso | temp, città, ora aggiornamento |
| Riconnessione MQTT | gestita da ESP-MQTT |

### 2.2 Non in repo / non implementato

| Funzione | Note |
|----------|------|
| Scorciatoie iPhone / Salute | Fuori repo; vedi [INTEGRATION.md](./INTEGRATION.md) |
| Account e topic Bemfa | Registrazione utente |
| Touch / IMU / voce | Non usati in questa demo; rotazione IMU → §1.5 |
| UI stato offline / MQTT | Nessun indicatore “in attesa push” |
| Schermo disturbato offline | Serve UI provisioning (§1.1) |
| Retain ultimo frame all’avvio | Non gestito |
| MQTT TLS (9503) | Solo 9501 in chiaro |
| OTA / provisioning / NVS / richieste ESP32 / BLE | Roadmap §1 |

### 2.3 Comportamento tipico

- **UID assente**: MQTT fallisce; UI segnaposto (`--:--`, `HRV: 0ms`).
- **UID ok, mittente non configurato**: seriale `MQTT connected, subscribe hrv001`; fiore invariato.
- **Pipeline completa**: ogni POST HTTP riuscito aggiorna il display **una volta**.

### 2.4 Monitor seriale

```bash
idf.py -p <PORT> monitor
```

`<PORT>` es. macOS `/dev/cu.usbmodem*`, Linux `/dev/ttyUSB*`.

---

## 3. Indice sorgenti

| File | Ruolo |
|------|-------|
| [main/main.cpp](../main/main.cpp) | Avvio: board, Wi-Fi, display, MQTT |
| [main/mqtt_hrv.cpp](../main/mqtt_hrv.cpp) | Subscribe, parse, trigger UI |
| [main/hrv_ui.cpp](../main/hrv_ui.cpp) | UI e stadi fiore |
| [main/wifi_connect.cpp](../main/wifi_connect.cpp) | Wi-Fi STA |
| [main/display.cpp](../main/display.cpp) | LCD + LVGL |
| [main/Kconfig.projbuild](../main/Kconfig.projbuild) | Kconfig Wi-Fi / Bemfa |

---

## Cronologia revisioni

| Data | Note |
|------|------|
| 2026-05-26 | Separato dal vecchio doc integrazione |
