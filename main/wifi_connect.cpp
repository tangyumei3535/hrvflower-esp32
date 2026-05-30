/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "wifi_connect.hpp"

#include <cstring>
#include <string>

#include "display.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "hrv_ui.hpp"
#include "ssid_manager.h"
#include "wifi_manager.h"

static const char *TAG = "wifi";

static constexpr int CONNECT_TIMEOUT_SEC = 60;
static constexpr int WIFI_UI_TASK_STACK = 6144;

#define WIFI_EVT_CONNECTED       BIT0
#define WIFI_EVT_CONFIG_EXIT     BIT1
#define WIFI_EVT_CONNECT_TIMEOUT BIT2

enum class WifiUiCmd : uint8_t {
    ProvShow,
    ProvHide,
    WifiStatus,
};

struct WifiUiMsg {
    WifiUiCmd cmd;
    char ap_ssid[33];
    char web_url[64];
    char status_line[48];
};

static EventGroupHandle_t s_wifi_events;
static esp_timer_handle_t s_connect_timer;
static QueueHandle_t s_ui_queue;
static bool s_connected;
static bool s_display_ready;
static bool s_ui_task_started;

static void wifi_ui_task(void *arg)
{
    (void)arg;
    WifiUiMsg msg;
    for (;;) {
        if (xQueueReceive(s_ui_queue, &msg, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!s_display_ready) {
            continue;
        }
        if (msg.cmd == WifiUiCmd::WifiStatus) {
            /* hrv_ui_set_wifi_status acquires LVGL lock internally */
            hrv_ui_set_wifi_status(msg.status_line[0] ? msg.status_line : nullptr);
            continue;
        }
        if (!display_lock(-1)) {
            ESP_LOGW(TAG, "LVGL lock timeout in wifi_ui task");
            continue;
        }
        switch (msg.cmd) {
        case WifiUiCmd::ProvShow:
            hrv_ui_provisioning_begin(msg.ap_ssid, msg.web_url);
            break;
        case WifiUiCmd::ProvHide:
            hrv_ui_provisioning_end();
            break;
        default:
            break;
        }
        display_unlock();
    }
}

static void ensure_wifi_ui_task(void)
{
    if (s_ui_task_started) {
        return;
    }
    s_ui_queue = xQueueCreate(4, sizeof(WifiUiMsg));
    if (!s_ui_queue) {
        ESP_LOGE(TAG, "Failed to create UI queue");
        return;
    }
    if (xTaskCreate(wifi_ui_task, "wifi_ui", WIFI_UI_TASK_STACK, nullptr, 3, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create wifi_ui task");
        vQueueDelete(s_ui_queue);
        s_ui_queue = nullptr;
        return;
    }
    s_ui_task_started = true;
}

static void queue_prov_show(const char *ap_ssid, const char *web_url)
{
    if (!s_display_ready) {
        return;
    }
    ensure_wifi_ui_task();
    if (!s_ui_queue) {
        return;
    }
    WifiUiMsg msg{};
    msg.cmd = WifiUiCmd::ProvShow;
    strncpy(msg.ap_ssid, ap_ssid ? ap_ssid : "", sizeof(msg.ap_ssid) - 1);
    strncpy(msg.web_url, web_url ? web_url : "", sizeof(msg.web_url) - 1);
    if (xQueueSend(s_ui_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "UI queue full (show)");
    }
}

static void queue_prov_hide(void)
{
    if (!s_display_ready || !s_ui_queue) {
        return;
    }
    WifiUiMsg msg{};
    msg.cmd = WifiUiCmd::ProvHide;
    if (xQueueSend(s_ui_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "UI queue full (hide)");
    }
}

static void queue_wifi_status(const char *text)
{
    if (!s_display_ready) {
        return;
    }
    ensure_wifi_ui_task();
    if (!s_ui_queue) {
        return;
    }
    WifiUiMsg msg{};
    msg.cmd = WifiUiCmd::WifiStatus;
    if (text && text[0]) {
        strncpy(msg.status_line, text, sizeof(msg.status_line) - 1);
    }
    if (xQueueSend(s_ui_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "UI queue full (wifi status)");
    }
}

static void on_wifi_event(WifiEvent event, const std::string &data)
{
    switch (event) {
    case WifiEvent::Connected:
        ESP_LOGI(TAG, "Connected to \"%s\"", data.c_str());
        s_connected = true;
        esp_timer_stop(s_connect_timer);
        xEventGroupSetBits(s_wifi_events, WIFI_EVT_CONNECTED);
        /* UI already drawn from NVS before Wi-Fi; redraw here caused a white flash. */
        queue_wifi_status(nullptr);
        queue_prov_hide();
        break;
    case WifiEvent::Disconnected:
        ESP_LOGW(TAG, "Disconnected");
        s_connected = false;
        break;
    case WifiEvent::Connecting: {
        ESP_LOGI(TAG, "Connecting to \"%s\"...", data.c_str());
        char line[48];
        snprintf(line, sizeof(line), "WiFi: %.28s", data.c_str());
        queue_wifi_status(line);
        break;
    }
    case WifiEvent::Scanning:
        ESP_LOGI(TAG, "Scanning for networks...");
        queue_wifi_status("WiFi scanning...");
        break;
    case WifiEvent::ConfigModeEnter: {
        auto &wifi = WifiManager::GetInstance();
        ESP_LOGI(TAG, "Config AP: %s — %s", wifi.GetApSsid().c_str(), wifi.GetApWebUrl().c_str());
        queue_wifi_status(nullptr);
        queue_prov_show(wifi.GetApSsid().c_str(), wifi.GetApWebUrl().c_str());
        break;
    }
    case WifiEvent::ConfigModeExit:
        ESP_LOGI(TAG, "Leaving config AP mode");
        xEventGroupSetBits(s_wifi_events, WIFI_EVT_CONFIG_EXIT);
        break;
    default:
        break;
    }
}

static void on_connect_timeout(void *arg)
{
    (void)arg;
    ESP_LOGW(TAG, "STA connect timeout (%ds), entering config AP", CONNECT_TIMEOUT_SEC);
    xEventGroupSetBits(s_wifi_events, WIFI_EVT_CONNECT_TIMEOUT);
}

static void seed_menuconfig_credentials_if_empty(void)
{
    auto &ssid_manager = SsidManager::GetInstance();
    if (!ssid_manager.GetSsidList().empty()) {
        return;
    }
    if (strlen(CONFIG_HRV_WIFI_SSID) == 0) {
        return;
    }
    ESP_LOGI(TAG, "Seeding Wi-Fi credentials from menuconfig");
    ssid_manager.AddSsid(CONFIG_HRV_WIFI_SSID, CONFIG_HRV_WIFI_PASSWORD);
}

static void start_config_ap_and_wait(void)
{
    auto &wifi = WifiManager::GetInstance();

    xEventGroupClearBits(s_wifi_events, WIFI_EVT_CONFIG_EXIT);
    wifi.StartConfigAp();

    xEventGroupWaitBits(s_wifi_events, WIFI_EVT_CONFIG_EXIT, pdTRUE, pdFALSE, portMAX_DELAY);

    ESP_LOGI(TAG, "Config AP closed, retrying station");
}

static bool start_station_and_wait(int timeout_sec)
{
    auto &wifi = WifiManager::GetInstance();

    xEventGroupClearBits(s_wifi_events, WIFI_EVT_CONNECTED | WIFI_EVT_CONNECT_TIMEOUT);
    s_connected = false;

    esp_timer_stop(s_connect_timer);
    if (timeout_sec > 0) {
        esp_timer_start_once(s_connect_timer, (uint64_t)timeout_sec * 1000000ULL);
    }

    wifi.StartStation();

    const EventBits_t bits = xEventGroupWaitBits(s_wifi_events,
                                                 WIFI_EVT_CONNECTED | WIFI_EVT_CONNECT_TIMEOUT,
                                                 pdTRUE,
                                                 pdFALSE,
                                                 portMAX_DELAY);

    esp_timer_stop(s_connect_timer);

    if (bits & WIFI_EVT_CONNECTED) {
        return true;
    }
    wifi.StopStation();
    return false;
}

static void try_wifi_connect(void)
{
    seed_menuconfig_credentials_if_empty();

    const bool have_saved = !SsidManager::GetInstance().GetSsidList().empty();
    if (!have_saved) {
        ESP_LOGI(TAG, "No saved Wi-Fi credentials");
        start_config_ap_and_wait();
        queue_wifi_status("WiFi connecting...");
        if (!start_station_and_wait(CONNECT_TIMEOUT_SEC)) {
            ESP_LOGE(TAG, "Failed to connect after provisioning");
        }
        return;
    }

    ESP_LOGI(TAG, "Trying saved Wi-Fi credentials");
    queue_wifi_status("WiFi connecting...");
    if (start_station_and_wait(CONNECT_TIMEOUT_SEC)) {
        return;
    }

    ESP_LOGW(TAG, "Saved credentials failed, opening config AP");
    start_config_ap_and_wait();
    queue_wifi_status("WiFi connecting...");
    if (!start_station_and_wait(CONNECT_TIMEOUT_SEC)) {
        ESP_LOGE(TAG, "Failed to connect after provisioning");
    }
}

void wifi_connect_set_display_ready(bool ready)
{
    s_display_ready = ready;
    if (ready) {
        ensure_wifi_ui_task();
    }
}

esp_err_t wifi_connect_init(void)
{
    s_wifi_events = xEventGroupCreate();
    if (!s_wifi_events) {
        return ESP_ERR_NO_MEM;
    }

    esp_timer_create_args_t timer_args = {
        .callback = on_connect_timeout,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_sta_timeout",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&timer_args, &s_connect_timer) != ESP_OK) {
        return ESP_FAIL;
    }

    WifiManagerConfig config;
    config.ssid_prefix = "HRVFlower";
    config.language = "en-US";

    auto &wifi = WifiManager::GetInstance();
    if (!wifi.Initialize(config)) {
        return ESP_FAIL;
    }

    wifi.SetEventCallback(on_wifi_event);
    try_wifi_connect();

    if (!wifi.IsConnected()) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "IP: %s", wifi.GetIpAddress().c_str());
    return ESP_OK;
}

bool wifi_connect_is_ready(void)
{
    return s_connected && WifiManager::GetInstance().IsConnected();
}
