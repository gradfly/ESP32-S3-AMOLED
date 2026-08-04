#include "ui_main.h"
#include "lcd_bsp.h"
#include "lcd_config.h"
#include "pwm_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UI";

static lv_obj_t *s_screen_main = NULL;
static lv_obj_t *s_screen_list = NULL;
static lv_obj_t *s_screen_data = NULL;
static lv_obj_t *s_screen_pwm = NULL;

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

/* 数据屏右上角 "PWM" 跳转按钮 -> 进入 PWM 屏 */
static lv_obj_t *s_data_pwm_btn = NULL;

/* PWM 屏：5 路（CH1~CH5）输入值 + 输出脉宽显示。
 * 布局与数据屏一致（标题 + Back + 状态行 + 2 列网格 + 底部按钮）。 */
static lv_obj_t *s_pwm_status_label = NULL;    /* 顶部状态行：阈值规则提示 */
static lv_obj_t *s_pwm_info_label = NULL;      /* 底部信息行：5 路输出汇总 */
typedef struct {
    lv_obj_t *title;    /* "Ch1" */
    lv_obj_t *input;    /* 输入值 "1800" */
    lv_obj_t *output;   /* 输出 PWM "2000us" */
} pwm_cell_t;
static pwm_cell_t s_pwm_cells[PWM_CHANNEL_COUNT];

static uint16_t s_selected_device = 0;
static ble_device_info_t s_device_list[BLE_SCAN_RESULT_MAX];
static uint16_t s_device_count = 0;
static char s_connected_name[32] = "";   /* 当前已连接设备名，用于数据屏显示 */

static lv_style_t s_btn_style;
static lv_style_t s_label_style;
static lv_style_t s_title_style;
static lv_style_t s_list_item_selected_style;  /* 设备列表项选中样式：蓝底白字 */

/* ====== 屏幕边缘滑动切换 ======
 * 仅对 主屏 / 数据屏 / PWM 屏 启用，线性顺序：Main <-> Data <-> PWM。
 * 左边缘起手右滑 -> 上一屏；右边缘起手左滑 -> 下一屏。
 * 按下需落在屏幕空白区域（非按钮/列表等子对象），否则手势事件不会到达屏幕。 */
#define SWIPE_EDGE_WIDTH    30      /* 距左右边缘 30px 内起手才算边缘滑动 */
#define SWIPE_ANIM_MS       300     /* 切屏滑动动画时长 */
#define SWIPE_DEBOUNCE_MS   400     /* 防抖：两次滑动最小间隔（>动画时长） */
static const ui_screen_t s_swipe_order[] = {UI_SCREEN_MAIN, UI_SCREEN_DATA, UI_SCREEN_PWM};
#define SWIPE_ORDER_LEN  (sizeof(s_swipe_order) / sizeof(s_swipe_order[0]))

static lv_point_t s_swipe_start_pt = {0};
static bool       s_swipe_started  = false;
static uint32_t   s_last_swipe_ms  = 0;

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
static void event_pwm_btn_cb(lv_event_t *e);        /* 数据屏 -> PWM 屏 */
static void event_pwm_back_btn_cb(lv_event_t *e);   /* PWM 屏 -> 数据屏 */
static void event_clear_pwm_cb(lv_event_t *e);      /* PWM 屏 Clear 按钮 */
static void swipe_press_cb(lv_event_t *e);          /* 按下：记录起手坐标 */
static void swipe_gesture_cb(lv_event_t *e);        /* 屏幕级手势：边缘+方向 -> 切屏 */
static void swipe_zone_gesture_cb(lv_event_t *e);   /* 热区手势：边缘热区 -> 切屏 */
static void swipe_do_switch(bool go_next);          /* 执行切屏（带动画+防抖） */
static void setup_screen_swipe(lv_obj_t *screen);   /* 为屏幕绑定边缘滑动事件 */

static void create_main_screen(void);
static void create_list_screen(void);
static void create_data_screen(void);
static void create_uuid_screen(void);
static void create_pwm_screen(void);

/* ====== 边缘滑动切换屏幕：实现 ====== */

/* 执行切屏：确定当前屏在滑动顺序中的位置，带动画加载目标屏。
 * go_next=true -> 下一屏(MOVE_LEFT)，false -> 上一屏(MOVE_RIGHT)。
 * 越界（已到头/尾）则什么都不做。 */
