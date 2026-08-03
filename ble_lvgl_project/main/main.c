#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_bsp.h"
#include "FT3168.h"
#include "ble_manager.h"
#include "ui_main.h"

static const char *TAG = "MAIN";

static void ble_data_callback(uint8_t *data, uint16_t len)
{
    ui_append_data(data, len);
}

static void ble_state_callback(ble_state_t state, const char *message)
{
    ui_update_state(state, message);
}

static void app_task(void *arg)
{
    ESP_LOGI(TAG, "Application task started");

    Touch_Init();
    lcd_lvgl_Init();

    ble_manager_init();
    ble_manager_set_data_callback(ble_data_callback);
    ble_manager_set_state_callback(ble_state_callback);

    ui_init();

    while (1) {
        ble_manager_process_data();

        ble_state_t state = ble_manager_get_state();
        if (state == BLE_STATE_IDLE) {
            static uint16_t last_scan_count = 0;
            uint16_t count = 0;
            ble_device_info_t results[BLE_SCAN_RESULT_MAX];
            ble_manager_get_scan_results(results, &count);
            if (count != last_scan_count) {
                ui_update_scan_results();
                last_scan_count = count;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting BLE LVGL Display application...");
    xTaskCreate(app_task, "app", 8192, NULL, 5, NULL);
}
