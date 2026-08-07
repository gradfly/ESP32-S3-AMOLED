#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include "display_bsp.h"
#include "lvgl.h"
#include "lv_demos.h"

extern "C" void app_main(void) {
    DisplayPort_Init();
    if(DisplayPort_Lock(-1) == ESP_OK)
  	{
		lv_demo_widgets(); 
		//lv_demo_music();
    	DisplayPort_Unlock();
  	}
}