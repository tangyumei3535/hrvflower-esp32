/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * HRV "Emotion Flower" receiver for ESP-SensairShuttle (ESP32-C5).
 * Subscribes to Bemfa MQTT and renders minimalist UI on the 1.83" LCD.
 */
#include "display.hpp"
#include "esp_board_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hrv_ui.hpp"
#include "mqtt_hrv.hpp"
#include "wifi_connect.hpp"

static const char *TAG = "hrv_main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "HRV Emotion Display — ESP-SensairShuttle");

    esp_board_manager_print_board_info();
    ESP_ERROR_CHECK(esp_board_manager_init());

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

    ESP_LOGI(TAG, "Running — UI updates on new MQTT payloads only");

    while (true) {
        /* MQTT runs in its own task; keep main loop idle for watchdog friendliness. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
