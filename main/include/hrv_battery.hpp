/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * One-shot battery read (ADC init → sample → teardown). For M5 Atomic Battery Base
 * divider: Vbat_mV ≈ 2 × Vadc_mV (12-bit, 3.3 V ref).
 * @return true if a sample was taken and @p percent_out / @p mv_out are valid.
 */
bool hrv_battery_read_once(int *percent_out, int *mv_out);

#ifdef __cplusplus
}
#endif
