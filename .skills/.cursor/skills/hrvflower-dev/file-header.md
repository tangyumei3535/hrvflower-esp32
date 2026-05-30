# Source file header (project firmware)

All **new or touched** files under `main/` (sources at `main/`, headers in `main/include/`), `boards/**` board-specific C sources, and other **Espressif-owned** project firmware must use this exact block at line 1 (before includes):

```c
/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
```

Reference: `main/hrv_status_store.cpp`.

## Rules

- Use the **multi-line** `/* ... */` form above — not a one-line `/* SPDX-... */` shortcut.
- No extra description, `@file`, or reference links in the header.
- **Do not** change headers in `components/esp-wifi-connect/` (MIT, upstream style).

## Applies to

| Path | Header |
|------|--------|
| `main/*.{c,cpp}` | Espressif block |
| `main/include/*.{h,hpp}` | Espressif block |
| `boards/**/setup_device.c`, `*_backlight.c`, etc. | Espressif block |
| `components/esp-wifi-connect/**` | Keep existing MIT / upstream headers |
