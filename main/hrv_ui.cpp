/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_ui.hpp"

#include "hrv_status_store.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "cJSON.h"
#include "display.hpp"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "hrv_ui";

static constexpr int TEMP_UNSET = -999;
static constexpr int COMPACT_SCREEN_MAX = 160;

static bool s_ui_compact = false;
static int s_panel_w = 148;
static int s_panel_h = 148;
static int s_safe_inset = 10;
static int s_top_bar_h = 22;
static int s_bottom_bar_h = 16;
static float s_flower_scale = 1.0f;

static lv_obj_t *s_root = nullptr;
static lv_obj_t *s_prov_root = nullptr;
static lv_obj_t *s_prov_glow = nullptr;
static lv_obj_t *s_flower_root = nullptr;
static lv_obj_t *s_lbl_weather = nullptr;
static lv_obj_t *s_lbl_time = nullptr;
static lv_obj_t *s_lbl_hrv = nullptr;
static lv_obj_t *s_lbl_city = nullptr;
static lv_obj_t *s_lbl_top_scroll = nullptr;
static lv_obj_t *s_lbl_bottom_scroll = nullptr;
static lv_obj_t *s_divider = nullptr;

static int s_screen_w = 284;
static int s_screen_h = 240;
static hrv_status_t s_last_status = {};
static bool s_bottom_show_hrv = true;
static lv_timer_t *s_bottom_timer = nullptr;

static lv_color_t color_hex(uint32_t rgb)
{
    return lv_color_hex(rgb);
}

static void configure_layout_metrics(void)
{
    s_ui_compact = (s_screen_w <= COMPACT_SCREEN_MAX && s_screen_h <= COMPACT_SCREEN_MAX);
    if (s_ui_compact) {
        s_panel_w = 84;
        s_panel_h = 84;
        s_safe_inset = 2;
        s_top_bar_h = 12;
        s_bottom_bar_h = 12;
        s_flower_scale = 0.58f;
    } else {
        s_panel_w = 148;
        s_panel_h = 148;
        s_safe_inset = 10;
        s_top_bar_h = 22;
        s_bottom_bar_h = 16;
        s_flower_scale = 1.0f;
    }
}

static void clear_flower_children(void)
{
    if (!s_flower_root) {
        return;
    }
    uint32_t n = lv_obj_get_child_cnt(s_flower_root);
    for (int i = (int)n - 1; i >= 0; --i) {
        lv_obj_t *child = lv_obj_get_child(s_flower_root, i);
        lv_obj_delete(child);
    }
}

static lv_obj_t *add_circle(int cx, int cy, int r, uint32_t color, int opa)
{
    lv_obj_t *c = lv_obj_create(s_flower_root);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, r * 2, r * 2);
    lv_obj_set_pos(c, cx - r, cy - r);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c, color_hex(color), 0);
    lv_obj_set_style_bg_opa(c, opa, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    return c;
}

static lv_obj_t *add_petal_ring(int cx, int cy, int radius, int petal_r, uint32_t color, int count)
{
    for (int i = 0; i < count; ++i) {
        float a = (360.0f / count) * i;
        float rad = a * 3.14159265f / 180.0f;
        int px = cx + (int)(radius * cosf(rad));
        int py = cy + (int)(radius * sinf(rad));
        add_circle(px, py, petal_r, color, LV_OPA_COVER);
    }
    return nullptr;
}

static int scaled_i(int value)
{
    return (int)(value * s_flower_scale + 0.5f);
}

static int hrv_to_stage(int hrv_ms)
{
    if (hrv_ms < 20) {
        return 0;
    }
    if (hrv_ms <= 30) {
        return 1;
    }
    if (hrv_ms <= 39) {
        return 2;
    }
    if (hrv_ms <= 49) {
        return 3;
    }
    return 4;
}

