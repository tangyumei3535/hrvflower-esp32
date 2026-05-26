# boards/ — 板级硬件定义

[English](./README.md)

硬件用 YAML 描述，经 **esp_board_manager** 接入工程。`gen-bmgr-config` 生成 `components/gen_bmgr_codes/`；`main/` 业务代码与具体板型解耦。

## 目录结构

```
boards/
└── <vendor>/<board_name>/     # 与 gen-bmgr-config -b 参数一致
    ├── board_info.yaml
    ├── board_peripherals.yaml
    ├── board_devices.yaml
    ├── sdkconfig.defaults.board   # 可选：Flash / PSRAM / Console
    └── setup_device.c               # 可选：非标准 LCD 初始化
```

## 当前已支持板型

| 板型 | 路径 | 芯片 | 说明 |
|------|------|------|------|
| `esp_SensairShuttle` | [espressif/esp_SensairShuttle/](../../boards/espressif/esp_SensairShuttle/) | ESP32-C5 | [README_CN](../../boards/espressif/esp_SensairShuttle/README_CN.md) |
| `m5_AtomS3R` | [m5stack/m5_AtomS3R/](../../boards/m5stack/m5_AtomS3R/) | ESP32-S3 | [README_CN](../../boards/m5stack/m5_AtomS3R/README_CN.md) |

## 选板并编译

换板时 `gen-bmgr-config` 会备份并清除旧 `sdkconfig`，再注入 `board_manager.defaults`：

```bash
idf.py gen-bmgr-config -c boards/<vendor> -b <board>
idf.py build flash monitor
```

快捷脚本：`./scripts/build_board.sh esp_SensairShuttle boards/espressif`

配置优先级：`sdkconfig` → `sdkconfig.defaults` → `board_manager.defaults`（来自 `sdkconfig.defaults.board`）。

## 添加新板型

复制相近板型到 `boards/<vendor>/<name>/`，改 YAML 与可选 `setup_device.c`，再执行 `gen-bmgr-config`。`board_devices.yaml` 中 `display_lcd` 须与 `main/display.cpp` 一致。

YAML schema 与 `idf.py bmgr` 脚手架见 [esp_board_manager](https://components.espressif.com/components/espressif/esp_board_manager)。
