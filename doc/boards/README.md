# boards/ — Board hardware definitions

[中文](./README_CN.md)

Hardware is described in YAML and wired through **esp_board_manager**. `gen-bmgr-config` generates `components/gen_bmgr_codes/`; application code in `main/` stays board-agnostic.

## Layout

```
boards/
└── <vendor>/<board_name>/     # must match gen-bmgr-config -b
    ├── board_info.yaml
    ├── board_peripherals.yaml
    ├── board_devices.yaml
    ├── sdkconfig.defaults.board   # optional: Flash / PSRAM / console
    └── setup_device.c               # optional: custom LCD init
```

## Supported boards

| Board | Path | SoC | Docs |
|-------|------|-----|------|
| `esp_SensairShuttle` | [espressif/esp_SensairShuttle/](../../boards/espressif/esp_SensairShuttle/) | ESP32-C5 | [README](../../boards/espressif/esp_SensairShuttle/README.md) |
| `m5_AtomS3R` | [m5stack/m5_AtomS3R/](../../boards/m5stack/m5_AtomS3R/) | ESP32-S3 | [README](../../boards/m5stack/m5_AtomS3R/README.md) |

## Select board and build

Switching boards backs up and clears `sdkconfig`, then applies `board_manager.defaults`:

```bash
idf.py gen-bmgr-config -c boards/<vendor> -b <board>
idf.py build flash monitor
```

Helper: `./scripts/build_board.sh esp_SensairShuttle boards/espressif`

Config precedence: `sdkconfig` → `sdkconfig.defaults` → `board_manager.defaults` (from `sdkconfig.defaults.board`).

## Add a new board

Copy a similar board under `boards/<vendor>/<name>/`, edit YAML and optional `setup_device.c`, then run `gen-bmgr-config`. Device name `display_lcd` must match `main/display.cpp`.

See [esp_board_manager](https://components.espressif.com/components/espressif/esp_board_manager) for YAML schema and `idf.py bmgr` scaffold.
