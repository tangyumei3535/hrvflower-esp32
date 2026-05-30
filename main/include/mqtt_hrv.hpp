/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

esp_err_t mqtt_hrv_start(void);

/** Block until a valid MQTT payload is applied or @p timeout_ms elapses. */
bool mqtt_hrv_wait_activity(uint32_t timeout_ms);
