/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_ota.hpp"

#include <cstdio>
#include <cstring>

#include "cJSON.h"
#include "display.hpp"
#include "esp_app_format.h"
#include "esp_check.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hrv_ui.hpp"
#include "sdkconfig.h"

#if CONFIG_HRV_OTA_ENABLE
#include "esp_crt_bundle.h"
#endif

static const char *TAG = "hrv_ota";

void hrv_ota_log_running_version(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Firmware %s (%s) on %s", desc->version, desc->project_name,
             running ? running->label : "?");
}

#if CONFIG_HRV_OTA_ENABLE

static void ota_ui_progress(esp_https_ota_handle_t handle, const char *phase)
{
    char line[40];
    const int total = esp_https_ota_get_image_size(handle);
    const int done = esp_https_ota_get_image_len_read(handle);

    if (total > 0 && done >= 0) {
        const int pct = (done * 100) / total;
        snprintf(line, sizeof(line), "OTA %s %d%%", phase, pct);
    } else {
        snprintf(line, sizeof(line), "OTA %s...", phase);
    }
    hrv_ui_set_ota_status(line);
}

esp_err_t hrv_ota_apply_https_url(const char *url)
{
    ESP_RETURN_ON_FALSE(url && url[0], ESP_ERR_INVALID_ARG, TAG, "empty URL");

    ESP_LOGI(TAG, "HTTPS OTA from %s", url);
    hrv_ui_set_ota_status("OTA starting...");

    esp_http_client_config_t http_cfg = {};
    http_cfg.url = url;
    http_cfg.timeout_ms = 120000;
    http_cfg.keep_alive_enable = true;
    http_cfg.buffer_size = 4096;
    http_cfg.buffer_size_tx = 2048;
#if CONFIG_HRV_OTA_USE_CERT_BUNDLE
    http_cfg.crt_bundle_attach = esp_crt_bundle_attach;
#endif
#if CONFIG_HRV_OTA_SKIP_COMMON_NAME_CHECK
    http_cfg.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http_cfg;
#if CONFIG_ESP_HTTPS_OTA_ENABLE_PARTIAL_DOWNLOAD
    ota_cfg.partial_http_download = true;
    ota_cfg.max_http_request_size = 64 * 1024;
#endif

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        hrv_ui_set_ota_status("OTA failed");
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return err;
    }

    int last_pct = -1;
    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        const int total = esp_https_ota_get_image_size(handle);
        const int done = esp_https_ota_get_image_len_read(handle);
        if (total > 0) {
            const int pct = (done * 100) / total;
            if (pct != last_pct) {
                last_pct = pct;
                ota_ui_progress(handle, "DL");
            }
        } else if (last_pct < 0) {
            last_pct = 0;
            ota_ui_progress(handle, "DL");
        }
    }

    if (err != ESP_OK) {
        hrv_ui_set_ota_status("OTA failed");
        esp_https_ota_abort(handle);
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        return err;
    }

    hrv_ui_set_ota_status("OTA verify...");
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        hrv_ui_set_ota_status("OTA failed");
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        return err;
    }

    hrv_ui_set_ota_status("OTA OK reboot");
    ESP_LOGI(TAG, "OTA success, rebooting");
    vTaskDelay(pdMS_TO_TICKS(400));
    esp_restart();
    return ESP_OK;
}

bool hrv_ota_try_mqtt_command(const char *json, size_t len)
{
    if (!json || len == 0) {
        return false;
    }

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) {
        return false;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type) || strcmp(type->valuestring, "ota") != 0) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *url_item = cJSON_GetObjectItem(root, "url");
    if (!cJSON_IsString(url_item) || !url_item->valuestring[0]) {
        ESP_LOGW(TAG, "OTA command missing url");
        cJSON_Delete(root);
        return true;
    }

    char url[CONFIG_HRV_OTA_URL_MAX_LEN];
    strlcpy(url, url_item->valuestring, sizeof(url));
    cJSON_Delete(root);

    (void)hrv_ota_apply_https_url(url);
    return true;
}

#else

esp_err_t hrv_ota_apply_https_url(const char *url)
{
    (void)url;
    return ESP_ERR_NOT_SUPPORTED;
}

bool hrv_ota_try_mqtt_command(const char *json, size_t len)
{
    (void)json;
    (void)len;
    return false;
}

#endif
