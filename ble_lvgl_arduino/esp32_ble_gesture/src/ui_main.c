#include "ui_main.h"
#include "lcd_bsp.h"
#include "lcd_config.h"
#include "pwm_manager.h"
#include "gesture_images.h"
#include "esp_log.h"
#include <string.h>

/* 中文字体声明（由 lv_font_conv 生成，基于 msyh 字体） */
extern const lv_font_t lv_font_cjk_14;

static const char *TAG = "UI";

static lv_obj_t *s_screen_main = NULL;
static lv_obj_t *s_screen_list = NULL;
static lv_obj_t *s_screen_data = NULL;
static lv_obj_t *s_screen_pwm = NULL;
static lv_obj_t *s_screen_gesture = NULL;   /* 数字手势屏：12 格图片网格 */
static lv_obj_t *s_screen_gesture_recv = NULL;  /* 手势识别屏：BLE 数据对应手势图形 */

/* 手势识别屏 UI 对象 */
static lv_obj_t *s_gesture_recv_img = NULL;     /* 手势图片（3x 放大显示） */
static lv_obj_t *s_gesture_recv_name = NULL;    /* 手势名称标签（Gesture 1 等） */
static lv_obj_t *s_gesture_recv_status = NULL;  /* 顶部状态行：连接状态 */
static lv_obj_t *s_gesture_recv_values = NULL;  /* 5 路通道值 + 模式显示 */

/* 手势屏状态：需在 gesture_swipe_to_main() 等早期定义的函数前声明 */
#define GESTURE_COUNT 12
static lv_obj_t *s_gesture_cells[GESTURE_COUNT] = {0};      /* 12 格子对象（点击匹配） */
static lv_obj_t *s_gesture_name_labels[GESTURE_COUNT] = {0};/* 各格名称标签（选中改色） */
static uint8_t   s_selected_gesture = 0xFF;              /* 当前选中手势索引，0xFF=无 */
static lv_obj_t *s_gesture_status_label = NULL;   /* 顶部状态行 */
static lv_obj_t *s_gesture_info_label   = NULL;   /* 底部信息行：6 路输出汇总 */

static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_scan_list = NULL;
static lv_obj_t *s_connect_btn = NULL;
static lv_obj_t *s_disconnect_btn = NULL;
static lv_obj_t *s_scan_btn = NULL;

/* UUID 设置屏：连接前让用户指定 服务/通知/写 特征 UUID */
static lv_obj_t *s_screen_uuid = NULL;
static lv_obj_t *s_ta_service = NULL;   /* 服务 UUID textarea，默认 FFE0 */
static lv_obj_t *s_ta_notify = NULL;    /* 通知特征 UUID textarea，默认 FFE2 */
static lv_obj_t *s_ta_write = NULL;    /* 写特征 UUID textarea，默认 FFE1 */
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
    lv_obj_t *cell;     /* 格子容器（用于设置边框等样式 + 接收点击） */
    lv_obj_t *title;    /* "Ch1" */
    lv_obj_t *input;    /* 输入值 "1800" */
    lv_obj_t *output;   /* 输出 PWM "2000us" */
} pwm_cell_t;
static pwm_cell_t s_pwm_cells[PWM_CHANNEL_COUNT];   /* CH1~CH6（CH6 由 CH1 派生） */

/* 急停按钮：开启后所有 CH 通道强制输出 1500us，再次点击恢复 */
static lv_obj_t *s_estop_btn   = NULL;
static lv_obj_t *s_estop_label = NULL;
/* gesture 页面急停按钮：与 PWM 页面按钮共享同一全局急停状态 */
static lv_obj_t *s_gesture_estop_btn   = NULL;
static lv_obj_t *s_gesture_estop_label = NULL;

/* 最新一帧数据缓存：供 cell 点击回调立即刷新 PWM 硬件 + UI 使用 */
static int16_t s_pwm_values_cache[BLE_DATA_VALUE_COUNT] = {0};
static uint8_t s_pwm_count_cache = 0;

static uint16_t s_selected_device = 0;
static ble_device_info_t s_device_list[BLE_SCAN_RESULT_MAX];
static uint16_t s_device_count = 0;
static char s_connected_name[32] = "";   /* 当前已连接设备名，用于数据屏显示 */

/* 最近一次 BLE 状态，供切屏时刷新目标屏状态显示（解决状态变化时非活动屏
 * status label 未刷新的问题：连接后回主屏/PWM 屏才更新文字） */
static ble_state_t s_last_state = BLE_STATE_IDLE;
static char s_last_message[48] = "";

/* 滑动切屏动画（lv_scr_load_anim）结束后刷新目标屏状态的一次性定时器。
 * 动画期间活动屏仍是旧屏，需延迟到 SWIPE_ANIM_MS 后再 refresh，
 * 避免对非活动屏对象操作导致 LVGL 卡死。 */
static lv_timer_t *s_status_refresh_timer = NULL;

static lv_style_t s_btn_style;
static lv_style_t s_label_style;
static lv_style_t s_title_style;
static lv_style_t s_list_item_selected_style;  /* 设备列表项选中样式：蓝底白字 */

/* 便捷函数：为标签设置中文字体 */
static void set_cjk_font(lv_obj_t *obj)
{
    lv_obj_set_style_text_font(obj, &lv_font_cjk_14, 0);
}

/* ====== 屏幕边缘滑动切换 ======
 * 主屏 / 数据屏 / PWM 屏 / 手势识别屏 线性顺序：Main <-> Data <-> PWM <-> GestureRecv。
 * 左边缘起手右滑 -> 上一屏；右边缘起手左滑 -> 下一屏。
 * 手势训练屏不在此线性顺序中：由主屏 "自主训练" 按钮进入，右边缘左滑返回主屏。
 * 按下需落在屏幕空白区域（非按钮/列表等子对象），否则手势事件不会到达屏幕。 */
#define SWIPE_EDGE_WIDTH    80      /* 距左右边缘 80px 内起手才算边缘滑动 */
#define SWIPE_ANIM_MS       300     /* 切屏滑动动画时长 */
#define SWIPE_DEBOUNCE_MS   400     /* 防抖：两次滑动最小间隔（>动画时长） */
static const ui_screen_t s_swipe_order[] = {UI_SCREEN_MAIN, UI_SCREEN_DATA, UI_SCREEN_PWM, UI_SCREEN_GESTURE_RECV};
#define SWIPE_ORDER_LEN  (sizeof(s_swipe_order) / sizeof(s_swipe_order[0]))

static lv_point_t s_swipe_start_pt = {0};
static bool       s_swipe_started  = false;
static uint32_t   s_last_swipe_ms  = 0;

static void event_scan_btn_cb(lv_event_t *e);
static void event_train_btn_cb(lv_event_t *e);     /* 主屏 "自主训练" -> 手势屏 */
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
static void event_pwm_cell_click_cb(lv_event_t *e); /* PWM 格子点击：切换手动覆盖 */
static void event_estop_btn_cb(lv_event_t *e);      /* 急停按钮：切换全局急停 */
static void event_gesture_cell_click_cb(lv_event_t *e); /* 手势格子点击：选中+输出 PWM */
static void swipe_press_cb(lv_event_t *e);          /* 按下：记录起手坐标 */
static void swipe_gesture_cb(lv_event_t *e);        /* 屏幕级手势：边缘+方向 -> 切屏 */
static void swipe_zone_gesture_cb(lv_event_t *e);   /* 热区手势：边缘热区 -> 切屏 */
static void swipe_do_switch(bool go_next);          /* 执行切屏（带动画+防抖） */
static void setup_screen_swipe(lv_obj_t *screen);   /* 为屏幕绑定边缘滑动事件（含热区） */
static void setup_screen_swipe_base(lv_obj_t *screen); /* 仅屏幕级手势（无热区） */
static void gesture_swipe_gesture_cb(lv_event_t *e);  /* 手势屏手势：右边缘左滑 -> 主屏 */
static void setup_gesture_screen_swipe(lv_obj_t *screen); /* 手势屏屏幕级手势（无热区） */
static void gesture_update_info_label(const uint16_t *us); /* 更新手势屏底部 6 路汇总 */
static void refresh_active_screen_status(void);     /* 按当前 BLE 状态刷新活动屏 status */
static void status_refresh_timer_cb(lv_timer_t *t); /* 滑动动画结束后 refresh 回调 */
static void schedule_status_refresh(void);           /* 安排一次性 refresh 定时器 */

