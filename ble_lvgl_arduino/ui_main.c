#include "ui_main.h"
#include "lcd_bsp.h"
#include "lcd_config.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UI";

static lv_obj_t *s_screen_main = NULL;
static lv_obj_t *s_screen_list = NULL;
static lv_obj_t *s_screen_data = NULL;

static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_scan_list = NULL;
static lv_obj_t *s_connect_btn = NULL;
static lv_obj_t *s_disconnect_btn = NULL;
static lv_obj_t *s_scan_btn = NULL;

/* UUID 设置屏：连接前让用户指定 服务/通知/写 特征 UUID */
static lv_obj_t *s_screen_uuid = NULL;
static lv_obj_t *s_ta_service = NULL;   /* 服务 UUID textarea，默认 FFF0 */
static lv_obj_t *s_ta_notify = NULL;    /* 通知特征 UUID textarea，默认 FFF1 */
static lv_obj_t *s_ta_write = NULL;    /* 写特征 UUID textarea，默认 FFF2 */
static lv_obj_t *s_active_ta = NULL;   /* 当前十六进制键盘输入目标 */

/* 数据屏：11 通道数值网格 + 原始帧 + 连接状态 */
static lv_obj_t *s_data_status_label = NULL;   /* 显示已连接设备名 */
static lv_obj_t *s_raw_label = NULL;           /* 最新一帧原始字符串 */
static lv_obj_t *s_value_grid = NULL;          /* 11 通道网格容器 */
typedef struct {
    lv_obj_t *title;
    lv_obj_t *value;
} value_cell_t;
static value_cell_t s_cells[BLE_DATA_VALUE_COUNT];

static uint16_t s_selected_device = 0;
static ble_device_info_t s_device_list[BLE_SCAN_RESULT_MAX];
static uint16_t s_device_count = 0;
static char s_connected_name[32] = "";   /* 当前已连接设备名，用于数据屏显示 */

static lv_style_t s_btn_style;
static lv_style_t s_label_style;
static lv_style_t s_title_style;
static lv_style_t s_list_item_selected_style;  /* 设备列表项选中样式：蓝底白字 */

static void event_scan_btn_cb(lv_event_t *e);
static void event_connect_btn_cb(lv_event_t *e);
static void event_disconnect_btn_cb(lv_event_t *e);
static void event_back_btn_cb(lv_event_t *e);
static void event_list_item_cb(lv_event_t *e);
static void event_clear_data_cb(lv_event_t *e);
static void event_uuid_connect_cb(lv_event_t *e);
static void event_uuid_cancel_cb(lv_event_t *e);
static void event_uuid_keypad_cb(lv_event_t *e);
static void event_uuid_field_click_cb(lv_event_t *e);

static void create_main_screen(void);
static void create_list_screen(void);
static void create_data_screen(void);
static void create_uuid_screen(void);