static void swipe_do_switch(bool go_next)
{
    /* 防抖：动画进行中不响应新滑动 */
    uint32_t now = lv_tick_get();
    if (now - s_last_swipe_ms < SWIPE_DEBOUNCE_MS) return;

    lv_obj_t *active = lv_disp_get_scr_act(lv_disp_get_default());
    int cur_idx = -1;
    for (int i = 0; i < (int)SWIPE_ORDER_LEN; i++) {
        lv_obj_t *scr = NULL;
        switch (s_swipe_order[i]) {
        case UI_SCREEN_MAIN: scr = s_screen_main; break;
        case UI_SCREEN_DATA: scr = s_screen_data; break;
        case UI_SCREEN_PWM:  scr = s_screen_pwm;  break;
        default: break;
        }
        if (scr == active) { cur_idx = i; break; }
    }
    if (cur_idx < 0) return;

    int target_idx = go_next ? cur_idx + 1 : cur_idx - 1;
    if (target_idx < 0 || target_idx >= (int)SWIPE_ORDER_LEN) return;  /* 越界：已到头 */

    lv_obj_t *target_scr = NULL;
    switch (s_swipe_order[target_idx]) {
    case UI_SCREEN_MAIN: target_scr = s_screen_main; break;
    case UI_SCREEN_DATA: target_scr = s_screen_data; break;
    case UI_SCREEN_PWM:  target_scr = s_screen_pwm;  break;
    default: return;
    }

    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;
    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    lv_scr_load_anim_t anim = go_next ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                      : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(target_scr, anim, SWIPE_ANIM_MS, 0, false);
    s_last_swipe_ms = now;

    xSemaphoreGiveRecursive(mux);
}

/* 按下空白区域时记录起手坐标（仅屏幕自身接收 PRESSED 才会触发） */
static void swipe_press_cb(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_indev_get_point(indev, &s_swipe_start_pt);
    s_swipe_started = true;
}

/* 屏幕级手势：按下落在屏幕空白区域时触发。
 * 左边缘(x<30) + 右滑 -> 上一屏；右边缘(x>250) + 左滑 -> 下一屏 */
static void swipe_gesture_cb(lv_event_t *e)
{
    if (!s_swipe_started) return;
    s_swipe_started = false;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);

    if (s_swipe_start_pt.x < SWIPE_EDGE_WIDTH && dir == LV_DIR_RIGHT) {
        swipe_do_switch(false);   /* 左边缘右滑 -> 上一屏 */
    } else if (s_swipe_start_pt.x > EXAMPLE_LCD_H_RES - SWIPE_EDGE_WIDTH &&
               dir == LV_DIR_LEFT) {
        swipe_do_switch(true);    /* 右边缘左滑 -> 下一屏 */
    }
}

/* 边缘透明热区手势：热区覆盖在网格等子对象上方，确保网格区域也能边缘滑动。
 * user_data: 0=左边缘热区(右滑->上一屏)，1=右边缘热区(左滑->下一屏) */
static void swipe_zone_gesture_cb(lv_event_t *e)
{
    lv_obj_t *zone = lv_event_get_target(e);
    bool is_right = ((intptr_t)lv_obj_get_user_data(zone)) != 0;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);

    if (!is_right && dir == LV_DIR_RIGHT) {
        swipe_do_switch(false);   /* 左热区右滑 -> 上一屏 */
    } else if (is_right && dir == LV_DIR_LEFT) {
        swipe_do_switch(true);    /* 右热区左滑 -> 下一屏 */
    }
}

/* 为屏幕绑定边缘滑动事件：
 * 1. 屏幕级 PRESSED+GESTURE（空白区域起手的滑动）
 * 2. 两个透明边缘热区覆盖在网格上方（网格区域起手的滑动）
 * 热区仅覆盖中部纵向区域（y=70~360），避开顶部按钮和底部按钮。 */