static void create_main_screen(void);
static void create_list_screen(void);
static void create_data_screen(void);
static void create_uuid_screen(void);
static void create_pwm_screen(void);
static void create_gesture_screen(void);
static void create_gesture_recv_screen(void);

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
        case UI_SCREEN_MAIN:    scr = s_screen_main;    break;
        case UI_SCREEN_DATA:    scr = s_screen_data;    break;
        case UI_SCREEN_PWM:     scr = s_screen_pwm;     break;
        case UI_SCREEN_GESTURE: scr = s_screen_gesture; break;
        case UI_SCREEN_GESTURE_RECV: scr = s_screen_gesture_recv; break;
        default: break;
        }
        if (scr == active) { cur_idx = i; break; }
    }
    if (cur_idx < 0) return;

    int target_idx = go_next ? cur_idx + 1 : cur_idx - 1;
    if (target_idx < 0 || target_idx >= (int)SWIPE_ORDER_LEN) return;  /* 越界：已到头 */

    lv_obj_t *target_scr = NULL;
    switch (s_swipe_order[target_idx]) {
    case UI_SCREEN_MAIN:    target_scr = s_screen_main;    break;
    case UI_SCREEN_DATA:    target_scr = s_screen_data;    break;
    case UI_SCREEN_PWM:     target_scr = s_screen_pwm;     break;
    case UI_SCREEN_GESTURE: target_scr = s_screen_gesture; break;
    case UI_SCREEN_GESTURE_RECV: target_scr = s_screen_gesture_recv; break;
    default: return;
    }

    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;
    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    lv_scr_load_anim_t anim = go_next ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                      : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(target_scr, anim, SWIPE_ANIM_MS, 0, false);
    s_last_swipe_ms = now;
    /* 动画结束后 refresh 目标屏状态（动画期间活动屏未切换，需延迟） */
    schedule_status_refresh();

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

/* 为屏幕绑定屏幕级边缘滑动事件（不含透明热区）。
 * 适用于格子本身已绑定 PRESSED+GESTURE 的屏幕（如 PWM 屏）。 */
static void setup_screen_swipe_base(lv_obj_t *screen)
{
    if (!screen) return;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, swipe_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, swipe_gesture_cb, LV_EVENT_GESTURE, NULL);
}

/* 为屏幕绑定边缘滑动事件：
 * 1. 屏幕级 PRESSED+GESTURE（空白区域起手的滑动）
 * 2. 两个透明边缘热区覆盖在网格上方（网格区域起手的滑动）
 * 热区仅覆盖中部纵向区域（y=70~360），避开顶部按钮和底部按钮。
 * 注意：热区会阻挡其覆盖区域内子对象的点击，故需要点击的屏幕（如 PWM 屏）
 * 应使用 setup_screen_swipe_base() 并在格子自身绑定手势。 */
static void setup_screen_swipe(lv_obj_t *screen)
{
    if (!screen) return;
    setup_screen_swipe_base(screen);

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

/* ====== 手势屏专用边缘滑动（不在线性顺序中） ======
 * 手势屏由主屏 "自主训练" 按钮进入，仅右边缘左滑返回主屏。
 * 不复用 swipe_do_switch（手势屏不在 s_swipe_order 中，cur_idx 会<0）。
 * 返回主屏前：6 路 PWM 输出回归姿势 1750/2000/2000/2000/2000/2000us。 */
static const uint16_t s_gesture_rest_pose[PWM_CHANNEL_COUNT] = {1750,2000,2000,2000,2000,2000};

static void gesture_swipe_to_main(void)
{
    /* 防抖：动画进行中不响应新滑动 */
    uint32_t now = lv_tick_get();
    if (now - s_last_swipe_ms < SWIPE_DEBOUNCE_MS) return;

    /* 返回主屏前设置回归姿势（手势模式仍开启，立即刷新硬件） */
    pwm_manager_set_gesture_outputs(s_gesture_rest_pose, PWM_CHANNEL_COUNT);

    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;
    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 更新手势屏底部汇总 + 清除选中高亮（下次进入为干净状态） */
    if (s_gesture_info_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "PWM: %u %u %u %u %u %u",
                 s_gesture_rest_pose[0], s_gesture_rest_pose[1], s_gesture_rest_pose[2],
                 s_gesture_rest_pose[3], s_gesture_rest_pose[4], s_gesture_rest_pose[5]);
        lv_label_set_text(s_gesture_info_label, buf);
    }
    for (uint8_t i = 0; i < GESTURE_COUNT; i++) {
        if (s_gesture_cells[i]) {
            lv_obj_set_style_bg_color(s_gesture_cells[i], lv_color_hex(0xF2F2F7), 0);
        }
        if (s_gesture_name_labels[i]) {
            lv_obj_set_style_text_color(s_gesture_name_labels[i], lv_color_hex(0x333333), 0);
        }
    }
    s_selected_gesture = 0xFF;

    lv_scr_load_anim(s_screen_main, LV_SCR_LOAD_ANIM_MOVE_LEFT, SWIPE_ANIM_MS, 0, false);
    s_last_swipe_ms = now;
    schedule_status_refresh();

    xSemaphoreGiveRecursive(mux);
}

/* 手势屏手势（屏幕级 + 格子级共用）：右边缘起手左滑 -> 返回主屏 */
static void gesture_swipe_gesture_cb(lv_event_t *e)
{
    if (!s_swipe_started) return;
    s_swipe_started = false;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);

    if (s_swipe_start_pt.x > EXAMPLE_LCD_H_RES - SWIPE_EDGE_WIDTH &&
        dir == LV_DIR_LEFT) {
        gesture_swipe_to_main();   /* 右边缘左滑 -> 主屏 */
    }
}

/* 为手势屏绑定屏幕级边缘滑动（无透明热区）。
 * 手势格子需接收点击，热区会阻挡点击，故格子自身另绑 PRESSED+GESTURE
 * （与 PWM 屏同模式），使右边缘左滑在格子区域也能触发返回主屏。 */
static void setup_gesture_screen_swipe(lv_obj_t *screen)
{
    if (!screen) return;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, swipe_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, gesture_swipe_gesture_cb, LV_EVENT_GESTURE, NULL);
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
    create_gesture_screen();
    create_gesture_recv_screen();

    /* 主屏 / 数据屏：含透明热区的完整边缘滑动（线性顺序 Main<->Data<->PWM<->GestureRecv） */
    setup_screen_swipe(s_screen_main);
    setup_screen_swipe(s_screen_data);
    /* PWM 屏仅用屏幕级手势（无热区）：格子自身需接收点击，热区会阻挡点击 */
    setup_screen_swipe_base(s_screen_pwm);
    /* 手势识别屏仅用屏幕级手势（无热区），与 PWM 屏一致 */
    setup_screen_swipe_base(s_screen_gesture_recv);
    /* 手势屏不在线性顺序中：由主屏 "自主训练" 按钮进入，右边缘左滑返回主屏 */
    setup_gesture_screen_swipe(s_screen_gesture);

    lv_scr_load(s_screen_main);
}

