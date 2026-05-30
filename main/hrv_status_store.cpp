/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_status_store.hpp"

#include <cstring>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "hrv_store";

static constexpr const char *NVS_NS = "hrv";
static constexpr const char *NVS_KEY = "status_json";

bool hrv_status_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash");
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool hrv_status_store_save(const char *json, size_t len)
{
    if (!json || len == 0 || len >= HRV_STATUS_JSON_MAX_LEN) {
        ESP_LOGW(TAG, "Refuse save: invalid length %u", (unsigned)len);
        return false;
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_blob(nvs, NVS_KEY, json, len);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs save failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Saved status JSON (%u bytes)", (unsigned)len);
    return true;
}

bool hrv_status_store_load(char *json, size_t json_cap, size_t *out_len)
{
    if (!json || json_cap < 2 || !out_len) {
        return false;
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        return false;
    }

    size_t len = 0;
    err = nvs_get_blob(nvs, NVS_KEY, nullptr, &len);
    if (err != ESP_OK || len == 0 || len >= json_cap) {
        nvs_close(nvs);
        return false;
    }

    err = nvs_get_blob(nvs, NVS_KEY, json, &len);
    nvs_close(nvs);
    if (err != ESP_OK || len == 0) {
        return false;
    }

    json[len] = '\0';
    *out_len = len;
    return true;
}
