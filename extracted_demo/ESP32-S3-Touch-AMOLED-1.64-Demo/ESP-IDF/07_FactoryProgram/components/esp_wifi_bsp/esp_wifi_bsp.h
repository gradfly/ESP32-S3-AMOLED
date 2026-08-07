#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"  //WIFI
void nvs_flash_Init(void);
void espwifi_Init(void);
void espwifi_Deinit(void);

#endif