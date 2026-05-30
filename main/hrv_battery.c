/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_battery.hpp"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "hrv_bat";

#if CONFIG_HRV_BATTERY_ADC_ENABLE

/** ESP32-S3 ADC1: GPIO1–10 → CH0–9; GPIO33 → CH5 (M5 Atom-Lite/Matrix base). */
static int gpio_to_adc1_channel(int gpio)
{
    if (gpio >= 1 && gpio <= 10) {
        return gpio - 1;
    }
    if (gpio == 33) {
        return ADC_CHANNEL_5;
    }
    return -1;
}

/**
 * M5 Atomic Battery Base (A151) LED tiers — docs.m5stack.com Atomic Battery Base.
 * 200mAh cell often tops ~3.78–3.82V on GPIO8 ADC; do not use 3.0–4.2V linear (shows ~65% at 3786mV).
 * M5Unified ADC boards use (mv-3300)*100/800 which also underestimates (~61% at 3786mV).
 */
typedef struct {
    int mv;
    int pct;
} bat_curve_point_t;

static const bat_curve_point_t s_m5_a151_curve[] = {
    {3000, 0},  /* tier 1: 0–25% */
    {3480, 25}, /* 3.47V */
    {3620, 50}, /* 3.61V */
    {3780, 100}, /* effective full on A151 ADC (~3.78V); doc chemistry max 4.20V */
};

static int mv_to_percent(int bat_mv)
{
    const size_t n = sizeof(s_m5_a151_curve) / sizeof(s_m5_a151_curve[0]);
    if (bat_mv <= s_m5_a151_curve[0].mv) {
        return 0;
    }
    if (bat_mv >= s_m5_a151_curve[n - 1].mv) {
        return 100;
    }
    for (size_t i = 0; i + 1 < n; ++i) {
        const bat_curve_point_t *a = &s_m5_a151_curve[i];
        const bat_curve_point_t *b = &s_m5_a151_curve[i + 1];
        if (bat_mv < b->mv) {
            return a->pct + (bat_mv - a->mv) * (b->pct - a->pct) / (b->mv - a->mv);
        }
    }
    return 100;
}

bool hrv_battery_read_once(int *percent_out, int *mv_out)
{
    const int gpio = CONFIG_HRV_BATTERY_ADC_GPIO;
    const int ch = gpio_to_adc1_channel(gpio);
    if (ch < 0) {
        ESP_LOGW(TAG, "GPIO%d has no ADC1 mapping", gpio);
        return false;
    }

    adc_oneshot_unit_handle_t unit = NULL;
    if (adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){
                                 .unit_id = ADC_UNIT_1,
                             },
                             &unit)
        != ESP_OK) {
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_oneshot_config_channel(unit, ch, &chan_cfg) != ESP_OK) {
        adc_oneshot_del_unit(unit);
        return false;
    }

    int raw = 0;
    int raw_sum = 0;
    const int samples = 8;
    for (int i = 0; i < samples; ++i) {
        if (adc_oneshot_read(unit, ch, &raw) != ESP_OK) {
            adc_oneshot_del_unit(unit);
            return false;
        }
        raw_sum += raw;
    }
    adc_oneshot_del_unit(unit);

    raw = raw_sum / samples;
    const int adc_mv = (raw * 3300) / 4095;
    const int bat_mv = adc_mv * 2;
    const int pct = mv_to_percent(bat_mv);

    if (percent_out) {
        *percent_out = pct;
    }
    if (mv_out) {
        *mv_out = bat_mv;
    }

    ESP_LOGI(TAG, "GPIO%d raw=%d adc=%dmV bat=%dmV %d%%", gpio, raw, adc_mv, bat_mv, pct);
    return true;
}

#else

bool hrv_battery_read_once(int *percent_out, int *mv_out)
{
    (void)percent_out;
    (void)mv_out;
    return false;
}

#endif
