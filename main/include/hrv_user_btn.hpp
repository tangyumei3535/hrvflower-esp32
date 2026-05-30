/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

/** GPIO interrupt on USER button; toggles system-info UI (battery read on enter only). */
esp_err_t hrv_user_btn_start(void);