static void draw_flower_stage(int stage)
{
    const int cx = s_panel_w / 2;
    const int cy = s_panel_h / 2 + scaled_i(5);

    switch (stage) {
    case 0:
        add_circle(cx, cy, scaled_i(10), 0x4A4A4A, LV_OPA_COVER);
        add_circle(cx, cy - scaled_i(2), scaled_i(6), 0x6B6D6D, LV_OPA_COVER);
        break;
    case 1:
        add_circle(cx, cy, scaled_i(8), 0xB84800, LV_OPA_COVER);
        add_petal_ring(cx, cy, scaled_i(14), scaled_i(6), 0xFD6000, 6);
        add_petal_ring(cx, cy, scaled_i(8), scaled_i(4), 0xFF9A00, 4);
        break;
    case 2:
        add_circle(cx, cy, scaled_i(7), 0xC2185B, LV_OPA_COVER);
        add_petal_ring(cx, cy, scaled_i(18), scaled_i(8), 0xF48FB1, 8);
        add_petal_ring(cx, cy, scaled_i(10), scaled_i(5), 0xF8BBD0, 5);
        break;
    case 3:
        add_circle(cx, cy, scaled_i(8), 0xFF6F00, LV_OPA_COVER);
        add_petal_ring(cx, cy, scaled_i(20), scaled_i(9), 0xFFAB00, 8);
        add_petal_ring(cx, cy, scaled_i(12), scaled_i(5), 0xFF6D00, 6);
        break;
    default:
        add_circle(cx, cy, scaled_i(22), 0x2E7D32, LV_OPA_40);
        add_petal_ring(cx, cy, scaled_i(24), scaled_i(4), 0x43A047, 12);
        add_circle(cx, cy, scaled_i(8), 0xFF6F00, LV_OPA_COVER);
        add_petal_ring(cx, cy, scaled_i(20), scaled_i(9), 0xFFAB00, 8);
        add_petal_ring(cx, cy, scaled_i(12), scaled_i(5), 0xFF6D00, 6);
        break;
    }
}

static void style_scroll_label(lv_obj_t *label, uint32_t color, const lv_font_t *font, int width)
{
    lv_obj_set_style_text_color(label, color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
}

static void update_compact_top(const hrv_status_t *status)
{
    char top_line[96];
    const char *weather = status->weather[0] ? status->weather : "--";
    const char *city = status->city[0] ? status->city : "--";
    if (status->temp_c != TEMP_UNSET) {
        snprintf(top_line, sizeof(top_line), "%dC|%s|%s", status->temp_c, weather, city);
    } else {
        snprintf(top_line, sizeof(top_line), "--C|%s|%s", weather, city);
    }
    lv_label_set_text(s_lbl_top_scroll, top_line);
}

static void update_compact_bottom(const hrv_status_t *status)
{
    char bottom_line[48];
    if (s_bottom_show_hrv) {
        snprintf(bottom_line, sizeof(bottom_line), "HRV: %dms", status->hrv_ms);
    } else {
        snprintf(bottom_line, sizeof(bottom_line), "%s",
                 status->time_str[0] ? status->time_str : "--");
    }
    lv_label_set_text(s_lbl_bottom_scroll, bottom_line);
}

static void bottom_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_bottom_show_hrv = !s_bottom_show_hrv;
    update_compact_bottom(&s_last_status);
}

static void update_standard_labels(const hrv_status_t *status)
{
    char top_left[56];
    const char *weather = status->weather[0] ? status->weather : "--";
    if (status->temp_c != TEMP_UNSET) {
        snprintf(top_left, sizeof(top_left), "%dC|%s", status->temp_c, weather);
    } else {
        snprintf(top_left, sizeof(top_left), "--C|%s", weather);
    }
    lv_label_set_text(s_lbl_weather, top_left);
    lv_label_set_text(s_lbl_city, status->city[0] ? status->city : "--");

    char update_line[64];
    snprintf(update_line, sizeof(update_line), "last update: %s",
             status->time_str[0] ? status->time_str : "--");
    lv_label_set_text(s_lbl_time, update_line);

    char hrv_line[24];
    snprintf(hrv_line, sizeof(hrv_line), "HRV: %dms", status->hrv_ms);
    lv_label_set_text(s_lbl_hrv, hrv_line);
}