static void create_main_screen(void)
{
    s_screen_main = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_main, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_main, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_main);
    lv_label_set_text(title, "Select Mode");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    /* "扫描设备"：进入 BLE 扫描列表 */
    s_scan_btn = lv_btn_create(s_screen_main);
    lv_obj_add_style(s_scan_btn, &s_btn_style, 0);
    lv_obj_set_size(s_scan_btn, 200, 50);
    lv_obj_align(s_scan_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_t *scan_label = lv_label_create(s_scan_btn);
    lv_label_set_text(scan_label, "SCAN");
    lv_obj_set_style_text_font(scan_label, &lv_font_montserrat_22, 0);
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(s_scan_btn, event_scan_btn_cb, LV_EVENT_CLICKED, NULL);

    /* "自主训练"：进入手势屏（无需 BLE 连接，离线浏览 12 个手势） */
    lv_obj_t *train_btn = lv_btn_create(s_screen_main);
    lv_obj_add_style(train_btn, &s_btn_style, 0);
    lv_obj_set_size(train_btn, 200, 50);
    lv_obj_align(train_btn, LV_ALIGN_CENTER, 0, 20);
    lv_obj_t *train_label = lv_label_create(train_btn);
    lv_label_set_text(train_label, "TRAINING");
    lv_obj_set_style_text_font(train_label, &lv_font_montserrat_22, 0);
    lv_obj_center(train_label);
    lv_obj_add_event_cb(train_btn, event_train_btn_cb, LV_EVENT_CLICKED, NULL);

    s_status_label = lv_label_create(s_screen_main);
    lv_label_set_text(s_status_label, "Ready");
    lv_obj_add_style(s_status_label, &s_label_style, 0);
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, 80);
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
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
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
    lv_obj_set_style_text_font(connect_label, &lv_font_montserrat_16, 0);
    lv_obj_center(connect_label);
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
    // s_data_pwm_btn = lv_btn_create(s_screen_data);
    // lv_obj_add_style(s_data_pwm_btn, &s_btn_style, 0);
    // lv_obj_set_size(s_data_pwm_btn, 60, 38);
    // lv_obj_align(s_data_pwm_btn, LV_ALIGN_TOP_RIGHT, -8, 8);
    // lv_obj_t *pwm_jump_label = lv_label_create(s_data_pwm_btn);
    // lv_label_set_text(pwm_jump_label, "PWM");
    // lv_obj_center(pwm_jump_label);
    // lv_obj_add_event_cb(s_data_pwm_btn, event_pwm_btn_cb, LV_EVENT_CLICKED, NULL);

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
    lv_obj_set_size(s_disconnect_btn, 120, 42);
    lv_obj_align(s_disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t *disconnect_label = lv_label_create(s_disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_set_style_text_font(disconnect_label, &lv_font_montserrat_18, 0);
    lv_obj_center(disconnect_label);
    lv_obj_add_event_cb(s_disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_btn = lv_btn_create(s_screen_data);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 120, 42);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_18, 0);
    lv_obj_center(clear_label);
    lv_obj_add_event_cb(clear_btn, event_clear_data_cb, LV_EVENT_CLICKED, NULL);
}

