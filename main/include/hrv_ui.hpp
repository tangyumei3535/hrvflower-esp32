/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int hrv_ms;
    char time_str[32];    /* send timestamp string from iPhone, e.g. "2026-05-25 14:32" */
    int temp_c;           /* integer °C from Weather app */
    char weather[32];     /* raw condition string from Weather app */
    char city[32];        /* sender's current city from GPS */
} hrv_status_t;

/** Full-screen provisioning UI (breathing indicator + hotspot name). */
void hrv_ui_provisioning_begin(const char *ap_ssid, const char *web_url);
void hrv_ui_provisioning_end(void);

/** Load NVS (if any), build UI once, first paint before Wi-Fi (call after display_init). */
void hrv_ui_init(void);

/** Redraw entire interface from parsed status. */
void drawInterface(const hrv_status_t *status);

/** Parse JSON payload from iOS Shortcut / Bemfa. */
bool hrv_parse_status_json(const char *json, size_t len, hrv_status_t *out);

/**
 * Parse payload, save JSON to NVS, and redraw UI.
 * @return true if JSON was valid and UI updated.
 */
bool hrv_ui_apply_payload(const char *json, size_t len);
