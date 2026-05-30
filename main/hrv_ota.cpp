/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_ota.hpp"

#include <cstring>

#include "cJSON.h"
#include "esp_app_format.h"
#include "esp_check.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
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

esp_err_t hrv_ota_apply_https_url(const char *url)
{
    ESP_RETURN_ON_FALSE(url && url[0], ESP_ERR_INVALID_ARG, TAG, "empty URL");

    ESP_LOGI(TAG, "HTTPS OTA from %s", url);

    esp_http_client_config_t http_cfg = {};
    http_cfg.url = url;
    http_cfg.timeout_ms = 60000;
    http_cfg.keep_alive_enable = true;
#if CONFIG_HRV_OTA_USE_CERT_BUNDLE
    http_cfg.crt_bundle_attach = esp_crt_bundle_attach;
#endif
#if CONFIG_HRV_OTA_SKIP_COMMON_NAME_CHECK
    http_cfg.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http_cfg;

    const esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA success, rebooting");
        esp_restart();
    }
    ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
    return err;
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
