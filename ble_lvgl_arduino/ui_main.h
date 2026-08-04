#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "lvgl.h"
#include "ble_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_SCREEN_MAIN = 0,
    UI_SCREEN_LIST,
    UI_SCREEN_UUID,   /* 连接前设置 服务/通知/写 UUID */
    UI_SCREEN_DATA,
    UI_SCREEN_PWM,    /* PWM 输出屏：显示 CH1~CH5 输入值 + 对应 PWM 输出 */
    UI_SCREEN_GESTURE,/* 数字手势屏：12 格网格，每格图片 + 名称 */
} ui_screen_t;

void ui_init(void);
void ui_switch_screen(ui_screen_t screen);
void ui_update_scan_results(void);
void ui_update_state(ble_state_t state, const char *message);
void ui_append_data(const uint8_t *data, uint16_t len);
void ui_clear_data(void);

/* 用最新一帧的 11 个数值刷新数据屏的通道网格（实时更新） */
void ui_update_data_values(const int16_t *values, uint8_t count);

/* 用最新一帧刷新 PWM 屏（取 CH1~CH5，按阈值 1650 输出 1000/2000us） */
void ui_update_pwm_values(const int16_t *values, uint8_t count);

/* 清空 PWM 屏的 5 路显示（恢复 "----"） */
void ui_clear_pwm(void);

#ifdef __cplusplus
}
#endif

#endif
