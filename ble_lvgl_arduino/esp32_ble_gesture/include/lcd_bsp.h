#ifndef LCD_BSP_H
#define LCD_BSP_H

#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_lvgl_Init(void);
lv_disp_t *get_display(void);
SemaphoreHandle_t get_lvgl_mutex(void);
esp_err_t set_amoled_backlight(uint8_t brig);

#ifdef __cplusplus
}
#endif

#endif
