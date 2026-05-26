/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * M5Stack AtomS3R LP5562 I2C backlight driver.
 */
#include <stdlib.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_board_entry.h"
#include "esp_board_periph.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gen_board_device_custom.h"

static const char *TAG = "atoms3r_backlight";

#define LP5562_REG_ENABLE 0x00
#define LP5562_REG_CFG    0x08
#define LP5562_REG_B_PWM  0x0E

typedef struct {
    i2c_master_dev_handle_t dev;
    const char             *peripheral_name;
} atoms3r_backlight_handle_t;

static atoms3r_backlight_handle_t *s_handle;

static esp_err_t lp5562_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    const uint8_t buf[2] = {reg, value};
    esp_err_t err = ESP_ERR_INVALID_STATE;
    for (int i = 0; i < 3; i++) {
        err = i2c_master_transmit(dev, buf, sizeof(buf), 100);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return err;
}

static esp_err_t lp5562_configure(i2c_master_dev_handle_t dev, uint8_t brightness)
{
    /* M5GFX Light_M5StackAtomS3R sequence */
    ESP_RETURN_ON_ERROR(lp5562_write_reg(dev, LP5562_REG_ENABLE, 0x40), TAG, "enable");
    vTaskDelay(pdMS_TO_TICKS(1));
    ESP_RETURN_ON_ERROR(lp5562_write_reg(dev, LP5562_REG_CFG, 0x01), TAG, "clock");
    ESP_RETURN_ON_ERROR(lp5562_write_reg(dev, 0x70, 0x00), TAG, "cfg2");
    ESP_RETURN_ON_ERROR(lp5562_write_reg(dev, LP5562_REG_B_PWM, brightness), TAG, "pwm");
    return ESP_OK;
}

static int display_backlight_init(void *config, int cfg_size, void **device_handle)
{
    (void)cfg_size;
    if (config == NULL || device_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_handle != NULL) {
        *device_handle = s_handle;
        return ESP_OK;
    }

    const dev_custom_display_backlight_config_t *cfg = config;
    if (cfg->chip == NULL || strcmp(cfg->chip, "lp5562") != 0) {
        ESP_LOGE(TAG, "Unsupported backlight chip: %s", cfg->chip ? cfg->chip : "(null)");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err = esp_board_periph_get_handle(cfg->peripheral_name, (void **)&bus);
    if (err != ESP_OK || bus == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus '%s'", cfg->peripheral_name ? cfg->peripheral_name : "(null)");
        return err == ESP_OK ? ESP_FAIL : err;
    }

    atoms3r_backlight_handle_t *handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return ESP_ERR_NO_MEM;
    }
    handle->peripheral_name = cfg->peripheral_name;

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (uint16_t)cfg->i2c_address,
        .scl_speed_hz = (uint32_t)cfg->i2c_frequency,
    };
    err = i2c_master_bus_add_device(bus, &dev_cfg, &handle->dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        free(handle);
        return err;
    }

    uint8_t brightness = (uint8_t)cfg->brightness;
    if (brightness == 0) {
        brightness = 255;
    }
    err = lp5562_configure(handle->dev, brightness);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LP5562 init failed: %s", esp_err_to_name(err));
        i2c_master_bus_rm_device(handle->dev);
        free(handle);
        return err;
    }

    s_handle = handle;
    *device_handle = handle;
    ESP_LOGI(TAG, "LP5562 backlight ready (brightness=%u)", brightness);
    return ESP_OK;
}

void board_display_backlight_on(void)
{
    if (s_handle == NULL || s_handle->dev == NULL) {
        return;
    }
    (void)lp5562_write_reg(s_handle->dev, LP5562_REG_B_PWM, 255);
}

static int display_backlight_deinit(void *device_handle)
{
    atoms3r_backlight_handle_t *handle = device_handle;
    if (handle == NULL) {
        return ESP_OK;
    }
    if (handle->dev) {
        i2c_master_bus_rm_device(handle->dev);
    }
    if (handle->peripheral_name) {
        esp_board_periph_unref_handle(handle->peripheral_name);
    }
    free(handle);
    if (s_handle == handle) {
        s_handle = NULL;
    }
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(display_backlight, display_backlight_init, display_backlight_deinit);