/* PWM 输出屏：布局与数据屏一致（标题 + Back + 状态行 + 2 列网格 + 底部按钮），
 * 仅显示 5 路（CH1~CH5）的输入值与对应输出脉宽。
 * 阈值规则：CH > 650 -> 2000us，< 650 -> 1000us，= 650 -> 1500us。 */
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

    /* 顶部状态行：连接状态（与数据屏同步） */
    s_pwm_status_label = lv_label_create(s_screen_pwm);
    lv_label_set_text(s_pwm_status_label, "Connecting...");
    lv_obj_add_style(s_pwm_status_label, &s_label_style, 0);
    lv_obj_set_style_text_color(s_pwm_status_label, lv_color_hex(0x34C759), 0);
    lv_obj_align(s_pwm_status_label, LV_ALIGN_TOP_MID, 0, 52);

    /* 6 格网格：2 列手动定位（CH1~CH5 硬件通道 + CH6 显示镜像）。
     * 与数据屏 11 通道网格同尺寸同间距。 */
    lv_obj_t *grid = lv_obj_create(s_screen_pwm);
    lv_obj_set_size(grid, EXAMPLE_LCD_H_RES - 12, 282);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_pad_all(grid, 4, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);
    /* 网格本身不拦截点击：空白区/CH6 的按压透传到屏幕，便于边缘滑动切屏 */
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_CLICKABLE);

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        uint8_t col = i % 2;       /* 0=左列, 1=右列 */
        uint8_t row = i / 2;       /* 0..2（第 3 行：CH5 左、CH6 右） */
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
        /* 格子可点击：点击切换手动覆盖（输出 1500us） */
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cell, event_pwm_cell_click_cb, LV_EVENT_CLICKED, NULL);
        /* 格子也接收 PRESSED+GESTURE：使边缘滑动在格子区域也能触发切屏 */
        lv_obj_add_event_cb(cell, swipe_press_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(cell, swipe_gesture_cb, LV_EVENT_GESTURE, NULL);

        /* 左上：通道名 "Ch1" */
        lv_obj_t *t = lv_label_create(cell);
        lv_label_set_text_fmt(t, "Ch%d", i + 1);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 4, 2);

        /* 右上：输入值（CH1~CH5 取自身 BLE 值；CH6 取 CH1 输入值） */
        lv_obj_t *in_lbl = lv_label_create(cell);
        lv_label_set_text(in_lbl, "----");
        lv_obj_set_style_text_font(in_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(in_lbl, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(in_lbl, LV_ALIGN_TOP_RIGHT, -4, 2);

        /* 右下：输出 PWM（CH1~5: 2000绿/1000蓝; CH6: 1750绿/1400蓝; 1500灰） */
        lv_obj_t *out_lbl = lv_label_create(cell);
        lv_label_set_text(out_lbl, "----");
        lv_obj_set_style_text_font(out_lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(out_lbl, lv_color_hex(0x8E8E93), 0);
        lv_obj_align(out_lbl, LV_ALIGN_BOTTOM_RIGHT, -4, -2);

        s_pwm_cells[i].cell   = cell;
        s_pwm_cells[i].title  = t;
        s_pwm_cells[i].input  = in_lbl;
        s_pwm_cells[i].output = out_lbl;
    }

    /* 急停按钮：CH5/CH6 下方区域（网格内第 4 行）。
     * 点击切换全局急停：开启 -> 所有 CH 通道强制 1500us；再次点击 -> 恢复。 */
    s_estop_btn = lv_btn_create(grid);
    lv_obj_set_pos(s_estop_btn, 4, 150);          /* row2(y=138) 下方留 12px */
    lv_obj_set_size(s_estop_btn, 256, 70);
    lv_obj_set_style_bg_color(s_estop_btn, lv_color_hex(0xFF3B30), 0);
    lv_obj_set_style_radius(s_estop_btn, 10, 0);
    lv_obj_set_style_shadow_width(s_estop_btn, 0, 0);
    lv_obj_set_style_pad_all(s_estop_btn, 0, 0);
    s_estop_label = lv_label_create(s_estop_btn);
    lv_label_set_text(s_estop_label, "E-STOP");
    lv_obj_set_style_text_font(s_estop_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_estop_label, lv_color_white(), 0);
    lv_obj_center(s_estop_label);
    lv_obj_add_event_cb(s_estop_btn, event_estop_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 底部信息行：操作提示 */
    s_pwm_info_label = lv_label_create(s_screen_pwm);
    lv_label_set_text(s_pwm_info_label, "Click the Ch to pause single finger.\n Click E-STOP to pause all.\n Click again to resume.");
    lv_obj_set_style_text_font(s_pwm_info_label, &lv_font_montserrat_12, 0);
    lv_obj_add_style(s_pwm_info_label, &s_label_style, 0);
    lv_obj_center(s_pwm_info_label);
    lv_obj_set_style_text_color(s_pwm_info_label, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_align(s_pwm_info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_pwm_info_label, EXAMPLE_LCD_H_RES - 16);
    /* WRAP 模式：按设置的宽度自动换行；需配合 recolor=off 避免解析开销 */
    lv_label_set_long_mode(s_pwm_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_pwm_info_label, LV_ALIGN_TOP_MID, 0, 310);

    /* 底部按钮：Disconnect（复用） + Clear（清空 PWM 显示） */
    lv_obj_t *disconnect_btn = lv_btn_create(s_screen_pwm);
    lv_obj_add_style(disconnect_btn, &s_btn_style, 0);
    lv_obj_set_size(disconnect_btn, 120, 42);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t *disconnect_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");    
    lv_obj_set_style_text_font(disconnect_label, &lv_font_montserrat_18, 0);
    lv_obj_center(disconnect_label);
    lv_obj_add_event_cb(disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_btn = lv_btn_create(s_screen_pwm);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 120, 42);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_18, 0);
    lv_obj_center(clear_label);
    lv_obj_add_event_cb(clear_btn, event_clear_pwm_cb, LV_EVENT_CLICKED, NULL);
}

/* 数字手势屏：布局与 PWM 屏一致（标题 + 状态行 + 2 列网格 + 底部信息行）。
 * 12 格网格（2 列 x 6 行），每格左侧放手势图片（36x36），右侧放图片名称文字。
 * 图片顺序：1、2、3、4、5、6、7、8、10、ok、good、love。
 * 格子可点击：点击设置 6 路 PWM 输出（互斥，选中格蓝底）。
 * 由主屏 "自主训练" 按钮进入（开启手势模式），右边缘左滑返回主屏（输出回归姿势）。
 * 注：GESTURE_COUNT / s_gesture_cells / s_selected_gesture / s_gesture_*_label
 *     已在文件顶部声明（供 gesture_swipe_to_main 等早期函数使用）。 */
typedef struct {
    const lv_img_dsc_t *img;   /* 手势图片描述符 */
    const char         *name;  /* 图片名称文字 */
    uint16_t            pwm[PWM_CHANNEL_COUNT];  /* 点击该手势时 6 路输出脉宽(us) */
} gesture_entry_t;
static const gesture_entry_t s_gestures[GESTURE_COUNT] = {
    { &img_gesture_1,    "1",    {1000,2000,1000,1000,1000,1400} },
    { &img_gesture_2,    "2",    {1000,2000,2000,1000,1000,1400} },
    { &img_gesture_3,    "3",    {1000,2000,2000,2000,1000,1400} },
    { &img_gesture_4,    "4",    {1000,2000,2000,2000,2000,1400} },
    { &img_gesture_5,    "5",    {2000,2000,2000,2000,2000,1750} },
    { &img_gesture_6,    "6",    {2000,1000,1000,1000,2000,1750} },
    { &img_gesture_7,    "7",    {2000,2000,2000,1000,1000,1750} },
    { &img_gesture_8,    "8",    {2000,2000,1000,1000,1000,1750} },
    { &img_gesture_10,   "10",   {1000,1000,1000,1000,1000,1400} },
    { &img_gesture_ok,   "ok",   {1000,1000,2000,2000,2000,1400} },
    { &img_gesture_good, "good", {2000,1000,1000,1000,1000,1750} },
    { &img_gesture_love, "love", {2000,2000,1000,1000,2000,1750} },
};

static void create_gesture_screen(void)
{
    s_screen_gesture = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_gesture, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_gesture, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(s_screen_gesture);
    lv_label_set_text(title, "Gesture");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* 顶部状态行：手势总数提示 */
    s_gesture_status_label = lv_label_create(s_screen_gesture);
    lv_label_set_text(s_gesture_status_label, "12 Gestures");
    lv_obj_add_style(s_gesture_status_label, &s_label_style, 0);
    lv_obj_set_style_text_font(s_gesture_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_gesture_status_label, lv_color_hex(0x8E8E93), 0);
    lv_obj_align(s_gesture_status_label, LV_ALIGN_TOP_MID, 0, 52);

    /* 12 格网格：2 列手动定位，与 PWM 屏同尺寸同间距（cell 124x42, 间距 46）。
     * 6 行需 grid 高度 284（4 + 5*46 + 42 + 4 = 284）以完整容纳。 */
    lv_obj_t *grid = lv_obj_create(s_screen_gesture);
    lv_obj_set_size(grid, EXAMPLE_LCD_H_RES - 12, 284);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_pad_all(grid, 4, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);
    /* 网格不拦截点击：空白区按压透传到屏幕，便于边缘滑动切屏 */
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < GESTURE_COUNT; i++) {
        uint8_t col = i % 2;       /* 0=左列, 1=右列 */
        uint8_t row = i / 2;       /* 0..5 */
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
        /* 格子可点击：点击选中该手势并输出对应 6 路 PWM */
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cell, event_gesture_cell_click_cb, LV_EVENT_CLICKED, NULL);
        /* 格子也接收 PRESSED+GESTURE：使右边缘左滑在格子区域也能触发返回主屏 */
        lv_obj_add_event_cb(cell, swipe_press_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(cell, gesture_swipe_gesture_cb, LV_EVENT_GESTURE, NULL);

        /* 左侧：手势图片（36x36） */
        lv_obj_t *img = lv_img_create(cell);
        lv_img_set_src(img, s_gestures[i].img);
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 2, 0);

        /* 右侧：图片名称文字 */
        lv_obj_t *name = lv_label_create(cell);
        lv_label_set_text(name, s_gestures[i].name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0x333333), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 44, 0);

        s_gesture_cells[i] = cell;            /* 保存指针供点击回调匹配 + 选中高亮 */
        s_gesture_name_labels[i] = name;      /* 保存名称标签供选中时改色 */
    }

    /* 底部信息行：6 路输出脉宽汇总（上移，接在网格下方，为 E-STOP 按钮留出空间） */
    s_gesture_info_label = lv_label_create(s_screen_gesture);
    lv_label_set_text(s_gesture_info_label, "PWM: ---- ---- ---- ---- ---- ----");
    lv_obj_add_style(s_gesture_info_label, &s_label_style, 0);
    lv_obj_set_style_text_font(s_gesture_info_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_gesture_info_label, lv_color_hex(0x333333), 0);
    lv_obj_set_width(s_gesture_info_label, EXAMPLE_LCD_H_RES - 16);
    lv_label_set_long_mode(s_gesture_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_gesture_info_label, LV_ALIGN_TOP_MID, 0, 360);

    /* 急停按钮：放在屏幕最下方，功能与 PWM 页面 E-STOP 完全一致
     * 尺寸与 PWM 屏按钮一致 (256x70)，位置：底部留 6px 间距，
     * 顶部位置 = 456 - 6 - 70 = 380，信息行底部 360+20=380，两者无缝衔接 */
    s_gesture_estop_btn = lv_btn_create(s_screen_gesture);
    lv_obj_set_size(s_gesture_estop_btn, 256, 70);
    lv_obj_set_style_radius(s_gesture_estop_btn, 10, 0);
    lv_obj_set_style_shadow_width(s_gesture_estop_btn, 0, 0);
    lv_obj_set_style_pad_all(s_gesture_estop_btn, 0, 0);
    lv_obj_align(s_gesture_estop_btn, LV_ALIGN_BOTTOM_MID, 0, -6);
    s_gesture_estop_label = lv_label_create(s_gesture_estop_btn);
    lv_obj_set_style_text_font(s_gesture_estop_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_gesture_estop_label, lv_color_white(), 0);
    lv_obj_center(s_gesture_estop_label);
    lv_obj_add_event_cb(s_gesture_estop_btn, event_estop_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 初始化按钮外观：按当前全局急停状态同步（避免从 PWM 屏开启急停后切过来显示不一致） */
    bool init_estop = pwm_manager_get_estop();
    if (s_gesture_estop_label) {
        lv_label_set_text(s_gesture_estop_label, init_estop ? "Resume" : "E-STOP");
    }
    if (s_gesture_estop_btn) {
        if (init_estop) {
            lv_obj_set_style_bg_color(s_gesture_estop_btn, lv_color_hex(0xB71C1C), 0);
            lv_obj_set_style_border_color(s_gesture_estop_btn, lv_color_hex(0xFFEB3B), 0);
            lv_obj_set_style_border_width(s_gesture_estop_btn, 3, 0);
        } else {
            lv_obj_set_style_bg_color(s_gesture_estop_btn, lv_color_hex(0xFF3B30), 0);
            lv_obj_set_style_border_width(s_gesture_estop_btn, 0, 0);
        }
    }
}

/* ====== 手势识别屏：根据 BLE 前 5 路数据匹配并显示手势图形 ======
 * 阈值 650：value > 650 为高（1），value <= 650 为低（0）。
 * 5 路通道组合为 5 位模式，查表匹配对应手势图片。
 * 用户示例：<650, >650, <650, <650, <650 → 模式 01000 → Gesture 1 */
#define GESTURE_RECV_THRESHOLD  650
#define GESTURE_RECV_MAP_SIZE   12

typedef struct {
    bool pattern[5];              /* true = <650, false = >650 */
    const lv_img_dsc_t *img;
    const char *name;
} gesture_recv_entry_t;

