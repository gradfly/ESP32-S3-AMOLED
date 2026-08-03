#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "lvgl.h"
#include "ble_manager.h"

typedef enum {
    UI_SCREEN_MAIN = 0,
    UI_SCREEN_LIST,
    UI_SCREEN_DATA,
} ui_screen_t;

void ui_init(void);
void ui_switch_screen(ui_screen_t screen);
void ui_update_scan_results(void);
void ui_update_state(ble_state_t state, const char *message);
void ui_append_data(const uint8_t *data, uint16_t len);
void ui_clear_data(void);

#endif
