/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

esp_err_t mqtt_hrv_start(void);

/** Block until @p idle_after_last_ms passes with no new valid MQTT payload. */
void mqtt_hrv_active_window(uint32_t idle_after_last_ms);
