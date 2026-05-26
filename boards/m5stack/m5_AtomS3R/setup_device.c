/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * M5Stack AtomS3R — GC9107 panel factory.
 */
#include "esp_check.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "atoms3r_setup";

/* GC9107 init sequence via gc9a01 driver. */
static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb2, (uint8_t[]){0x2f}, 1, 0},
    {0xb3, (uint8_t[]){0x03}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x01}, 1, 0},
    {0xac, (uint8_t[]){0xcb}, 1, 0},
    {0xab, (uint8_t[]){0x0e}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x19}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xe8, (uint8_t[]){0x24}, 1, 0},
    {0xe9, (uint8_t[]){0x48}, 1, 0},
    {0xea, (uint8_t[]){0x22}, 1, 0},
    {0xc6, (uint8_t[]){0x30}, 1, 0},
    {0xc7, (uint8_t[]){0x18}, 1, 0},
    {0xf0,
     (uint8_t[]){0x1f, 0x28, 0x04, 0x3e, 0x2a, 0x2e, 0x20, 0x00, 0x0c, 0x06,
                 0x00, 0x1c, 0x1f, 0x0f},
     14, 0},
    {0xf1,
     (uint8_t[]){0x00, 0x2d, 0x2f, 0x3c, 0x6f, 0x1c, 0x0b, 0x00, 0x00, 0x00,
                 0x07, 0x0d, 0x11, 0x0f},
     14, 0},
};

esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_panel_dev_config_t *panel_dev_config,
                                    esp_lcd_panel_handle_t *ret_panel)
{
    gc9a01_vendor_config_t vendor_cfg = {
        .init_cmds = gc9107_lcd_init_cmds,
        .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9107_lcd_init_cmds[0]),
    };

    esp_lcd_panel_dev_config_t panel_cfg = *panel_dev_config;
    panel_cfg.vendor_config = &vendor_cfg;

    esp_err_t ret = esp_lcd_new_panel_gc9a01(io, &panel_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GC9107 (gc9a01) panel init failed");
        return ret;
    }

    ret = esp_lcd_panel_set_gap(*ret_panel, 0, 32);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "esp_lcd_panel_set_gap failed: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "GC9107 panel ready (128x128, gap_y=32)");
    return ESP_OK;
}
