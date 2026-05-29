# esp-wifi-connect (hrvflower)

STA + captive portal for Wi-Fi setup (MIT). Vendored in-tree for HRV Emotion Flower.

## Behavior

- **STA**: reads credentials from NVS namespace `wifi` (`ssid` / `password`, up to 10 pairs).
- **Config AP**: SoftAP + HTTP at `http://192.168.4.1` (SSID prefix set in `main/wifi_connect.cpp`, default `HRVFlower-XXXX`).

## `assets/` (build-required)

| File | Role |
|------|------|
| `wifi_configuration.html` | Provisioning UI (`/`, `/scan`, `/submit`) |
| `wifi_configuration_done.html` | Success page → `POST /exit` |

Embedded via `EMBED_TXTFILES` in `CMakeLists.txt`; do not remove.

## Project wiring

Boot policy and LCD provisioning UI live in `main/wifi_connect.cpp`, not in this component.

See `LICENSE` for third-party MIT terms.
