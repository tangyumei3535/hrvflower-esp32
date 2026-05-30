/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_power.hpp"

#include "board_display.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "hrv_power";

void hrv_power_log_wakeup_reason(void)
{
    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        ESP_LOGI(TAG, "Wake: reset");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "Wake: EXT1 mask=0x%llx",
                 (unsigned long long)esp_sleep_get_ext1_wakeup_status());
        break;
    default:
        break;
    }
}

void hrv_power_display_off(void)
{
    board_display_backlight_off();
}

void hrv_power_enter_deep_sleep(void)
{
    esp_deep_sleep_start();
}
