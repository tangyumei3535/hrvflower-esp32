/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "display.hpp"
#include "esp_board_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hrv_imu_wake.h"
#include "hrv_power.hpp"
#include "hrv_ui.hpp"
#include "mqtt_hrv.hpp"
#include "wifi_connect.hpp"
#include "sdkconfig.h"

static const char *TAG = "hrv_main";

extern "C" void app_main(void)
{
#if CONFIG_HRV_LOW_POWER_ENABLE
    hrv_imu_wake_on_boot();
    hrv_power_log_wakeup_reason();
#endif

    esp_board_manager_print_board_info();
    ESP_ERROR_CHECK(esp_board_manager_init());

#if CONFIG_HRV_IMU_WAKE_ENABLE
    (void)hrv_imu_wake_prepare();
#endif

    if (!display_init()) {
        ESP_LOGE(TAG, "display_init failed");
        return;
    }
    wifi_connect_set_display_ready(true);
    ESP_ERROR_CHECK(wifi_connect_init());

    if (display_lock(-1)) {
        hrv_ui_init();
        display_unlock();
    }

    ESP_ERROR_CHECK(mqtt_hrv_start());

#if CONFIG_HRV_LOW_POWER_ENABLE
    (void)mqtt_hrv_wait_activity((uint32_t)CONFIG_HRV_ACTIVE_WINDOW_SEC * 1000U);

    hrv_power_display_off();
    hrv_power_prepare_deep_sleep();
#if CONFIG_HRV_IMU_WAKE_ENABLE
    if (hrv_imu_wake_arm_for_deep_sleep() != ESP_OK) {
        ESP_LOGW(TAG, "IMU wake not armed");
    }
#endif
    hrv_power_enter_deep_sleep();
#else
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif
}
