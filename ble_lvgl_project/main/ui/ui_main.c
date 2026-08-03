#include "ui_main.h"
#include "lcd_bsp.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UI";

static lv_obj_t *s_screen_main = NULL;
static lv_obj_t *s_screen_list = NULL;
static lv_obj_t *s_screen_data = NULL;

static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_scan_list = NULL;
static lv_obj_t *s_data_textarea = NULL;
static lv_obj_t *s_connect_btn = NULL;
static lv_obj_t *s_disconnect_btn = NULL;
static lv_obj_t *s_scan_btn = NULL;

static uint16_t s_selected_device = 0;
static ble_device_info_t s_device_list[BLE_SCAN_RESULT_MAX];
static uint16_t s_device_count = 0;

static lv_style_t s_btn_style;
static lv_style_t s_label_style;
static lv_style_t s_title_style;

static void event_scan_btn_cb(lv_event_t *e);
static void event_connect_btn_cb(lv_event_t *e);
static void event_disconnect_btn_cb(lv_event_t *e);
static void event_back_btn_cb(lv_event_t *e);
static void event_list_item_cb(lv_event_t *e);
static void event_clear_data_cb(lv_event_t *e);

static void create_main_screen(void);
static void create_list_screen(void);
static void create_data_screen(void);

void ui_init(void)
{
    lv_disp_t *disp = get_display();
    if (!disp) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }

    lv_style_init(&s_btn_style);
    lv_style_set_bg_color(&s_btn_style, lv_color_hex(0x007AFF));
    lv_style_set_radius(&s_btn_style, 8);
    lv_style_set_text_color(&s_btn_style, lv_color_white());
    lv_style_set_pad_all(&s_btn_style, 12);

    lv_style_init(&s_label_style);
    lv_style_set_text_color(&s_label_style, lv_color_hex(0x333333));
    lv_style_set_text_font(&s_label_style, &lv_font_montserrat_14);

    lv_style_init(&s_title_style);
    lv_style_set_text_color(&s_title_style, lv_color_hex(0x007AFF));
    lv_style_set_text_font(&s_title_style, &lv_font_montserrat_24);

    create_main_screen();
    create_list_screen();
    create_data_screen();

    lv_scr_load(s_screen_main);
}

static void create_main_screen(void)
{
    s_screen_main = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_main, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_main, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_main);
    lv_label_set_text(title, "BLE Serial");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    s_scan_btn = lv_btn_create(s_screen_main);
    lv_obj_add_style(s_scan_btn, &s_btn_style, 0);
    lv_obj_set_size(s_scan_btn, 200, 50);
    lv_obj_align(s_scan_btn, LV_ALIGN_CENTER, 0, -40);
    lv_label_set_text(lv_obj_get_child(s_scan_btn, 0), "Scan Devices");
    lv_obj_add_event_cb(s_scan_btn, event_scan_btn_cb, LV_EVENT_CLICKED, NULL);

    s_status_label = lv_label_create(s_screen_main);
    lv_label_set_text(s_status_label, "Ready");
    lv_obj_add_style(s_status_label, &s_label_style, 0);
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, 30);
}

static void create_list_screen(void)
{
    s_screen_list = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_list, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_list, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_list);
    lv_label_set_text(title, "Devices");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *back_btn = lv_btn_create(s_screen_list);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 15);
    lv_label_set_text(lv_obj_get_child(back_btn, 0), "Back");
    lv_obj_add_event_cb(back_btn, event_back_btn_cb, LV_EVENT_CLICKED, NULL);

    s_scan_list = lv_list_create(s_screen_list);
    lv_obj_set_size(s_scan_list, EXAMPLE_LCD_H_RES - 20, 300);
    lv_obj_align(s_scan_list, LV_ALIGN_TOP_MID, 0, 70);

    s_connect_btn = lv_btn_create(s_screen_list);
    lv_obj_add_style(s_connect_btn, &s_btn_style, 0);
    lv_obj_set_size(s_connect_btn, 200, 50);
    lv_obj_align(s_connect_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_label_set_text(lv_obj_get_child(s_connect_btn, 0), "Connect");
    lv_obj_add_event_cb(s_connect_btn, event_connect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
}

static void create_data_screen(void)
{
    s_screen_data = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_data, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_data, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_data);
    lv_label_set_text(title, "BLE Data");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *back_btn = lv_btn_create(s_screen_data);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_obj_get_child(back_btn, 0), "Back");
    lv_obj_add_event_cb(back_btn, event_back_btn_cb, LV_EVENT_CLICKED, NULL);

    s_data_textarea = lv_textarea_create(s_screen_data);
    lv_obj_set_size(s_data_textarea, EXAMPLE_LCD_H_RES - 20, 320);
    lv_obj_align(s_data_textarea, LV_ALIGN_TOP_MID, 0, 65);
    lv_textarea_set_placeholder_text(s_data_textarea, "Waiting for data...");
    lv_textarea_set_text(s_data_textarea, "");

    s_disconnect_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(s_disconnect_btn, &s_btn_style, 0);
    lv_obj_set_size(s_disconnect_btn, 140, 45);
    lv_obj_align(s_disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 15, -15);
    lv_label_set_text(lv_obj_get_child(s_disconnect_btn, 0), "Disconnect");
    lv_obj_add_event_cb(s_disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 100, 45);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
    lv_label_set_text(lv_obj_get_child(clear_btn, 0), "Clear");
    lv_obj_add_event_cb(clear_btn, event_clear_data_cb, LV_EVENT_CLICKED, NULL);
}