static const gesture_recv_entry_t s_gesture_recv_map[GESTURE_RECV_MAP_SIZE] = {
    {{false, true,  false, false, false}, &img_gesture_1,    "Gesture 1"},   /* 01000 */
    {{false, true,  true,  false, false}, &img_gesture_2,    "Gesture 2"},   /* 01100 */
    {{false, true,  true,  true,  false}, &img_gesture_3,    "Gesture 3"},   /* 01110 */
    {{false, true,  true,  true,  true},  &img_gesture_4,    "Gesture 4"},   /* 01111 */
    {{true,  true,  true,  true,  true},  &img_gesture_5,    "Gesture 5"},   /* 11111 */
    {{true,  false, false, false, true},  &img_gesture_6,    "Gesture 6"},   /* 10001 */
    {{true,  true,  true,  false, false}, &img_gesture_7,    "Gesture 7"},   /* 11100 */
    {{true,  true,  false, false, false}, &img_gesture_8,    "Gesture 8"},   /* 11000 */
    {{false, false, false, false, false}, &img_gesture_10,   "Gesture 10"},  /* 00000 */
    {{true,  false, false, false, false}, &img_gesture_good, "Good"},        /* 10000 */
    {{false, false, true,  true,  true},  &img_gesture_ok,   "OK"},          /* 00111 */
    {{true,  true,  false, false, true},  &img_gesture_love, "Love"},        /* 11001 */
};

static void create_gesture_recv_screen(void)
{
    s_screen_gesture_recv = lv_obj_create(NULL);
    lv_obj_set_size(s_screen_gesture_recv, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(s_screen_gesture_recv, lv_color_hex(0xFFFFFF), 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(s_screen_gesture_recv);
    lv_label_set_text(title, "Gesture");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* 顶部状态行：连接状态（与数据屏/PWM屏同步） */
    s_gesture_recv_status = lv_label_create(s_screen_gesture_recv);
    lv_label_set_text(s_gesture_recv_status, "Connecting...");
    lv_obj_add_style(s_gesture_recv_status, &s_label_style, 0);
    lv_obj_set_style_text_color(s_gesture_recv_status, lv_color_hex(0x34C759), 0);
    lv_obj_align(s_gesture_recv_status, LV_ALIGN_TOP_MID, 0, 48);

    /* 手势图片：居中，3x 放大（36*3=144px） */
    s_gesture_recv_img = lv_img_create(s_screen_gesture_recv);
    lv_img_set_src(s_gesture_recv_img, &img_gesture_1);
    lv_img_set_zoom(s_gesture_recv_img, 256 * 4);
    lv_obj_align(s_gesture_recv_img, LV_ALIGN_TOP_MID, 0, 150);

    /* 手势名称标签 */
    s_gesture_recv_name = lv_label_create(s_screen_gesture_recv);
    lv_label_set_text(s_gesture_recv_name, "Gesture 1");
    lv_obj_set_style_text_font(s_gesture_recv_name, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_gesture_recv_name, lv_color_hex(0x1A1A1A), 0);
    lv_obj_align(s_gesture_recv_name, LV_ALIGN_TOP_MID, 0, 250);

    /* 5 路通道值 + 模式显示 */
    s_gesture_recv_values = lv_label_create(s_screen_gesture_recv);
    lv_label_set_text(s_gesture_recv_values,
                      "CH1:----  CH2:----  CH3:----\nCH4:----  CH5:----\nPattern: - - - - -");
    lv_obj_set_style_text_font(s_gesture_recv_values, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_gesture_recv_values, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_style_text_align(s_gesture_recv_values, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_gesture_recv_values, LV_ALIGN_TOP_MID, 0, 290);

    /* 底部 Disconnect 按钮 */
    lv_obj_t *disconnect_btn = lv_btn_create(s_screen_gesture_recv);
    lv_obj_add_style(disconnect_btn, &s_btn_style, 0);
    lv_obj_set_size(disconnect_btn, 120, 42);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t *disconnect_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_set_style_text_font(disconnect_label, &lv_font_montserrat_18, 0);
    lv_obj_center(disconnect_label);
    lv_obj_add_event_cb(disconnect_btn, event_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);
    
    /* 底部 Clear 按钮 */
    lv_obj_t *clear_btn = lv_btn_create(s_screen_gesture_recv);
    lv_obj_add_style(clear_btn, &s_btn_style, 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_size(clear_btn, 120, 42);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_18, 0);
    lv_obj_center(clear_label);
    lv_obj_add_event_cb(clear_btn, event_clear_pwm_cb, LV_EVENT_CLICKED, NULL);
}

/* 更新手势屏底部信息行：6 路输出脉宽汇总 */
static void gesture_update_info_label(const uint16_t *us)
{
    if (!us) return;
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;
    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    if (s_gesture_info_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "PWM: %u %u %u %u %u %u",
                 us[0], us[1], us[2], us[3], us[4], us[5]);
        lv_label_set_text(s_gesture_info_label, buf);
    }

    xSemaphoreGiveRecursive(mux);
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
    lv_label_set_text(title, "UUID Config");
    lv_obj_add_style(title, &s_title_style, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    /* 3 个 UUID 字段：标签 + 单行 textarea，最大 8 字符（16/32 位 hex） */
    static const char *labels[3]   = {"Service", "Notify", "Write"};
    /* 目标手机 GATT（nRF Connect 实测）：
     *   Service: 0000FFE0-0000-1000-8000-00805F9B34FB
     *   Notify : 0000FFE2-0000-1000-8000-00805F9B34FB
     *   Write  : 0000FFE1-0000-1000-8000-00805F9B34FB
     * NimBLEUUID(std::string) 会把 16 位扩展到 128 位标准基比较，
     * 所以这里直接写 16 位短码即可匹配完整 128 位 UUID。 */
    static const char *defaults[3] = {"FFE0", "FFE2", "FFE1"};
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

    /* 先切到数据屏，再更新状态——避免在非活动屏上操作对象 */
    ui_switch_screen(UI_SCREEN_DATA);
    if (s_data_status_label) {
        lv_label_set_text(s_data_status_label, "Connecting...");
    }
    ble_manager_connect(&s_device_list[s_selected_device], &uuids);
}

/* 切屏后或状态变化时调用：根据 s_last_state 刷新当前活动屏的状态显示。
 * 仅操作当前活动屏对象，避免对非活动屏写触发重绘导致 LVGL 卡死。
 * 各屏 status 内容规则：
 *   MAIN  s_status_label:       显示 s_last_message
 *   MAIN  s_scan_btn 文字/禁用: SCANNING→"Scanning..."+禁用, 其它→"Scan"+可用
 *   DATA  s_data_status_label:  CONNECTED→"Connected: <name>", 否则→s_last_message
 *   PWM   s_pwm_status_label:   同 DATA
 * LIST/UUID/GESTURE 屏无 status label，自动跳过。 */
static void refresh_active_screen_status(void)
{
    lv_obj_t *scr = lv_scr_act();
    const char *msg = s_last_message[0] ? s_last_message : "BLE Ready";

    if (scr == s_screen_main) {
        if (s_status_label) {
            lv_label_set_text(s_status_label, msg);
        }
        if (s_scan_btn) {
            lv_obj_t *lbl = lv_obj_get_child(s_scan_btn, 0);
            if (s_last_state == BLE_STATE_SCANNING) {
                if (lbl) lv_label_set_text(lbl, "Scanning...");
                lv_obj_add_state(s_scan_btn, LV_STATE_DISABLED);
            } else {
                if (lbl) lv_label_set_text(lbl, "Scan");
                lv_obj_clear_state(s_scan_btn, LV_STATE_DISABLED);
            }
        }
    } else if (scr == s_screen_data) {
        if (s_data_status_label) {
            if (s_last_state == BLE_STATE_CONNECTED && s_connected_name[0]) {
                char buf[48];
                snprintf(buf, sizeof(buf), "Connected: %s", s_connected_name);
                lv_label_set_text(s_data_status_label, buf);
            } else {
                lv_label_set_text(s_data_status_label, msg);
            }
        }
    } else if (scr == s_screen_pwm) {
        if (s_pwm_status_label) {
            if (s_last_state == BLE_STATE_CONNECTED && s_connected_name[0]) {
                char buf[48];
                snprintf(buf, sizeof(buf), "Connected: %s", s_connected_name);
                lv_label_set_text(s_pwm_status_label, buf);
            } else {
                lv_label_set_text(s_pwm_status_label, msg);
            }
        }
        /* 切到 PWM 屏时用缓存数据刷新格子显示（非活动期间数据已缓存但未更新 UI） */
        ui_update_pwm_values(s_pwm_values_cache, s_pwm_count_cache);
    } else if (scr == s_screen_gesture_recv) {
        if (s_gesture_recv_status) {
            if (s_last_state == BLE_STATE_CONNECTED && s_connected_name[0]) {
                char buf[48];
                snprintf(buf, sizeof(buf), "Connected: %s", s_connected_name);
                lv_label_set_text(s_gesture_recv_status, buf);
            } else {
                lv_label_set_text(s_gesture_recv_status, msg);
            }
        }
        /* 切到手势识别屏时用缓存数据刷新手势显示 */
        ui_update_gesture_recv(s_pwm_values_cache, s_pwm_count_cache);
    }
}

/* lv_scr_load_anim 动画期间活动屏仍是旧屏，动画结束（SWIPE_ANIM_MS 后）才切换
 * 为目标屏。此回调在动画结束后触发，此时目标屏已活动，refresh 安全。 */
static void status_refresh_timer_cb(lv_timer_t *t)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (mux && xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) == pdTRUE) {
        refresh_active_screen_status();
        xSemaphoreGiveRecursive(mux);
    }
    s_status_refresh_timer = NULL;  /* 一次性 timer，LVGL 已自动删除 */
}

/* 滑动切屏后调用：安排一次性定时器，在动画结束后刷新目标屏状态显示。
 * 用于 swipe_do_switch / gesture_swipe_to_main（它们用 lv_scr_load_anim 带动画，
 * 不经过 ui_switch_screen，需补刷目标屏 status label）。 */
static void schedule_status_refresh(void)
{
    if (s_status_refresh_timer) {
        lv_timer_del(s_status_refresh_timer);
    }
    s_status_refresh_timer = lv_timer_create(status_refresh_timer_cb, SWIPE_ANIM_MS + 20, NULL);
    lv_timer_set_repeat_count(s_status_refresh_timer, 1);
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
    case UI_SCREEN_GESTURE:
        lv_scr_load(s_screen_gesture);
        break;
    case UI_SCREEN_GESTURE_RECV:
        lv_scr_load(s_screen_gesture_recv);
        break;
    }

    /* 切到目标屏后立即按当前 BLE 状态刷新该屏显示，确保进入任意屏
     * 都能看到正确的连接/扫描状态（修复连接后回主屏仍显示旧状态的问题） */
    refresh_active_screen_status();

    xSemaphoreGiveRecursive(mux);
}

