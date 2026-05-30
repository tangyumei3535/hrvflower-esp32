/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "display.hpp"

#include <algorithm>
#include <cstring>

#include "board_display.h"
#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"
#if defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT) || defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT)
#include "esp_lvgl_port_touch.h"
#endif
#include "driver/ledc.h"
#include "sdkconfig.h"

static const char *TAG = "display";

constexpr int LVGL_TASK_PRIORITY     = 4;
constexpr int LVGL_TASK_STACK_SIZE   = 20 * 1024;
constexpr int LVGL_TASK_MAX_SLEEP_MS = 500;
constexpr int LVGL_TASK_TIMER_PERIOD_MS = 5;
constexpr int BRIGHTNESS_DEFAULT     = 100;

static dev_display_lcd_handles_t *s_lcd_handles = nullptr;
static dev_display_lcd_config_t *s_lcd_cfg = nullptr;
#if defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT) || defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT)
static dev_lcd_touch_i2c_handles_t *s_touch_handles = nullptr;
#endif
static lv_disp_t *s_lvgl_disp = nullptr;

static bool set_lcd_backlight(int brightness_percent)
{
#ifdef CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT
    periph_ledc_config_t *ledc_config = nullptr;
    periph_ledc_handle_t *ledc_handle = nullptr;
    if (esp_board_manager_get_device_handle("lcd_brightness", (void **)&ledc_handle) != ESP_OK) {
        return true;
    }
    dev_ledc_ctrl_config_t *dev_ledc_cfg = nullptr;
    if (esp_board_manager_get_device_config("lcd_brightness", (void **)&dev_ledc_cfg) != ESP_OK
            || dev_ledc_cfg == nullptr) {
        return true;
    }
    if (esp_board_manager_get_periph_config(dev_ledc_cfg->ledc_name, (void **)&ledc_config) != ESP_OK) {
        return true;
    }
    brightness_percent = std::clamp(brightness_percent, 10, 100);
    uint32_t duty = (brightness_percent * ((1 << (uint32_t)ledc_config->duty_resolution) - 1)) / 100;
    ledc_set_duty(ledc_handle->speed_mode, ledc_handle->channel, duty);
    ledc_update_duty(ledc_handle->speed_mode, ledc_handle->channel);
#endif
    return true;
}

static bool init_devices(void)
{
    void *dev_lcd_handle = nullptr;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle("display_lcd", &dev_lcd_handle), TAG, "LCD handle");
    ESP_RETURN_ON_FALSE(dev_lcd_handle, false, TAG, "LCD handle null");
    s_lcd_handles = (dev_display_lcd_handles_t *)dev_lcd_handle;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_config("display_lcd", (void **)&s_lcd_cfg), TAG, "LCD cfg");

#if defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT) || defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT)
    void *dev_touch_handle = nullptr;
    if (esp_board_manager_get_device_handle("lcd_touch", &dev_touch_handle) == ESP_OK && dev_touch_handle) {
        s_touch_handles = (dev_lcd_touch_i2c_handles_t *)dev_touch_handle;
    }
#endif
    return true;
}

static bool init_lvgl_port(void)
{
    lvgl_port_cfg_t lvgl_port_cfg = {
        .task_priority = LVGL_TASK_PRIORITY,
        .task_stack = LVGL_TASK_STACK_SIZE,
        .task_affinity = -1,
        .task_max_sleep_ms = LVGL_TASK_MAX_SLEEP_MS,
        .task_stack_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT,
        .timer_period_ms = LVGL_TASK_TIMER_PERIOD_MS,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_port_cfg), TAG, "lvgl_port_init");

#ifdef CONFIG_ESP_BOARD_M5_ATOMS3R
    const uint32_t lvgl_buf_lines = 20;
    const bool lvgl_double_buf = false;
#else
    const uint32_t lvgl_buf_lines = 50;
    const bool lvgl_double_buf = true;
#endif

    lvgl_port_display_cfg_t disp_cfg{};
    disp_cfg.io_handle = s_lcd_handles->io_handle;
    disp_cfg.panel_handle = s_lcd_handles->panel_handle;
    disp_cfg.buffer_size = static_cast<uint32_t>(s_lcd_cfg->lcd_width * lvgl_buf_lines);
    disp_cfg.double_buffer = lvgl_double_buf;
    disp_cfg.hres = s_lcd_cfg->lcd_width;
    disp_cfg.vres = s_lcd_cfg->lcd_height;
    disp_cfg.monochrome = false;
    disp_cfg.rotation.swap_xy = static_cast<bool>(s_lcd_cfg->swap_xy);
    disp_cfg.rotation.mirror_x = static_cast<bool>(s_lcd_cfg->mirror_x);
    disp_cfg.rotation.mirror_y = static_cast<bool>(s_lcd_cfg->mirror_y);
    disp_cfg.flags.buff_dma = true;
    disp_cfg.flags.buff_spiram = false;
    disp_cfg.flags.swap_bytes = true;

    if (strcmp(s_lcd_cfg->sub_type, "spi") == 0) {
        s_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    } else {
        lvgl_port_display_dsi_cfg_t dsi_cfg{};
        dsi_cfg.flags.avoid_tearing = false;
        s_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    }
    ESP_RETURN_ON_FALSE(s_lvgl_disp, false, TAG, "lvgl_port_add_disp");

    set_lcd_backlight(BRIGHTNESS_DEFAULT);
    board_display_backlight_on();

#if defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT) || defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT)
    if (s_touch_handles) {
        lvgl_port_touch_cfg_t touch_cfg = {
            .disp = s_lvgl_disp,
            .handle = s_touch_handles->touch_handle,
        };
        lvgl_port_add_touch(&touch_cfg);
    }
#endif
    return true;
}

bool display_init(void)
{
    ESP_RETURN_ON_FALSE(init_devices(), false, TAG, "init_devices");
    ESP_RETURN_ON_FALSE(init_lvgl_port(), false, TAG, "init_lvgl_port");
    ESP_LOGI(TAG, "LCD %dx%d ready", s_lcd_cfg->lcd_width, s_lcd_cfg->lcd_height);
    return true;
}

bool display_get_info(display_info_t *info)
{
    ESP_RETURN_ON_FALSE(info && s_lcd_cfg, false, TAG, "invalid");
    info->width = s_lcd_cfg->lcd_width;
    info->height = s_lcd_cfg->lcd_height;
    return true;
}

lv_disp_t *display_get_lvgl_disp(void)
{
    return s_lvgl_disp;
}

bool display_lock(int timeout_ms)
{
    if (timeout_ms < 0) {
        timeout_ms = 0;
    } else if (timeout_ms == 0) {
        timeout_ms = 1;
    }
    return lvgl_port_lock(timeout_ms);
}

void display_unlock(void)
{
    lvgl_port_unlock();
}