static void prov_breath_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_bg_opa(static_cast<lv_obj_t *>(var), (lv_opa_t)v, 0);
}

static void prov_breath_anim(void *obj, int32_t start, int32_t end)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_duration(&a, 1400);
    lv_anim_set_playback_duration(&a, 1400);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, prov_breath_anim_cb);
    lv_anim_start(&a);
}

void hrv_ui_provisioning_end(void)
{
    if (s_prov_root) {
        lv_obj_delete(s_prov_root);
        s_prov_root = nullptr;
        s_prov_glow = nullptr;
    }
}

void hrv_ui_provisioning_begin(const char *ap_ssid, const char *web_url)
{
    hrv_ui_provisioning_end();

    display_info_t info = {};
    display_get_info(&info);
    const int w = info.width > 0 ? info.width : 128;
    const int h = info.height > 0 ? info.height : 128;
    const bool compact = (w <= COMPACT_SCREEN_MAX && h <= COMPACT_SCREEN_MAX);
    const lv_font_t *title_font = compact ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
    const lv_font_t *body_font = compact ? &lv_font_montserrat_10 : &lv_font_montserrat_12;
    const int glow_size = compact ? 36 : 56;

    s_prov_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_prov_root);
    lv_obj_set_size(s_prov_root, w, h);
    lv_obj_set_style_bg_color(s_prov_root, color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_prov_root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_prov_root, LV_OBJ_FLAG_SCROLLABLE);

    s_prov_glow = lv_obj_create(s_prov_root);
    lv_obj_remove_style_all(s_prov_glow);
    lv_obj_set_size(s_prov_glow, glow_size, glow_size);
    lv_obj_set_style_radius(s_prov_glow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_prov_glow, color_hex(0xFF6D00), 0);
    lv_obj_set_style_bg_opa(s_prov_glow, LV_OPA_40, 0);
    lv_obj_align(s_prov_glow, LV_ALIGN_CENTER, 0, compact ? -22 : -28);
    prov_breath_anim(s_prov_glow, LV_OPA_30, LV_OPA_COVER);

    lv_obj_t *title = lv_label_create(s_prov_root);
    lv_label_set_text(title, "Wi-Fi Setup");
    lv_obj_set_style_text_color(title, color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(title, title_font, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, compact ? 6 : 10);

    lv_obj_t *status = lv_label_create(s_prov_root);
    lv_label_set_text(status, "Provisioning...");
    lv_obj_set_style_text_color(status, color_hex(0x9E9E9E), 0);
    lv_obj_set_style_text_font(status, body_font, 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, compact ? 22 : 32);

    char ap_line[64];
    snprintf(ap_line, sizeof(ap_line), "Hotspot: %s", ap_ssid && ap_ssid[0] ? ap_ssid : "--");
    lv_obj_t *ap_lbl = lv_label_create(s_prov_root);
    lv_label_set_text(ap_lbl, ap_line);
    lv_obj_set_style_text_color(ap_lbl, color_hex(0xFF9100), 0);
    lv_obj_set_style_text_font(ap_lbl, body_font, 0);
    lv_obj_set_width(ap_lbl, w - 8);
    lv_label_set_long_mode(ap_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ap_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ap_lbl, LV_ALIGN_CENTER, 0, compact ? 38 : 52);

    char url_line[48];
    snprintf(url_line, sizeof(url_line), "%s", web_url && web_url[0] ? web_url : "http://192.168.4.1");
    lv_obj_t *url_lbl = lv_label_create(s_prov_root);
    lv_label_set_text(url_lbl, url_line);
    lv_obj_set_style_text_color(url_lbl, color_hex(0x757575), 0);
    lv_obj_set_style_text_font(url_lbl, body_font, 0);
    lv_obj_align(url_lbl, LV_ALIGN_BOTTOM_MID, 0, compact ? -4 : -8);
}

