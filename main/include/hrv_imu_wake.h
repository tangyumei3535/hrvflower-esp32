/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void hrv_imu_wake_on_boot(void);
esp_err_t hrv_imu_wake_prepare(void);
esp_err_t hrv_imu_wake_arm_for_deep_sleep(void);

#ifdef __cplusplus
}
#endif
