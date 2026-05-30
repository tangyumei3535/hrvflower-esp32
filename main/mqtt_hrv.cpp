/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mqtt_hrv.hpp"

#include <cstring>

#include "display.hpp"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hrv_ui.hpp"
#include "mqtt_client.h"

static const char *TAG = "mqtt_hrv";

static esp_mqtt_client_handle_t s_client;
static char s_mqtt_client_id[64];

static void configure_mqtt_credentials(esp_mqtt_client_config_t *mqtt_cfg)
{
#if CONFIG_BEMFA_MQTT_AUTH_APPKEY
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_mqtt_client_id, sizeof(s_mqtt_client_id), "hrv_%02X%02X%02X%02X%02X%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);
    mqtt_cfg->credentials.username = CONFIG_BEMFA_MQTT_APP_ID;
    mqtt_cfg->credentials.authentication.password = CONFIG_BEMFA_MQTT_SECRET_KEY;
#else
    strlcpy(s_mqtt_client_id, CONFIG_BEMFA_PRIVATE_KEY, sizeof(s_mqtt_client_id));
#endif
    mqtt_cfg->credentials.client_id = s_mqtt_client_id;
}

static void on_mqtt_payload(const char *data, int data_len)
{
    if (data_len <= 0) {
        return;
    }

    if (display_lock(200)) {
        if (!hrv_ui_apply_payload(data, (size_t)data_len)) {
            ESP_LOGW(TAG, "Ignored payload: %.*s", data_len, data);
        }
        display_unlock();
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected as %s, subscribe %s", s_mqtt_client_id,
                 CONFIG_BEMFA_MQTT_TOPIC);
        esp_mqtt_client_subscribe(s_client, CONFIG_BEMFA_MQTT_TOPIC, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_DATA:
        if (event->topic_len > 0 && event->data_len > 0) {
            on_mqtt_payload(event->data, event->data_len);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

esp_err_t mqtt_hrv_start(void)
{
    if (strlen(CONFIG_BEMFA_PRIVATE_KEY) == 0) {
        ESP_LOGE(TAG, "Set BEMFA_PRIVATE_KEY in menuconfig");
        return ESP_ERR_INVALID_STATE;
    }

#if CONFIG_BEMFA_MQTT_AUTH_APPKEY
    if (strlen(CONFIG_BEMFA_MQTT_APP_ID) == 0 || strlen(CONFIG_BEMFA_MQTT_SECRET_KEY) == 0) {
        ESP_LOGE(TAG, "Set BEMFA_MQTT_APP_ID and BEMFA_MQTT_SECRET_KEY in menuconfig");
        return ESP_ERR_INVALID_STATE;
    }
#endif

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = CONFIG_BEMFA_MQTT_BROKER_URI;
    configure_mqtt_credentials(&mqtt_cfg);
    mqtt_cfg.session.keepalive = 60;
    mqtt_cfg.network.reconnect_timeout_ms = 5000;
    mqtt_cfg.network.timeout_ms = 10000;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        return ESP_ERR_NO_MEM;
    }

    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
    return esp_mqtt_client_start(s_client);
}
