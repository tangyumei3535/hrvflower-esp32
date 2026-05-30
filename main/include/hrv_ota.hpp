/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"

/** Log PROJECT_VER / esp_app_desc at boot. */
void hrv_ota_log_running_version(void);

/**
 * Download firmware from HTTPS URL and reboot on success.
 * Blocks until complete, failed, or reboot.
 */
esp_err_t hrv_ota_apply_https_url(const char *url);

/**
 * MQTT JSON command: {"type":"ota","url":"https://host/path.bin"}
 * @return true if payload was an OTA command (handled or rejected).
 */
bool hrv_ota_try_mqtt_command(const char *json, size_t len);