void ui_switch_screen(ui_screen_t screen)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTake(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    switch (screen) {
    case UI_SCREEN_MAIN:
        lv_scr_load(s_screen_main);
        break;
    case UI_SCREEN_LIST:
        lv_scr_load(s_screen_list);
        break;
    case UI_SCREEN_DATA:
        lv_scr_load(s_screen_data);
        break;
    }

    xSemaphoreGive(mux);
}

void ui_update_state(ble_state_t state, const char *message)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTake(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_status_label) {
        lv_label_set_text(s_status_label, message);
    }

    switch (state) {
    case BLE_STATE_SCANNING:
        if (s_scan_btn) {
            lv_label_set_text(lv_obj_get_child(s_scan_btn, 0), "Scanning...");
            lv_obj_add_state(s_scan_btn, LV_OBJ_FLAG_DISABLED);
        }
        break;
    case BLE_STATE_IDLE:
        if (s_scan_btn) {
            lv_label_set_text(lv_obj_get_child(s_scan_btn, 0), "Scan Devices");
            lv_obj_remove_state(s_scan_btn, LV_OBJ_FLAG_DISABLED);
        }
        break;
    case BLE_STATE_CONNECTED:
        ui_switch_screen(UI_SCREEN_DATA);
        break;
    case BLE_STATE_DISCONNECTED:
        ui_switch_screen(UI_SCREEN_MAIN);
        break;
    default:
        break;
    }

    xSemaphoreGive(mux);
}

void ui_update_scan_results(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTake(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_scan_list) {
        lv_obj_clean(s_scan_list);

        ble_manager_get_scan_results(s_device_list, &s_device_count);

        for (uint16_t i = 0; i < s_device_count; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s  (RSSI: %d)",
                     s_device_list[i].name, s_device_list[i].rssi);

            lv_obj_t *btn = lv_list_add_btn(s_scan_list, LV_SYMBOL_WIFI, buf);
            lv_obj_set_user_data(btn, (void *)(intptr_t)i);
            lv_obj_add_event_cb(btn, event_list_item_cb, LV_EVENT_CLICKED, NULL);
        }

        if (s_device_count > 0) {
            lv_obj_remove_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    xSemaphoreGive(mux);
}

void ui_append_data(const uint8_t *data, uint16_t len)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTake(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_data_textarea && len > 0) {
        char text[256];
        uint16_t copy_len = (len < sizeof(text) - 1) ? len : sizeof(text) - 1;
        memcpy(text, data, copy_len);
        text[copy_len] = '\0';

        const char *current = lv_textarea_get_text(s_data_textarea);
        static char combined[4096];
        uint16_t current_len = strlen(current);
        uint16_t new_len = strlen(text);

        if (current_len + new_len < sizeof(combined) - 1) {
            strcpy(combined, current);
            strcat(combined, text);
            lv_textarea_set_text(s_data_textarea, combined);
        } else {
            memmove(combined, combined + new_len, current_len - new_len + 1);
            memcpy(combined, text, new_len + 1);
            lv_textarea_set_text(s_data_textarea, combined);
        }
    }

    xSemaphoreGive(mux);
}

void ui_clear_data(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTake(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_data_textarea) {
        lv_textarea_set_text(s_data_textarea, "");
    }

    xSemaphoreGive(mux);
}

static void event_scan_btn_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_LIST);
    ble_manager_start_scan();
}

static void event_connect_btn_cb(lv_event_t *e)
{
    if (s_selected_device < s_device_count) {
        ble_manager_connect(s_device_list[s_selected_device].mac);
    }
}

static void event_disconnect_btn_cb(lv_event_t *e)
{
    ble_manager_disconnect();
}

static void event_back_btn_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_MAIN);
}

static void event_list_item_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    s_selected_device = (uint16_t)(intptr_t)lv_obj_get_user_data(btn);

    for (uint16_t i = 0; i < s_device_count; i++) {
        lv_obj_t *child = lv_obj_get_child(s_scan_list, i);
        if (child) {
            lv_obj_clear_flag(child, LV_OBJ_FLAG_CHECKABLE);
        }
    }
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
}

static void event_clear_data_cb(lv_event_t *e)
{
    ui_clear_data();
}