void hrv_ui_init(void)
{
    hrv_ui_provisioning_end();

    display_info_t info = {};
    display_get_info(&info);
    s_screen_w = info.width > 0 ? info.width : s_screen_w;
    s_screen_h = info.height > 0 ? info.height : s_screen_h;
    configure_layout_metrics();

    const int content_w = s_screen_w - 2 * s_safe_inset;
    const int content_h = s_screen_h - 2 * s_safe_inset;
    const int panel_x = (content_w - s_panel_w) / 2;
    const int avail_h = content_h - s_top_bar_h - s_bottom_bar_h;
    const int panel_y = s_top_bar_h + (avail_h - s_panel_h) / 2;
    const lv_font_t *bar_font = s_ui_compact ? &lv_font_montserrat_10 : &lv_font_montserrat_12;

    s_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, s_screen_w, s_screen_h);
    lv_obj_set_style_bg_color(s_root, color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = lv_obj_create(s_root);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, content_w, content_h);
    lv_obj_set_pos(content, s_safe_inset, s_safe_inset);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    if (s_ui_compact) {
        s_lbl_top_scroll = lv_label_create(content);
        style_scroll_label(s_lbl_top_scroll, 0xB0B0B0, bar_font, content_w);
        lv_obj_align(s_lbl_top_scroll, LV_ALIGN_TOP_MID, 0, 0);

        s_lbl_bottom_scroll = lv_label_create(content);
        style_scroll_label(s_lbl_bottom_scroll, 0xE0E0E0, bar_font, content_w);
        lv_obj_align(s_lbl_bottom_scroll, LV_ALIGN_BOTTOM_MID, 0, 0);

        s_bottom_timer = lv_timer_create(bottom_timer_cb, 3500, nullptr);
    } else {
        s_lbl_weather = lv_label_create(content);
        lv_obj_set_style_text_color(s_lbl_weather, color_hex(0x9E9E9E), 0);
        lv_obj_set_style_text_font(s_lbl_weather, bar_font, 0);
        lv_obj_set_width(s_lbl_weather, content_w * 55 / 100);
        lv_label_set_long_mode(s_lbl_weather, LV_LABEL_LONG_DOT);
        lv_obj_align(s_lbl_weather, LV_ALIGN_TOP_LEFT, 10, 0);

        s_lbl_city = lv_label_create(content);
        lv_obj_set_style_text_color(s_lbl_city, color_hex(0x757575), 0);
        lv_obj_set_style_text_font(s_lbl_city, bar_font, 0);
        lv_obj_set_width(s_lbl_city, content_w * 45 / 100 - 10);
        lv_label_set_long_mode(s_lbl_city, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(s_lbl_city, LV_TEXT_ALIGN_RIGHT, 0);
        lv_label_set_text(s_lbl_city, "--");
        lv_obj_align(s_lbl_city, LV_ALIGN_TOP_RIGHT, -10, 0);

        s_lbl_hrv = lv_label_create(content);
        lv_obj_set_style_text_color(s_lbl_hrv, color_hex(0xB0B0B0), 0);
        lv_obj_set_style_text_font(s_lbl_hrv, bar_font, 0);
        lv_obj_align(s_lbl_hrv, LV_ALIGN_BOTTOM_LEFT, 10, 0);

        s_lbl_time = lv_label_create(content);
        lv_obj_set_style_text_color(s_lbl_time, color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_font(s_lbl_time, bar_font, 0);
        lv_obj_set_width(s_lbl_time, content_w * 70 / 100);
        lv_label_set_long_mode(s_lbl_time, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(s_lbl_time, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(s_lbl_time, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    }

    if (!s_ui_compact) {
        s_divider = lv_obj_create(content);
        lv_obj_remove_style_all(s_divider);
        lv_obj_set_size(s_divider, content_w, 1);
        lv_obj_set_style_bg_color(s_divider, color_hex(0x303030), 0);
        lv_obj_set_style_bg_opa(s_divider, LV_OPA_COVER, 0);
        lv_obj_align(s_divider, LV_ALIGN_TOP_MID, 0, s_top_bar_h - 2);
    }

    s_flower_root = lv_obj_create(content);
    lv_obj_remove_style_all(s_flower_root);
    lv_obj_set_size(s_flower_root, s_panel_w, s_panel_h);
    lv_obj_set_pos(s_flower_root, panel_x, panel_y);
    lv_obj_set_style_bg_opa(s_flower_root, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s_flower_root, LV_OBJ_FLAG_SCROLLABLE);

    if (!hrv_status_store_has_data()) {
        hrv_status_t boot = {
            .hrv_ms = 0,
            .time_str = "--",
            .temp_c = TEMP_UNSET,
            .weather = "",
            .city = "",
        };
        drawInterface(&boot);
        ESP_LOGI(TAG, "No cached status in NVS (first run placeholder)");
    } else {
        hrv_ui_restore_from_nvs();
    }

    ESP_LOGI(TAG, "UI mode: %s (%dx%d)", s_ui_compact ? "compact" : "standard", s_screen_w, s_screen_h);
}

void hrv_ui_restore_from_nvs(void)
{
    if (!s_root) {
        return;
    }

    char json[HRV_STATUS_JSON_MAX_LEN];
    size_t len = 0;
    if (!hrv_status_store_load(json, sizeof(json), &len)) {
        ESP_LOGD(TAG, "No status to restore from NVS");
        return;
    }

    hrv_status_t status = {};
    if (!hrv_parse_status_json(json, len, &status)) {
        ESP_LOGW(TAG, "Cached NVS JSON invalid, ignored");
        return;
    }

    drawInterface(&status);
    ESP_LOGI(TAG, "Restored UI from NVS (%u bytes)", (unsigned)len);
}

bool hrv_ui_apply_payload(const char *json, size_t len)
{
    if (!json || len == 0 || !s_root) {
        return false;
    }

    hrv_status_t status = {};
    if (!hrv_parse_status_json(json, len, &status)) {
        return false;
    }

    hrv_status_store_save(json, len);
    drawInterface(&status);
    return true;
}

void drawInterface(const hrv_status_t *status)
{
    if (!status || !s_root) {
        return;
    }

    s_last_status = *status;
    if (s_ui_compact) {
        update_compact_top(status);
        update_compact_bottom(status);
    } else {
        update_standard_labels(status);
    }

    clear_flower_children();
    draw_flower_stage(hrv_to_stage(status->hrv_ms));
}

static void parse_status_fields(cJSON *root, hrv_status_t *out, bool *ok)
{
    cJSON *hrv = cJSON_GetObjectItem(root, "hrv");
    cJSON *time = cJSON_GetObjectItem(root, "time");
    cJSON *temp = cJSON_GetObjectItem(root, "temp");
    cJSON *weather = cJSON_GetObjectItem(root, "weather");
    cJSON *city = cJSON_GetObjectItem(root, "city");

    if (cJSON_IsNumber(hrv)) {
        out->hrv_ms = hrv->valueint;
        *ok = true;
    }
    if (cJSON_IsNumber(temp)) {
        out->temp_c = temp->valueint;
    }
    if (cJSON_IsString(time)) {
        strncpy(out->time_str, time->valuestring, sizeof(out->time_str) - 1);
        out->time_str[sizeof(out->time_str) - 1] = '\0';
    }
    if (cJSON_IsString(weather)) {
        strncpy(out->weather, weather->valuestring, sizeof(out->weather) - 1);
        out->weather[sizeof(out->weather) - 1] = '\0';
    }
    if (cJSON_IsString(city)) {
        strncpy(out->city, city->valuestring, sizeof(out->city) - 1);
        out->city[sizeof(out->city) - 1] = '\0';
    }
}

bool hrv_parse_status_json(const char *json, size_t len, hrv_status_t *out)
{
    if (!json || !out) {
        return false;
    }

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) {
        ESP_LOGW(TAG, "JSON parse failed");
        return false;
    }

    out->temp_c = TEMP_UNSET;

    bool ok = false;
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsString(type) && strcmp(type->valuestring, "status") == 0) {
        parse_status_fields(root, out, &ok);
    } else {
        parse_status_fields(root, out, &ok);
    }

    cJSON_Delete(root);
    return ok;
}