void ui_update_state(ble_state_t state, const char *message)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 保存最近状态，供切屏时 refresh_active_screen_status() 刷新目标屏 */
    s_last_state = state;
    if (message) {
        strncpy(s_last_message, message, sizeof(s_last_message) - 1);
        s_last_message[sizeof(s_last_message) - 1] = '\0';
    }

    switch (state) {
    case BLE_STATE_CONNECTED:
        /* 区分两种连接模式：
         *   1) ESP32 作为 Client 去连接外设 → message="Connected" → 跳数据屏（旧行为）
         *   2) 手机小程序作为 Client 连接 ESP32 → message="Phone Connected" → 不跳屏，
         *      保留在首页显示 "Phone Connected" 提示，用户可手动滑到数据/PWM 屏 */
        {
            bool is_phone_connect = (message && strstr(message, "Phone") != NULL);
            if (is_phone_connect) {
                /* 小程序连接：设置 connected_name 为 "Phone"，
                 * 后续进入数据/PWM 屏时顶部会显示 "Connected: Phone" */
                strncpy(s_connected_name, "Phone", sizeof(s_connected_name) - 1);
                s_connected_name[sizeof(s_connected_name) - 1] = '\0';
                /* 不跳屏：停留在首页显示 "Phone Connected" 成功提示 */
                refresh_active_screen_status();
                /* 清空旧数据：即便未跳屏，后续滑动进入时也是干净状态 */
                ui_clear_data();
                ui_clear_pwm();
                /* 手势模式在连接后自动关闭：PWM 重新跟随 BLE 输入数据 */
                pwm_manager_set_gesture_mode(false);
            } else {
                /* 外设连接：跳数据屏（原有行为） */
                ui_switch_screen(UI_SCREEN_DATA);
                ui_clear_data();
                ui_clear_pwm();
            }
        }
        break;
    case BLE_STATE_DISCONNECTED:
        s_connected_name[0] = '\0';
        /* 同样区分两种断开：
         *   小程序断开 → message="Phone Disconnected" → 停在当前屏刷新状态
         *   外设断开 → message="Disconnected"         → 强制返回首页（旧行为） */
        {
            bool is_phone_disconnect = (message && strstr(message, "Phone") != NULL);
            if (is_phone_disconnect) {
                /* 小程序断开：刷新当前屏状态，不跳回首页
                 * （用户可能刚切到数据屏想看历史，直接跳回会很突兀） */
                refresh_active_screen_status();
                /* 清空手动覆盖 + 急停状态，释放舵机回归低脉宽 */
                for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
                    pwm_manager_set_override(i, false);
                }
                pwm_manager_set_estop(false);
                pwm_manager_set_gesture_mode(false);
                /* 触发一次 pwm_manager_update(NULL,1000) → 全通道按默认高脉宽输出 */
                pwm_manager_update(NULL, 1000);
            } else {
                /* 外设断开：跳回首页（原有行为） */
                ui_switch_screen(UI_SCREEN_MAIN);
            }
        }
        break;
    case BLE_STATE_SCANNING:
    case BLE_STATE_CONNECTING:
    case BLE_STATE_IDLE:
    default:
        /* 仅刷新当前活动屏。SCAN 时 LIST 屏无 status label 自动跳过；
         * 回主屏时由 ui_switch_screen 末尾 refresh 更新 s_scan_btn/s_status_label */
        refresh_active_screen_status();
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
            /* 为列表项内的标签设置中文字体 */
            lv_obj_t *btn_label = lv_obj_get_child(btn, 0);
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

    /* 仅在数据屏活动时更新——对非活动屏对象操作会触发重绘 */
    if (lv_scr_act() != s_screen_data) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

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

    /* 仅在数据屏活动时更新——对非活动屏对象操作会触发重绘，
     * 在切屏动画期间导致 LVGL 渲染负载激增、栈溢出重启。 */
    if (lv_scr_act() != s_screen_data) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

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

    /* 仅在数据屏活动时清屏——对非活动屏对象操作会触发重绘导致 LVGL 卡死。
     * 连接成功后数据会自动覆盖旧值，非活动时无需清屏。 */
    if (lv_scr_act() != s_screen_data) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

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

    /* 缓存最新数据，供 cell 点击回调立即刷新使用 */
    s_pwm_count_cache = (count < BLE_DATA_VALUE_COUNT) ? count : BLE_DATA_VALUE_COUNT;
    if (values && s_pwm_count_cache > 0) {
        memcpy(s_pwm_values_cache, values, s_pwm_count_cache * sizeof(int16_t));
    }

    /* 仅在 PWM 屏或 Gesture 屏活动时更新 UI 对象——对非活动屏对象操作
     * 会触发重绘，在切屏动画期间导致 LVGL 渲染负载激增、栈溢出重启。
     * 数据已缓存，切到 PWM/Gesture 屏后会从缓存刷新显示。 */
    lv_obj_t *act_scr = lv_scr_act();
    bool pwm_active = (act_scr == s_screen_pwm);
    bool gesture_active = (act_scr == s_screen_gesture);
    if (!pwm_active && !gesture_active) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

    /* 全局急停：开启时所有通道显示并输出 1500us */
    bool estop = pwm_manager_get_estop();

    /* PWM 格子更新：仅在 PWM 屏活动时执行 */
    if (pwm_active) {
    /* CH1~CH5 需要 count>=5 才有自身输入；CH6 输入取 CH1 值（需 count>=1） */
    uint8_t n = (s_pwm_count_cache < PWM_DIRECT_CH_COUNT) ? s_pwm_count_cache : PWM_DIRECT_CH_COUNT;
    bool has_ch1 = (s_pwm_count_cache >= 1);

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        bool ovr = pwm_manager_get_override(i);
        bool is_ch6 = (i >= PWM_DIRECT_CH_COUNT);   /* index 5 == CH6 */

        /* 输入值显示：CH1~CH5 取自身 BLE 值；CH6 取 CH1 输入值 */
        if (s_pwm_cells[i].input) {
            if (is_ch6) {
                if (has_ch1) {
                    lv_label_set_text_fmt(s_pwm_cells[i].input, "%04d", s_pwm_values_cache[0]);
                } else {
                    lv_label_set_text(s_pwm_cells[i].input, "----");
                }
            } else if (i < n) {
                lv_label_set_text_fmt(s_pwm_cells[i].input, "%04d", s_pwm_values_cache[i]);
            }
        }

        /* 输出脉宽：急停 > 单通道覆盖 > 自动跟随 CH 值 */
        uint16_t us;
        bool show_value;
        if (estop) {
            us = PWM_OUT_MID_US;                 /* 急停 -> 1500us */
            show_value = true;
        } else if (ovr) {
            us = PWM_OUT_MID_US;                 /* 覆盖 -> 1500us */
            show_value = true;
        } else if (is_ch6) {
            /* CH6：由 CH1 输入值派生（基于输入值，点击 CH1 覆盖不影响 CH6） */
            if (has_ch1) {
                int16_t v0 = s_pwm_values_cache[0];
                us = (v0 > PWM_VALUE_THRESHOLD) ? PWM_OUT_CH6_HIGH_US : PWM_OUT_CH6_LOW_US;
                show_value = true;
            } else {
                us = 0;
                show_value = false;
            }
        } else if (i < n) {
            int16_t v = s_pwm_values_cache[i];
            /* <650 -> 2000us，>650 -> 1000us，=650 -> 1500us */
            if (v < PWM_VALUE_THRESHOLD) {
                us = PWM_OUT_HIGH_US;
            } else if (v > PWM_VALUE_THRESHOLD) {
                us = PWM_OUT_LOW_US;
            } else {
                us = PWM_OUT_MID_US;
            }
            show_value = true;
        } else {
            us = 0;                              /* 无数据且未覆盖 */
            show_value = false;
        }

        /* 输出 PWM 文字 + 颜色：2000/1750 绿 / 1000/1400 蓝 / 1500 灰 */
        if (s_pwm_cells[i].output) {
            if (show_value) {
                lv_label_set_text_fmt(s_pwm_cells[i].output, "%uus", us);
                lv_color_t color;
                if (us == PWM_OUT_HIGH_US || us == PWM_OUT_CH6_HIGH_US)
                    color = lv_color_hex(0x34C759);   /* 绿 */
                else if (us == PWM_OUT_LOW_US || us == PWM_OUT_CH6_LOW_US)
                    color = lv_color_hex(0x007AFF);   /* 蓝 */
                else
                    color = lv_color_hex(0x8E8E93);   /* 灰(1500) */
                lv_obj_set_style_text_color(s_pwm_cells[i].output, color, 0);
            } else {
                lv_label_set_text(s_pwm_cells[i].output, "----");
                lv_obj_set_style_text_color(s_pwm_cells[i].output,
                                            lv_color_hex(0x8E8E93), 0);
            }
        }

        /* 覆盖状态：蓝色边框高亮（急停时不显示单通道覆盖边框） */
        if (s_pwm_cells[i].cell) {
            if (ovr && !estop) {
                lv_obj_set_style_border_color(s_pwm_cells[i].cell,
                                              lv_color_hex(0x007AFF), 0);
                lv_obj_set_style_border_width(s_pwm_cells[i].cell, 2, 0);
            } else {
                lv_obj_set_style_border_width(s_pwm_cells[i].cell, 0, 0);
            }
        }
    }
    } /* end if (pwm_active) */

    /* 急停按钮文字 + 样式：仅在对应屏活动时更新，避免非活动屏重绘 */
    if (pwm_active) {
        if (s_estop_label) {
            lv_label_set_text(s_estop_label, estop ? "Resume" : "E-STOP");
        }
        if (s_estop_btn) {
            if (estop) {
                lv_obj_set_style_bg_color(s_estop_btn, lv_color_hex(0xB71C1C), 0);
                lv_obj_set_style_border_color(s_estop_btn, lv_color_hex(0xFFEB3B), 0);
                lv_obj_set_style_border_width(s_estop_btn, 3, 0);
            } else {
                lv_obj_set_style_bg_color(s_estop_btn, lv_color_hex(0xFF3B30), 0);
                lv_obj_set_style_border_width(s_estop_btn, 0, 0);
            }
        }
    }
    if (gesture_active) {
        if (s_gesture_estop_label) {
            lv_label_set_text(s_gesture_estop_label, estop ? "Resume" : "E-STOP");
        }
        if (s_gesture_estop_btn) {
            if (estop) {
                lv_obj_set_style_bg_color(s_gesture_estop_btn, lv_color_hex(0xB71C1C), 0);
                lv_obj_set_style_border_color(s_gesture_estop_btn, lv_color_hex(0xFFEB3B), 0);
                lv_obj_set_style_border_width(s_gesture_estop_btn, 3, 0);
            } else {
                lv_obj_set_style_bg_color(s_gesture_estop_btn, lv_color_hex(0xFF3B30), 0);
                lv_obj_set_style_border_width(s_gesture_estop_btn, 0, 0);
            }
        }
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_clear_pwm(void)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;

    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 仅在 PWM 屏活动时清屏——对非活动屏对象操作会触发重绘导致 LVGL 卡死。
     * CONNECTED 等场景调用时 PWM 屏非活动，由后续数据刷新覆盖即可。 */
    if (lv_scr_act() != s_screen_pwm) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        if (s_pwm_cells[i].input) {
            lv_label_set_text(s_pwm_cells[i].input, "----");
        }
        if (s_pwm_cells[i].output) {
            lv_label_set_text(s_pwm_cells[i].output, "----");
            lv_obj_set_style_text_color(s_pwm_cells[i].output,
                                        lv_color_hex(0x8E8E93), 0);
        }
        /* 清除覆盖边框（但不清除覆盖状态本身——状态由 pwm_manager 维护） */
        if (s_pwm_cells[i].cell) {
            lv_obj_set_style_border_width(s_pwm_cells[i].cell, 0, 0);
        }
    }
    /* s_pwm_info_label 为固定操作提示，清屏时不重置 */
    /* 急停按钮文字复位（不影响急停状态本身——状态由 pwm_manager 维护） */
    if (s_estop_label) {
        lv_label_set_text(s_estop_label, pwm_manager_get_estop() ? "Resume" : "E-STOP"); 
    }

    xSemaphoreGiveRecursive(mux);
}

