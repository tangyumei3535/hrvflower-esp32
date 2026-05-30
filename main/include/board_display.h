/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Optional board hook when LEDC backlight device is absent (e.g. AtomS3R LP5562). */
void board_display_backlight_on(void);
void board_display_backlight_off(void);

#ifdef __cplusplus
}
#endif
