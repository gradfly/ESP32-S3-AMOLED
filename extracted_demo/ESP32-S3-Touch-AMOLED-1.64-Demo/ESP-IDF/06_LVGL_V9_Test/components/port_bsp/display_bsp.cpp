#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include "esp_lcd_co5300.h"
#include "display_bsp.h"
#include "esp_lv_adapter.h"
#include "user_config.h"
#include "i2c_bsp.h"

static const char* TAG = "Display";

I2cMasterBus I2cBus(48,47,0);
i2c_master_dev_handle_t touch_dev_handle;

static const co5300_lcd_init_cmd_t lcd_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 80},   
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 1},
    {0x63, (uint8_t []){0xFF}, 1, 1},
    {0x51, (uint8_t []){0x00}, 1, 1},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0}, 
};

static void LcdTouch_Init(void) {
    i2c_master_bus_handle_t BusHandle = I2cBus.Get_I2cBusHandle();
    i2c_device_config_t     dev_cfg   = {};
    dev_cfg.dev_addr_length           = I2C_ADDR_BIT_LEN_7;
    dev_cfg.scl_speed_hz              = 400000;
    dev_cfg.device_address            = 0x38;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(BusHandle, &dev_cfg, &touch_dev_handle));
}

static uint8_t LcdTouch_GetCoordinates(uint16_t *x,uint16_t *y)
{
    uint8_t buf[5];
    I2cBus.i2c_read_buff(touch_dev_handle,0x02,buf,5);
    if(buf[0]) {
      *x = (((uint16_t)buf[1] & 0x0f)<<8) | (uint16_t)buf[2];
      *y = (((uint16_t)buf[3] & 0x0f)<<8) | (uint16_t)buf[4];
      return 1;
    }
    return 0;
}

static void TouchInputReadCallback(lv_indev_t * indev, lv_indev_data_t *indevData) {
  	uint16_t x;
  	uint16_t y;
  	if(LcdTouch_GetCoordinates(&x,&y)) {
  	  	if(x > LCD_WIDTH)
  	  	{x = LCD_WIDTH;}
  	  	if(y > LCD_HEIGHT)
  	  	{y = LCD_HEIGHT;}
  	  	indevData->point.x = x;
  	  	indevData->point.y = y;
  	  	indevData->state = LV_INDEV_STATE_PRESSED;
  	} else {
  	  	indevData->state = LV_INDEV_STATE_RELEASED;
  	}
}

static void rounder_event_cb(lv_event_t *e) {
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

void DisplayPort_Init(void) {
    LcdTouch_Init();
    int max_trans_sz = LCD_WIDTH * LCD_HEIGHT;
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = LCD_SCL_PIN;                
    buscfg.data0_io_num = LCD_D0_PIN;             
    buscfg.data1_io_num = LCD_D1_PIN;             
    buscfg.data2_io_num = LCD_D2_PIN;             
    buscfg.data3_io_num = LCD_D3_PIN;             
    buscfg.max_transfer_sz = max_trans_sz;
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = LCD_CS_PIN;                                
    io_config.dc_gpio_num = -1;                            
    io_config.spi_mode = 0;                                 
    io_config.pclk_hz = 40 * 1000 * 1000;                   
    io_config.trans_queue_depth = 10;                       
    io_config.on_color_trans_done = NULL;                     
    io_config.user_ctx = NULL;                            
    io_config.lcd_cmd_bits = 32;                            
    io_config.lcd_param_bits = 8;                           
    io_config.flags.quad_mode = true;
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, &io_config, &io_handle));

    co5300_vendor_config_t vendor_config = {};
    vendor_config.init_cmds = lcd_init_cmds;
    vendor_config.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
    vendor_config.flags.use_qspi_interface = 1;

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST_PIN;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = &vendor_config;
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle,0x14,0x00));	

    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&cfg));
    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(
        panel_handle,           	// LCD 面板句柄
        io_handle,        			// LCD 面板 IO 句柄（某些接口可为 NULL）
        LCD_WIDTH,             		// 水平分辨率
        LCD_HEIGHT,             	// 垂直分辨率
        ESP_LV_ADAPTER_ROTATE_0 	// 旋转角度
    );
    disp_cfg.profile.buffer_height = 100; // 设置更合适的缓冲区高度以提高性能
    
	lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    assert(disp != NULL);
    lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);


    lv_indev_t *touch_indev = NULL;
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, TouchInputReadCallback);

    ESP_ERROR_CHECK(esp_lv_adapter_start());
}

int DisplayPort_Lock(int lock) {
    return esp_lv_adapter_lock(lock);
}

void DisplayPort_Unlock(void) {
    esp_lv_adapter_unlock();
}