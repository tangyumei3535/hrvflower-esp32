/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdbool.h>

/** Max stored MQTT/Shortcut JSON payload (NVS blob). */
#define HRV_STATUS_JSON_MAX_LEN 512

/** Call once before load/save (e.g. before hrv_ui_init). Idempotent with Wi-Fi init. */
bool hrv_status_store_init(void);

/** Persist last valid status JSON (replaces previous blob). */
bool hrv_status_store_save(const char *json, size_t len);

/** Load blob into @p json; @p out_len is written length. */
bool hrv_status_store_load(char *json, size_t json_cap, size_t *out_len);
