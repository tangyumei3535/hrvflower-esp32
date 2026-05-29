/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"
#include <stdbool.h>

/** Call after display_init() so provisioning UI can be shown on the LCD. */
void wifi_connect_set_display_ready(bool ready);

/** Block until Wi-Fi is connected (saved STA, or captive portal setup). */
esp_err_t wifi_connect_init(void);

bool wifi_connect_is_ready(void);
