#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"

#define BLE_SCAN_RESULT_MAX  20
#define BLE_DATA_BUFFER_SIZE 512

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_SCANNING,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTED,
} ble_state_t;

typedef struct {
    uint8_t mac[6];
    char name[32];
    int8_t rssi;
} ble_device_info_t;

typedef void (*ble_data_callback_t)(uint8_t *data, uint16_t len);
typedef void (*ble_state_callback_t)(ble_state_t state, const char *message);

void ble_manager_init(void);
void ble_manager_start_scan(void);
void ble_manager_stop_scan(void);
void ble_manager_connect(uint8_t mac[6]);
void ble_manager_disconnect(void);
void ble_manager_send_data(uint8_t *data, uint16_t len);
ble_state_t ble_manager_get_state(void);
void ble_manager_get_scan_results(ble_device_info_t *results, uint16_t *count);
void ble_manager_set_data_callback(ble_data_callback_t cb);
void ble_manager_set_state_callback(ble_state_callback_t cb);
void ble_manager_process_data(void);

#endif