void ui_init(void)
{
    lv_disp_t *disp = get_display();
    if (!disp) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }

    lv_style_init(&s_btn_style);
    lv_style_init(&s_label_style);
    lv_style_init(&s_title_style);
    lv_style_init(&s_list_item_selected_style);

    lv_style_set_bg_color(&s_btn_style, lv_color_hex(0x007AFF));
    lv_style_set_radius(&s_btn_style, 8);
    lv_style_set_text_color(&s_btn_style, lv_color_white());
    lv_style_set_pad_all(&s_btn_style, 12);

    lv_style_set_text_color(&s_label_style, lv_color_hex(0x333333));
    lv_style_set_text_font(&s_label_style, &lv_font_montserrat_14);

    lv_style_set_text_color(&s_title_style, lv_color_hex(0x007AFF));
    lv_style_set_text_font(&s_title_style, &lv_font_montserrat_24);

    /* 设备列表项选中样式：蓝底白字（仅在 LV_STATE_CHECKED 状态下生效） */
    lv_style_set_bg_color(&s_list_item_selected_style, lv_color_hex(0x007AFF));
    lv_style_set_bg_opa(&s_list_item_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_list_item_selected_style, lv_color_white());
    lv_style_set_border_color(&s_list_item_selected_style, lv_color_hex(0x0051D5));
    lv_style_set_border_width(&s_list_item_selected_style, 2);
    lv_style_set_radius(&s_list_item_selected_style, 6);

    create_main_screen();
    create_list_screen();
    create_data_screen();
    create_uuid_screen();

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
    lv_obj_t *scan_label = lv_label_create(s_scan_btn);
    lv_label_set_text(scan_label, "Scan Devices");
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
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_add_event_cb(back_btn, event_back_btn_cb, LV_EVENT_CLICKED, NULL);

    s_scan_list = lv_list_create(s_screen_list);
    lv_obj_set_size(s_scan_list, EXAMPLE_LCD_H_RES - 20, 300);
    lv_obj_align(s_scan_list, LV_ALIGN_TOP_MID, 0, 70);

    s_connect_btn = lv_btn_create(s_screen_list);
    lv_obj_add_style(s_connect_btn, &s_btn_style, 0);
    lv_obj_set_size(s_connect_btn, 200, 50);
    lv_obj_align(s_connect_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t *connect_label = lv_label_create(s_connect_btn);
    lv_label_set_text(connect_label, "Connect");
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
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *back_btn = lv_btn_create(s_screen_data);
    lv_obj_set_size(back_btn, 56, 36);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_add_event_cb(back_btn, event_back_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 连接状态行：显示已连接的设备名 */
    s_data_status_label = lv_label_create(s_screen_data);
    lv_label_set_text(s_data_status_label, "Connecting...");
    lv_obj_add_style(s_data_status_label, &s_label_style, 0);
    lv_obj_set_style_text_color(s_data_status_label, lv_color_hex(0x34C759), 0);
    lv_obj_align(s_data_status_label, LV_ALIGN_TOP_MID, 0, 52);

    /* 11 通道数值网格：2 列手动定位，每格显示 "ChN" + 4 位数值。
     * 不依赖 LV_USE_FLEX，兼容性更好。 */
    s_value_grid = lv_obj_create(s_screen_data);
    lv_obj_set_size(s_value_grid, EXAMPLE_LCD_H_RES - 12, 282);
    lv_obj_align(s_value_grid, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_pad_all(s_value_grid, 4, 0);
    lv_obj_set_style_border_width(s_value_grid, 0, 0);
    lv_obj_set_style_bg_opa(s_value_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(s_value_grid, LV_SCROLLBAR_MODE_OFF);

    for (uint8_t i = 0; i < BLE_DATA_VALUE_COUNT; i++) {
        uint8_t col = i % 2;       /* 0=左列, 1=右列 */
        uint8_t row = i / 2;       /* 0..5 */
        lv_coord_t x = col ? 136 : 4;
        lv_coord_t y = 4 + row * 46;

        lv_obj_t *cell = lv_obj_create(s_value_grid);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, 124, 42);
        lv_obj_set_style_bg_color(cell, lv_color_hex(0xF2F2F7), 0);
        lv_obj_set_style_radius(cell, 6, 0);
        lv_obj_set_style_pad_all(cell, 3, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *t = lv_label_create(cell);
        lv_label_set_text_fmt(t, "Ch%d", i + 1);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 4, 2);

        lv_obj_t *v = lv_label_create(cell);
        lv_label_set_text(v, "----");
        lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(v, lv_color_hex(0x007AFF), 0);
        lv_obj_align(v, LV_ALIGN_BOTTOM_RIGHT, -4, -2);

        s_cells[i].title = t;
        s_cells[i].value = v;
    }

    /* 最新一帧原始数据（调试用，单行省略） */
    s_raw_label = lv_label_create(s_screen_data);
    lv_label_set_text(s_raw_label, "Raw: --");
    lv_obj_add_style(s_raw_label, &s_label_style, 0);
    lv_obj_set_style_text_color(s_raw_label, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_width(s_raw_label, EXAMPLE_LCD_H_RES - 16);
    lv_label_set_long_mode(s_raw_label, LV_LABEL_LONG_DOT);
    lv_obj_align(s_raw_label, LV_ALIGN_TOP_MID, 0, 368);

    s_disconnect_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(s_disconnect_btn, &s_btn_style, 0);
    lv_obj_set_size(s_disconnect_btn, 130, 42);
    lv_obj_align(s_disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t *disconnect_label = lv_label_create(s_disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_add_event_cb(s_disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 90, 42);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_add_event_cb(clear_btn, event_clear_data_cb, LV_EVENT_CLICKED, NULL);
}

/* UUID 设置屏：连接前让用户设置 服务/通知/写 UUID。
 * 布局（280x456，内边距 8）：标题 / 3 字段 / 十六进制键盘 / 取消+连接。
 * LV_USE_KEYBOARD=0，故自建 btnmatrix 十六进制键盘（0-F + DEL/CLR）。 */
static void create_uuid_screen(void)
{
    s_screen_uuid = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_uuid, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_uuid, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(s_screen_uuid, 8, 0);

    lv_obj_t *title = lv_label_create(s_screen_uuid);
    lv_label_set_text(title, "UUID Settings");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    /* 3 个 UUID 字段：标签 + 单行 textarea，最大 8 字符（16/32 位 hex） */
    static const char *labels[3]   = {"Service", "Notify", "Write"};
    /* 目标手机 GATT（nRF Connect 实测）：
     *   Service: 0000FFF0-0000-1000-8000-00805F9B34FB
     *   Notify : 0000FFF1-0000-1000-8000-00805F9B34FB
     *   Write  : 0000FFF2-0000-1000-8000-00805F9B34FB
     * NimBLEUUID(std::string) 会把 16 位扩展到 128 位标准基比较，
     * 所以这里直接写 16 位短码即可匹配完整 128 位 UUID。 */
    static const char *defaults[3] = {"FFF0", "FFF1", "FFF2"};
    lv_obj_t **tas[3] = {&s_ta_service, &s_ta_notify, &s_ta_write};
    for (int i = 0; i < 3; i++) {
        lv_coord_t y = 40 + i * 42;

        lv_obj_t *lbl = lv_label_create(s_screen_uuid);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_add_style(lbl, &s_label_style, 0);
        lv_obj_set_pos(lbl, 0, y + 6);

        lv_obj_t *ta = lv_textarea_create(s_screen_uuid);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_max_length(ta, 8);
        lv_textarea_set_text(ta, defaults[i]);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, 0);
        lv_obj_set_style_border_color(ta, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(ta, 2, 0);
        lv_obj_set_style_radius(ta, 6, 0);
        lv_obj_set_pos(ta, 66, y);
        lv_obj_set_size(ta, 198, 32);
        lv_obj_add_event_cb(ta, event_uuid_field_click_cb, LV_EVENT_CLICKED, NULL);
        *tas[i] = ta;
    }
    /* 默认激活服务字段：蓝色边框高亮 */
    s_active_ta = s_ta_service;
    lv_obj_set_style_border_color(s_ta_service, lv_color_hex(0x007AFF), 0);

    /* 十六进制键盘：4 列 x 5 行，最后一行 DEL/CLR 各跨 2 列 */
    static const char *kb_map[] = {
        "1", "2", "3", "4", "\n",
        "5", "6", "7", "8", "\n",
        "9", "0", "A", "B", "\n",
        "C", "D", "E", "F", "\n",
        "DEL", "CLR", ""
    };
    lv_obj_t *kb = lv_btnmatrix_create(s_screen_uuid);
    lv_btnmatrix_set_map(kb, kb_map);
    lv_btnmatrix_set_btn_width(kb, 16, 2);   /* DEL 跨 2 列 */
    lv_btnmatrix_set_btn_width(kb, 17, 2);   /* CLR 跨 2 列 */
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xF2F2F7), 0);
    lv_obj_set_style_pad_all(kb, 4, 0);
    lv_obj_set_style_pad_gap(kb, 4, 0);
    lv_obj_set_pos(kb, 0, 174);
    lv_obj_set_size(kb, 264, 188);
    lv_obj_add_event_cb(kb, event_uuid_keypad_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 取消 / 连接 按钮，各占一半宽度 */
    lv_coord_t half_w = (264 - 8) / 2;
    lv_obj_t *cancel_btn = lv_btn_create(s_screen_uuid);
    lv_obj_add_style(cancel_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(cancel_btn, half_w, 44);
    lv_obj_set_pos(cancel_btn, 0, 372);
    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_center(cancel_lbl);
    lv_obj_add_event_cb(cancel_btn, event_uuid_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *connect_btn = lv_btn_create(s_screen_uuid);
    lv_obj_add_style(connect_btn, &s_btn_style, 0);
    lv_obj_set_size(connect_btn, half_w, 44);
    lv_obj_set_pos(connect_btn, half_w + 8, 372);
    lv_obj_t *connect_lbl = lv_label_create(connect_btn);
    lv_label_set_text(connect_lbl, "Connect");
    lv_obj_center(connect_lbl);
    lv_obj_add_event_cb(connect_btn, event_uuid_connect_cb, LV_EVENT_CLICKED, NULL);
}

/* 点击某个 UUID 字段：设为当前键盘输入目标，蓝色边框高亮 */
static void event_uuid_field_click_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_current_target(e);
    s_active_ta = ta;
    /* 激活字段用蓝色边框高亮，其余灰色（本版本无 set_cursor_visible，
     * 边框颜色即唯一激活指示） */
    lv_obj_set_style_border_color(s_ta_service,
        ta == s_ta_service ? lv_color_hex(0x007AFF) : lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_color(s_ta_notify,
        ta == s_ta_notify ? lv_color_hex(0x007AFF) : lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_color(s_ta_write,
        ta == s_ta_write ? lv_color_hex(0x007AFF) : lv_color_hex(0xCCCCCC), 0);
}

/* 十六进制键盘按键：向当前 textarea 追加字符 / 退格 / 清空 */
static void event_uuid_keypad_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(btnm);
    if (id == LV_BTNMATRIX_BTN_NONE || !s_active_ta) return;
    const char *txt = lv_btnmatrix_get_btn_text(btnm, id);
    if (!txt) return;
    if (strcmp(txt, "DEL") == 0) {
        lv_textarea_del_char(s_active_ta);
    } else if (strcmp(txt, "CLR") == 0) {
        lv_textarea_set_text(s_active_ta, "");
    } else {
        lv_textarea_add_char(s_active_ta, txt[0]);
    }
}

static void event_uuid_cancel_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_LIST);
}

/* 读取 3 个 UUID，校验非空后发起带 UUID 的连接，并切到数据屏等连接结果 */
static void event_uuid_connect_cb(lv_event_t *e)
{
    if (s_selected_device >= s_device_count) {
        ESP_LOGW(TAG, "No device selected, cannot connect");
        return;
    }
    const char *svc    = lv_textarea_get_text(s_ta_service);
    const char *notify = lv_textarea_get_text(s_ta_notify);
    const char *write  = lv_textarea_get_text(s_ta_write);
    if (!svc || !notify || !write ||
        svc[0] == '\0' || notify[0] == '\0' || write[0] == '\0') {
        ESP_LOGW(TAG, "UUID fields must not be empty");
        return;
    }

    ble_uuid_config_t uuids;
    strncpy(uuids.service, svc, sizeof(uuids.service) - 1);
    strncpy(uuids.notify, notify, sizeof(uuids.notify) - 1);
    strncpy(uuids.write, write, sizeof(uuids.write) - 1);
    uuids.service[sizeof(uuids.service) - 1] = '\0';
    uuids.notify[sizeof(uuids.notify) - 1] = '\0';
    uuids.write[sizeof(uuids.write) - 1] = '\0';

    /* 保存设备名供数据屏连接成功后显示 */
    strncpy(s_connected_name, s_device_list[s_selected_device].name,
            sizeof(s_connected_name) - 1);
    s_connected_name[sizeof(s_connected_name) - 1] = '\0';

    /* 切到数据屏显示 "Connecting..."，再发起连接（connect_task 异步执行） */
    if (s_data_status_label) {
        lv_label_set_text(s_data_status_label, "Connecting...");
    }
    ui_switch_screen(UI_SCREEN_DATA);
    ble_manager_connect(&s_device_list[s_selected_device], &uuids);
}

void ui_switch_screen(ui_screen_t screen)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    switch (screen) {
    case UI_SCREEN_MAIN:
        lv_scr_load(s_screen_main);
        break;
    case UI_SCREEN_LIST:
        lv_scr_load(s_screen_list);
        break;
    case UI_SCREEN_UUID:
        lv_scr_load(s_screen_uuid);
        break;
    case UI_SCREEN_DATA:
        lv_scr_load(s_screen_data);
        break;
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_update_state(ble_state_t state, const char *message)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_status_label) {
        lv_label_set_text(s_status_label, message);
    }

    switch (state) {
    case BLE_STATE_SCANNING:
        if (s_scan_btn) {
            lv_obj_t *lbl = lv_obj_get_child(s_scan_btn, 0);
            if (lbl) lv_label_set_text(lbl, "Scanning...");
            lv_obj_add_state(s_scan_btn, LV_STATE_DISABLED);
        }
        break;
    case BLE_STATE_IDLE:
        if (s_scan_btn) {
            lv_obj_t *lbl = lv_obj_get_child(s_scan_btn, 0);
            if (lbl) lv_label_set_text(lbl, "Scan Devices");
            lv_obj_clear_state(s_scan_btn, LV_STATE_DISABLED);
        }
        break;
    case BLE_STATE_CONNECTING:
        if (s_data_status_label) {
            lv_label_set_text(s_data_status_label, "Connecting...");
        }
        break;
    case BLE_STATE_CONNECTED:
        if (s_data_status_label) {
            char buf[48];
            snprintf(buf, sizeof(buf), "Connected: %s",
                     s_connected_name[0] ? s_connected_name : "device");
            lv_label_set_text(s_data_status_label, buf);
        }
        ui_clear_data();
        ui_switch_screen(UI_SCREEN_DATA);
        break;
    case BLE_STATE_DISCONNECTED:
        s_connected_name[0] = '\0';
        if (s_data_status_label) {
            lv_label_set_text(s_data_status_label, "Disconnected");
        }
        ui_switch_screen(UI_SCREEN_MAIN);
        break;
    default:
        break;
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_update_scan_results(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) {
        ESP_LOGW(TAG, "LVGL mutex not available, skipping UI update");
        return;
    }

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to take LVGL mutex");
        return;
    }

    if (s_scan_list) {
        lv_obj_clean(s_scan_list);

        ble_manager_get_scan_results(s_device_list, &s_device_count);

        ESP_LOGI(TAG, "Updating scan results: %d devices", s_device_count);

        for (uint16_t i = 0; i < s_device_count; i++) {
            char buf[128];
            if (s_device_list[i].has_custom_id) {
                /* 优先显示自定义 UUID + SN（随机 MAC 场景下的稳定标识） */
                snprintf(buf, sizeof(buf), "%s\nUUID:%08lX SN:%lu  RSSI:%d",
                         s_device_list[i].name,
                         (unsigned long)s_device_list[i].custom_uuid,
                         (unsigned long)s_device_list[i].sn,
                         s_device_list[i].rssi);
            } else {
                /* 无自定义 ID 时回退到 MAC 地址 */
                snprintf(buf, sizeof(buf), "%s\n%02X:%02X:%02X:%02X:%02X:%02X  RSSI: %d",
                         s_device_list[i].name,
                         s_device_list[i].mac[0], s_device_list[i].mac[1], s_device_list[i].mac[2],
                         s_device_list[i].mac[3], s_device_list[i].mac[4], s_device_list[i].mac[5],
                         s_device_list[i].rssi);
            }

            lv_obj_t *btn = lv_list_add_btn(s_scan_list, LV_SYMBOL_WIFI, buf);
            lv_obj_set_user_data(btn, (void *)(intptr_t)i);
            /* 启用可选中标志 + 绑定选中样式（蓝底白字，仅在 LV_STATE_CHECKED 下生效） */
            lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_style(btn, &s_list_item_selected_style, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_add_event_cb(btn, event_list_item_cb, LV_EVENT_CLICKED, NULL);

            ESP_LOGI(TAG, "  Device %d: %s, RSSI=%d", i + 1, s_device_list[i].name, s_device_list[i].rssi);
        }

        if (s_device_count > 0) {
            lv_obj_clear_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Connect button shown");
        } else {
            lv_obj_add_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGW(TAG, "No devices found, connect button hidden");
        }
    } else {
        ESP_LOGW(TAG, "s_scan_list is NULL, cannot update scan results");
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_append_data(const uint8_t *data, uint16_t len)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 仅显示最新一帧原始数据（实时刷新，不累加） */
    if (s_raw_label && len > 0) {
        char buf[128];
        uint16_t copy_len = (len < sizeof(buf) - 8) ? len : sizeof(buf) - 8;
        memcpy(buf, "Raw: ", 5);
        memcpy(buf + 5, data, copy_len);
        buf[5 + copy_len] = '\0';
        lv_label_set_text(s_raw_label, buf);
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_update_data_values(const int16_t *values, uint8_t count)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 用 4 位零填充显示，与源数据 "0920"/"0000" 格式一致 */
    for (uint8_t i = 0; i < count && i < BLE_DATA_VALUE_COUNT; i++) {
        if (s_cells[i].value) {
            lv_label_set_text_fmt(s_cells[i].value, "%04d", values[i]);
        }
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_clear_data(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    for (uint8_t i = 0; i < BLE_DATA_VALUE_COUNT; i++) {
        if (s_cells[i].value) {
            lv_label_set_text(s_cells[i].value, "----");
        }
    }
    if (s_raw_label) {
        lv_label_set_text(s_raw_label, "Raw: --");
    }

    xSemaphoreGiveRecursive(mux);
}

static void event_scan_btn_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_LIST);
    ble_manager_start_scan();
}

static void event_connect_btn_cb(lv_event_t *e)
{
    if (s_selected_device >= s_device_count) {
        return;
    }
    /* 不直接连接：弹出 UUID 设置屏，让用户确认/修改 服务/通知/写 UUID。
     * 每次进入都重置为默认值（FFF0/FFF1/FFF2），并高亮服务字段。 */
    lv_textarea_set_text(s_ta_service, "FFF0");
    lv_textarea_set_text(s_ta_notify, "FFF1");
    lv_textarea_set_text(s_ta_write, "FFF2");
    s_active_ta = s_ta_service;
    lv_obj_set_style_border_color(s_ta_service, lv_color_hex(0x007AFF), 0);
    lv_obj_set_style_border_color(s_ta_notify, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_color(s_ta_write, lv_color_hex(0xCCCCCC), 0);
    ui_switch_screen(UI_SCREEN_UUID);
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

    /* 单选逻辑：清除所有列表项的 CHECKED 状态，仅给当前项添加 CHECKED 状态
     * 触发 s_list_item_selected_style（蓝底白字）生效 */
    for (uint16_t i = 0; i < s_device_count; i++) {
        lv_obj_t *child = lv_obj_get_child(s_scan_list, i);
        if (child && child != btn) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }
    lv_obj_add_state(btn, LV_STATE_CHECKED);
}

static void event_clear_data_cb(lv_event_t *e)
{
    ui_clear_data();
}
