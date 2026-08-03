#ifndef BLE_SCAN_BSP_H
#define BLE_SCAN_BSP_H
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"


void ble_scan_Init(void);
void ble_scan_setconf(void);
void ble_scan_Deinit(void);
void ble_scan_class_init(void);
extern EventGroupHandle_t ble_Even;
extern QueueHandle_t ble_Queue;

#endif
