/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "lvgl.h"

typedef struct {
    uint16_t width;
    uint16_t height;
} display_info_t;

bool display_init(void);
bool display_get_info(display_info_t *info);
lv_disp_t *display_get_lvgl_disp(void);

/** Acquire LVGL mutex before any LVGL API call from non-LVGL tasks. */
bool display_lock(int timeout_ms);
void display_unlock(void);

/** Push a full LVGL frame to the panel (call before heavy Wi-Fi work). */
void display_refresh_now(void);