static void setup_screen_swipe(lv_obj_t *screen)
{
    if (!screen) return;

    /* 屏幕级手势：空白区域起手 */
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, swipe_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, swipe_gesture_cb, LV_EVENT_GESTURE, NULL);

    /* 透明边缘热区：覆盖网格区域，使边缘滑动在整屏均可触发 */
    lv_coord_t zone_y = 70;
    lv_coord_t zone_h = EXAMPLE_LCD_V_RES - 70 - 100;  /* 避开底部按钮区 */

    lv_obj_t *left_zone = lv_obj_create(screen);
    lv_obj_set_size(left_zone, SWIPE_EDGE_WIDTH, zone_h);
    lv_obj_set_pos(left_zone, 0, zone_y);
    lv_obj_set_style_bg_opa(left_zone, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_zone, 0, 0);
    lv_obj_set_style_pad_all(left_zone, 0, 0);
    lv_obj_clear_flag(left_zone, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(left_zone, (void *)(intptr_t)0);   /* 0=左 */
    lv_obj_add_event_cb(left_zone, swipe_zone_gesture_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_t *right_zone = lv_obj_create(screen);
    lv_obj_set_size(right_zone, SWIPE_EDGE_WIDTH, zone_h);
    lv_obj_set_pos(right_zone, EXAMPLE_LCD_H_RES - SWIPE_EDGE_WIDTH, zone_y);
    lv_obj_set_style_bg_opa(right_zone, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_zone, 0, 0);
    lv_obj_set_style_pad_all(right_zone, 0, 0);
    lv_obj_clear_flag(right_zone, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(right_zone, (void *)(intptr_t)1);  /* 1=右 */
    lv_obj_add_event_cb(right_zone, swipe_zone_gesture_cb, LV_EVENT_GESTURE, NULL);
}

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
    create_pwm_screen();

    /* 为主屏 / 数据屏 / PWM 屏绑定边缘滑动切换 */
    setup_screen_swipe(s_screen_main);
    setup_screen_swipe(s_screen_data);
    setup_screen_swipe(s_screen_pwm);

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

    /* 左上角 Back 按钮已移除：改用屏幕边缘滑动切换（左边缘右滑 -> 主屏） */
    /* 右上角 "PWM" 按钮：跳转到 PWM 输出屏（也可右边缘左滑切到 PWM 屏） */
    s_data_pwm_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(s_data_pwm_btn, &s_btn_style, 0);
    lv_obj_set_size(s_data_pwm_btn, 60, 38);
    lv_obj_align(s_data_pwm_btn, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_t *pwm_jump_label = lv_label_create(s_data_pwm_btn);
    lv_label_set_text(pwm_jump_label, "PWM");
    lv_obj_center(pwm_jump_label);
    lv_obj_add_event_cb(s_data_pwm_btn, event_pwm_btn_cb, LV_EVENT_CLICKED, NULL);

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

/* PWM 输出屏：布局与数据屏一致（标题 + Back + 状态行 + 2 列网格 + 底部按钮），
 * 仅显示 5 路（CH1~CH5）的输入值与对应输出脉宽。
 * 阈值规则：CH > 1650 -> 2000us，否则 -> 1000us。 */
static void create_pwm_screen(void)
{
    s_screen_pwm = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_pwm, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_pwm, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_pwm);
    lv_label_set_text(title, "PWM Output");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* 左上角 Back：返回数据屏（与数据屏 Back 返回主屏区分） */
    // lv_obj_t *back_btn = lv_btn_create(s_screen_pwm);
    // lv_obj_set_size(back_btn, 56, 36);
    // lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 8, 8);
    // lv_obj_t *back_label = lv_label_create(back_btn);
    // lv_label_set_text(back_label, "Back");
    // lv_obj_add_event_cb(back_btn, event_pwm_back_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 顶部状态行：阈值规则提示 */
    s_pwm_status_label = lv_label_create(s_screen_pwm);
    lv_label_set_text(s_pwm_status_label, "5ch  >1650 -> 2000us  else 1000us");
    lv_obj_add_style(s_pwm_status_label, &s_label_style, 0);
    lv_obj_set_style_text_color(s_pwm_status_label, lv_color_hex(0x8E8E93), 0);
    lv_obj_align(s_pwm_status_label, LV_ALIGN_TOP_MID, 0, 52);

    /* 5 路网格：2 列手动定位（与数据屏 11 通道网格同尺寸同间距） */
    lv_obj_t *grid = lv_obj_create(s_screen_pwm);
    lv_obj_set_size(grid, EXAMPLE_LCD_H_RES - 12, 282);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_pad_all(grid, 4, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        uint8_t col = i % 2;       /* 0=左列, 1=右列 */
        uint8_t row = i / 2;       /* 0..2（第 3 行仅 CH5） */
        lv_coord_t x = col ? 136 : 4;
        lv_coord_t y = 4 + row * 46;

        lv_obj_t *cell = lv_obj_create(grid);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, 124, 42);
        lv_obj_set_style_bg_color(cell, lv_color_hex(0xF2F2F7), 0);
        lv_obj_set_style_radius(cell, 6, 0);
        lv_obj_set_style_pad_all(cell, 3, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        /* 左上：通道名 "Ch1" */
        lv_obj_t *t = lv_label_create(cell);
        lv_label_set_text_fmt(t, "Ch%d", i + 1);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 4, 2);

        /* 右上：输入值 "1800"（来自 BLE 帧 CH1~CH5） */
        lv_obj_t *in_lbl = lv_label_create(cell);
        lv_label_set_text(in_lbl, "----");
        lv_obj_set_style_text_font(in_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(in_lbl, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(in_lbl, LV_ALIGN_TOP_RIGHT, -4, 2);

        /* 右下：输出 PWM "2000us"（2000 绿色 / 1000 灰色，即时区分） */
        lv_obj_t *out_lbl = lv_label_create(cell);
        lv_label_set_text(out_lbl, "----");
        lv_obj_set_style_text_font(out_lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(out_lbl, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(out_lbl, LV_ALIGN_BOTTOM_RIGHT, -4, -2);

        s_pwm_cells[i].title  = t;
        s_pwm_cells[i].input  = in_lbl;
        s_pwm_cells[i].output = out_lbl;
    }

    /* 底部信息行：5 路输出脉宽汇总（一眼看出哪些通道为高） */
    s_pwm_info_label = lv_label_create(s_screen_pwm);
    lv_label_set_text(s_pwm_info_label, "Out: ---- ---- ---- ---- ----");
    lv_obj_add_style(s_pwm_info_label, &s_label_style, 0);
    lv_obj_set_style_text_color(s_pwm_info_label, lv_color_hex(0x333333), 0);
    lv_obj_set_width(s_pwm_info_label, EXAMPLE_LCD_H_RES - 16);
    lv_label_set_long_mode(s_pwm_info_label, LV_LABEL_LONG_DOT);
    lv_obj_align(s_pwm_info_label, LV_ALIGN_TOP_MID, 0, 368);

    /* 底部按钮：Disconnect（复用） + Clear（清空 PWM 显示） */
    lv_obj_t *disconnect_btn = lv_btn_create(s_screen_pwm);
    lv_obj_add_style(disconnect_btn, &s_btn_style, 0);
    lv_obj_set_size(disconnect_btn, 130, 42);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t *disconnect_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_add_event_cb(disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_btn = lv_btn_create(s_screen_pwm);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 90, 42);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_add_event_cb(clear_btn, event_clear_pwm_cb, LV_EVENT_CLICKED, NULL);
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
    case UI_SCREEN_PWM:
        lv_scr_load(s_screen_pwm);
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
        ui_clear_pwm();
        ui_switch_screen(UI_SCREEN_DATA);
        break;
    case BLE_STATE_DISCONNECTED:
        s_connected_name[0] = '\0';
        if (s_data_status_label) {
            lv_label_set_text(s_data_status_label, "Disconnected");
        }
        ui_clear_pwm();
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

void ui_update_pwm_values(const int16_t *values, uint8_t count)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 取前 5 路（CH1~CH5）。count 不足 5 时仅更新可用部分。 */
    uint8_t n = (count < PWM_CHANNEL_COUNT) ? count : PWM_CHANNEL_COUNT;

    /* 底部汇总行的 5 段输出脉宽 */
    char summary[48] = "Out:";
    uint8_t summary_len = 4;

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        if (i < n) {
            int16_t v = values[i];
            uint16_t us = (v > PWM_VALUE_THRESHOLD) ? PWM_OUT_HIGH_US : PWM_OUT_LOW_US;

            /* 输入值：4 位零填充，与数据屏格式一致 */
            if (s_pwm_cells[i].input) {
                lv_label_set_text_fmt(s_pwm_cells[i].input, "%04d", v);
            }
            /* 输出 PWM：2000 绿色 / 1000 灰色，颜色即时区分高低状态 */
            if (s_pwm_cells[i].output) {
                lv_label_set_text_fmt(s_pwm_cells[i].output, "%uus", us);
                lv_obj_set_style_text_color(
                    s_pwm_cells[i].output,
                    (us == PWM_OUT_HIGH_US) ? lv_color_hex(0x34C759)
                                            : lv_color_hex(0x8E8E93),
                    0);
            }
            /* 追加到底部汇总：如 " 2000" */
            int remain = (int)sizeof(summary) - (int)summary_len;
            int w = snprintf(summary + summary_len, remain, " %u", us);
            if (w > 0) summary_len += w;
        } else {
            /* 数据不足：该路保持 "----" */
            int remain = (int)sizeof(summary) - (int)summary_len;
            int w = snprintf(summary + summary_len, remain, " ----");
            if (w > 0) summary_len += w;
        }
    }

    if (s_pwm_info_label) {
        lv_label_set_text(s_pwm_info_label, summary);
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_clear_pwm(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        if (s_pwm_cells[i].input) {
            lv_label_set_text(s_pwm_cells[i].input, "----");
        }
        if (s_pwm_cells[i].output) {
            lv_label_set_text(s_pwm_cells[i].output, "----");
            lv_obj_set_style_text_color(s_pwm_cells[i].output,
                                        lv_color_hex(0x8E8E93), 0);
        }
    }
    if (s_pwm_info_label) {
        lv_label_set_text(s_pwm_info_label, "Out: ---- ---- ---- ---- ----");
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

/* 数据屏右上角 "PWM" 按钮：跳转到 PWM 输出屏 */
static void event_pwm_btn_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_PWM);
}

/* PWM 屏左上角 "Back" 按钮：返回数据屏 */
static void event_pwm_back_btn_cb(lv_event_t *e)
{
    ui_switch_screen(UI_SCREEN_DATA);
}

/* PWM 屏 "Clear" 按钮：清空 5 路显示（直到下一帧 BLE 数据到达） */
static void event_clear_pwm_cb(lv_event_t *e)
{
    ui_clear_pwm();
}
