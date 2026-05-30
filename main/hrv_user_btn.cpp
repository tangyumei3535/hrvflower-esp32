/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_user_btn.hpp"

#include "display.hpp"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hrv_battery.hpp"
#include "hrv_ui.hpp"
#include "mqtt_hrv.hpp"
#include "iot_button.h"
#include "button_gpio.h"
#include "sdkconfig.h"

static const char *TAG = "hrv_btn";

#if CONFIG_HRV_USER_BTN_ENABLE

static constexpr int BTN_UI_TASK_STACK = 6144;

static QueueHandle_t s_btn_queue;
static bool s_sys_page_visible;
static button_handle_t s_btn;

static void btn_ui_task(void *arg)
{
    (void)arg;
    uint8_t ev = 0;

    for (;;) {
        if (xQueueReceive(s_btn_queue, &ev, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        (void)ev;

        mqtt_hrv_kick_idle();

        if (!display_lock(-1)) {
            ESP_LOGW(TAG, "LVGL lock timeout");
            continue;
        }

        if (!s_sys_page_visible) {
            int pct = -1;
            int mv = 0;
            if (!hrv_battery_read_once(&pct, &mv)) {
                pct = -1;
            }
            const esp_app_desc_t *desc = esp_app_get_description();
            hrv_ui_show_sysinfo(true, pct, mv, desc ? desc->version : "?");
            s_sys_page_visible = true;
        } else {
            hrv_ui_show_sysinfo(false, 0, 0, NULL);
            s_sys_page_visible = false;
        }

        display_refresh_now();
        display_unlock();
    }
}

static void on_single_click(void *button_handle, void *usr_data)
{
    (void)button_handle;
    (void)usr_data;
    static const uint8_t ev = 1;
    (void)xQueueSend(s_btn_queue, &ev, 0);
}

esp_err_t hrv_user_btn_start(void)
{
    s_btn_queue = xQueueCreate(4, sizeof(uint8_t));
    if (!s_btn_queue) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(btn_ui_task, "hrv_btn_ui", BTN_UI_TASK_STACK, NULL, 5, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    const button_config_t btn_cfg = {};
    const button_gpio_config_t gpio_cfg = {
        .gpio_num = CONFIG_HRV_USER_BTN_GPIO,
        .active_level = 0,
        .enable_power_save = false,
        .disable_pull = false,
    };

    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_btn);
    if (ret != ESP_OK || s_btn == NULL) {
        ESP_LOGE(TAG, "iot_button_new_gpio_device failed");
        return ret != ESP_OK ? ret : ESP_FAIL;
    }

    ret = iot_button_register_cb(s_btn, BUTTON_SINGLE_CLICK, NULL, on_single_click, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "register SINGLE_CLICK failed");
        return ret;
    }

    ESP_LOGI(TAG, "USER button GPIO%d via iot_button (toggle sysinfo + ADC on enter)",
             CONFIG_HRV_USER_BTN_GPIO);
    return ESP_OK;
}

#else

esp_err_t hrv_user_btn_start(void)
{
    return ESP_OK;
}

#endif