void ui_update_gesture_recv(const int16_t *values, uint8_t count)
{
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (!mux) return;
    if (xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) != pdTRUE) return;

    /* 仅在手势识别屏活动时更新 UI 对象——对非活动屏对象操作会触发重绘，
     * 在切屏动画期间导致 LVGL 渲染负载激增、栈溢出重启（与 PWM 屏修复一致）。
     * 数据已在 s_pwm_values_cache 中缓存，切到本屏后由 refresh_active_screen_status 刷新。 */
    if (lv_scr_act() != s_screen_gesture_recv) {
        xSemaphoreGiveRecursive(mux);
        return;
    }

    /* 数据不足 5 路时显示等待 */
    if (!values || count < 5) {
        if (s_gesture_recv_img) lv_obj_add_flag(s_gesture_recv_img, LV_OBJ_FLAG_HIDDEN);
        if (s_gesture_recv_name) lv_label_set_text(s_gesture_recv_name, "Waiting...");
        if (s_gesture_recv_values) lv_label_set_text(s_gesture_recv_values,
            "CH1:----  CH2:----  CH3:----\nCH4:----  CH5:----\nPattern: - - - - -");
        xSemaphoreGiveRecursive(mux);
        return;
    }

    /* 计算 5 位模式并构建显示文本 */
    bool pattern[5];
    char val_buf[96];
    int pat_bits[5];

    for (int i = 0; i < 5; i++) {
        pattern[i] = (values[i] < GESTURE_RECV_THRESHOLD);
        pat_bits[i] = pattern[i] ? 1 : 0;
    }

    snprintf(val_buf, sizeof(val_buf),
             "CH1:%d  CH2:%d  CH3:%d\nCH4:%d  CH5:%d\nPattern: %d %d %d %d %d",
             values[0], values[1], values[2], values[3], values[4],
             pat_bits[0], pat_bits[1], pat_bits[2], pat_bits[3], pat_bits[4]);

    /* 查找匹配的手势 */
    int matched = -1;
    for (int i = 0; i < GESTURE_RECV_MAP_SIZE; i++) {
        bool match = true;
        for (int j = 0; j < 5; j++) {
            if (s_gesture_recv_map[i].pattern[j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) { matched = i; break; }
    }

    /* 更新图片和名称 */
    if (matched >= 0) {
        if (s_gesture_recv_img) {
            lv_obj_clear_flag(s_gesture_recv_img, LV_OBJ_FLAG_HIDDEN);
            /* 只更新图片源，zoom 和 align 由 create_gesture_recv_screen() 统一设置，
             * 避免每次刷新都覆盖，改位置只需要调 create 里的 lv_obj_align 即可。 */
            lv_img_set_src(s_gesture_recv_img, s_gesture_recv_map[matched].img);
        }
        if (s_gesture_recv_name) {
            lv_label_set_text(s_gesture_recv_name, s_gesture_recv_map[matched].name);
        }
    } else {
        if (s_gesture_recv_img) lv_obj_add_flag(s_gesture_recv_img, LV_OBJ_FLAG_HIDDEN);
        if (s_gesture_recv_name) lv_label_set_text(s_gesture_recv_name, "No Match");
    }

    if (s_gesture_recv_values) {
        lv_label_set_text(s_gesture_recv_values, val_buf);
    }

    xSemaphoreGiveRecursive(mux);
}

static void event_scan_btn_cb(lv_event_t *e)
{
    /* 恢复 BLE 驱动模式：关闭手势模式，PWM 重新跟随 BLE 输入 */
    pwm_manager_set_gesture_mode(false);
    ui_switch_screen(UI_SCREEN_LIST);
    ble_manager_start_scan();
}

/* 主屏 "自主训练" 按钮：开启手势模式（直接指定 PWM 输出，忽略 BLE），
 * 切换到手势屏。手势模式下点击网格设置 6 路 PWM，右边缘左滑返回主屏。 */
static void event_train_btn_cb(lv_event_t *e)
{
    pwm_manager_set_gesture_mode(true);
    ui_switch_screen(UI_SCREEN_GESTURE);
}

/* 手势格子点击：选中该手势（互斥蓝底白字），设置对应 6 路 PWM 输出，
 * 更新底部 6 路汇总。12 格互斥，同一时刻仅 1 格高亮。
 * 注意：全局急停优先级高于手势输出，急停开启时硬件仍输出 1500us，
 *       底部汇总也同步显示 1500us（保持与实际硬件输出一致）。 */
static void event_gesture_cell_click_cb(lv_event_t *e)
{
    lv_obj_t *clicked = lv_event_get_current_target(e);
    if (!clicked) return;

    /* 通过指针匹配确定点击的是哪个手势格子 */
    uint8_t idx;
    for (idx = 0; idx < GESTURE_COUNT; idx++) {
        if (s_gesture_cells[idx] == clicked) break;
    }
    if (idx >= GESTURE_COUNT) return;

    /* 设置 6 路 PWM 输出（手势模式已开启，立即刷新硬件） */
    pwm_manager_set_gesture_outputs(s_gestures[idx].pwm, PWM_CHANNEL_COUNT);

    /* 互斥高亮：清除所有格子，当前格蓝底白字 */
    SemaphoreHandle_t mux = get_lvgl_mutex();
    if (mux && xSemaphoreTakeRecursive(mux, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint8_t i = 0; i < GESTURE_COUNT; i++) {
            bool sel = (i == idx);
            if (s_gesture_cells[i]) {
                lv_obj_set_style_bg_color(s_gesture_cells[i],
                    sel ? lv_color_hex(0x007AFF) : lv_color_hex(0xF2F2F7), 0);
            }
            if (s_gesture_name_labels[i]) {
                lv_obj_set_style_text_color(s_gesture_name_labels[i],
                    sel ? lv_color_white() : lv_color_hex(0x333333), 0);
            }
        }
        s_selected_gesture = idx;
        xSemaphoreGiveRecursive(mux);
    }

    /* 更新底部 6 路输出汇总：急停开启时统一显示 1500us（与硬件实际输出一致） */
    if (pwm_manager_get_estop()) {
        static const uint16_t estop_pwm[PWM_CHANNEL_COUNT] = {
            PWM_OUT_MID_US, PWM_OUT_MID_US, PWM_OUT_MID_US,
            PWM_OUT_MID_US, PWM_OUT_MID_US, PWM_OUT_MID_US
        };
        gesture_update_info_label(estop_pwm);
    } else {
        gesture_update_info_label(s_gestures[idx].pwm);
    }
}

static void event_connect_btn_cb(lv_event_t *e)
{
    if (s_selected_device >= s_device_count) {
        return;
    }
    /* 先切换到 UUID 屏，再初始化 textarea——避免在非活动屏上操作对象导致卡死 */
    ui_switch_screen(UI_SCREEN_UUID);
    lv_textarea_set_text(s_ta_service, "FFE0");
    lv_textarea_set_text(s_ta_notify, "FFE2");
    lv_textarea_set_text(s_ta_write, "FFE1");
    s_active_ta = s_ta_service;
    lv_obj_set_style_border_color(s_ta_service, lv_color_hex(0x007AFF), 0);
    lv_obj_set_style_border_color(s_ta_notify, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_color(s_ta_write, lv_color_hex(0xCCCCCC), 0);
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

/* PWM 格子点击：切换该通道的手动覆盖状态。
 * 覆盖时该通道直接输出 1500us（灰色），不受 CH 值影响；
 * 再次点击恢复自动跟随 CH 值。
 * 点击后立即刷新 PWM 硬件输出 + UI 显示（不等下一帧 BLE 数据）。 */
static void event_pwm_cell_click_cb(lv_event_t *e)
{
    lv_obj_t *clicked = lv_event_get_current_target(e);
    if (!clicked) return;

    /* 通过指针匹配确定点击的是哪个格子（回调注册在 cell 上，current_target 即 cell） */
    uint8_t ch;
    for (ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (s_pwm_cells[ch].cell == clicked) break;
    }
    if (ch >= PWM_CHANNEL_COUNT) return;

    /* 切换覆盖状态 */
    pwm_manager_toggle_override(ch);

    /* 立即刷新 PWM 硬件输出（使用缓存的最新数据） */
    pwm_manager_update(s_pwm_values_cache, s_pwm_count_cache);

    /* 立即刷新 UI 显示（输入值、输出脉宽、颜色、边框、汇总行） */
    ui_update_pwm_values(s_pwm_values_cache, s_pwm_count_cache);
}

/* 急停按钮：切换全局急停状态。
 * 开启 -> 所有 CH 通道强制输出 1500us（优先级高于 CH 值与单通道覆盖）；
 * 再次点击 -> 关闭急停，恢复各通道正常行为。
 * 点击后立即刷新 PWM 硬件 + UI（输出脉宽、颜色、汇总行、按钮文字）。
 * Gesture 页面的汇总信息行也同步更新（急停显示6路1500，关闭则恢复当前手势值）。 */
static void event_estop_btn_cb(lv_event_t *e)
{
    (void)e;
    bool new_state = !pwm_manager_get_estop();
    pwm_manager_set_estop(new_state);

    /* 立即刷新 PWM 硬件输出（使用缓存的最新数据） */
    pwm_manager_update(s_pwm_values_cache, s_pwm_count_cache);

    /* 立即刷新 PWM 屏 UI 显示（含两个页面的 E-STOP 按钮状态同步） */
    ui_update_pwm_values(s_pwm_values_cache, s_pwm_count_cache);

    /* Gesture 屏底部 6 路汇总信息行同步：急停 → 全部 1500us；关闭急停 → 恢复选中手势 */
    if (new_state) {
        static const uint16_t estop_pwm[PWM_CHANNEL_COUNT] = {
            PWM_OUT_MID_US, PWM_OUT_MID_US, PWM_OUT_MID_US,
            PWM_OUT_MID_US, PWM_OUT_MID_US, PWM_OUT_MID_US
        };
        gesture_update_info_label(estop_pwm);
    } else if (s_selected_gesture < GESTURE_COUNT) {
        /* 恢复之前选中的手势输出（如果有选中项） */
        pwm_manager_set_gesture_outputs(s_gestures[s_selected_gesture].pwm, PWM_CHANNEL_COUNT);
        gesture_update_info_label(s_gestures[s_selected_gesture].pwm);
    }
}
