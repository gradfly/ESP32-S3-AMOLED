#include <Arduino.h>
#include <lvgl.h>
#include "esp_log.h"
#include "lcd_bsp.h"
#include "FT3168.h"
#include "ble_manager.h"
#include "ui_main.h"

/* 最新一帧解析结果（11 路数值）+ 脏标志。
 * ble_data_callback 在 ble_manager_process_data 内被调用（主任务上下文），
 * 仅记录最新值；loop() 中按脏标志统一刷新 UI，避免高频 notify 刷屏。 */
static int16_t s_latest_values[BLE_DATA_VALUE_COUNT] = {0};
static uint8_t s_latest_value_count = 0;
static char s_latest_raw[128] = "";
static uint16_t s_latest_raw_len = 0;
static bool s_data_dirty = false;

static void ble_data_callback(uint8_t *data, uint16_t len)
{
    /* data 是已按 ';' 重组的完整帧，直接解析为 11 个数值 */
    uint8_t n = ble_manager_parse_frame(data, len, s_latest_values, BLE_DATA_VALUE_COUNT);
    if (n > 0) {
        s_latest_value_count = n;

        /* 同步保存最新原始帧用于调试显示 */
        uint16_t c = (len < sizeof(s_latest_raw) - 1) ? len : sizeof(s_latest_raw) - 1;
        memcpy(s_latest_raw, data, c);
        s_latest_raw[c] = '\0';
        s_latest_raw_len = c;

        s_data_dirty = true;
    }
}

static void ble_state_callback(ble_state_t state, const char *message)
{
    ui_update_state(state, message);
}

void setup()
{
    Serial.begin(115200);
    // Wait for serial to connect (up to 3 seconds)
    uint32_t wait_start = millis();
    while (!Serial && (millis() - wait_start) < 3000) {
        delay(10);
    }
    delay(200);

    esp_log_level_set("*", ESP_LOG_INFO);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP32-S3 BLE LVGL Display Starting");
    Serial.println("========================================");

    Serial.println("[STEP 1] Initializing touch driver...");
    Touch_Init();
    Serial.println("[STEP 1] Touch driver initialized");

    Serial.println("[STEP 2] Initializing LCD and LVGL...");
    lcd_lvgl_Init();
    Serial.println("[STEP 2] LCD and LVGL initialized");

    Serial.println("[STEP 3] Initializing BLE manager...");
    ble_manager_init();
    ble_manager_set_data_callback(ble_data_callback);
    ble_manager_set_state_callback(ble_state_callback);
    Serial.println("[STEP 3] BLE manager initialized");

    Serial.println("[STEP 4] Initializing UI...");
    ui_init();
    Serial.println("[STEP 4] UI initialized");

    Serial.println("========================================");
    Serial.println("  System Ready!");
    Serial.println("========================================");
    Serial.println();
}

void loop()
{
    /* 处理 BLE 接收队列 + 帧重组，触发 ble_data_callback */
    ble_manager_process_data();

    /* 有新帧时统一刷新一次 UI（数值网格 + 原始帧） */
    if (s_data_dirty) {
        s_data_dirty = false;
        ui_update_data_values(s_latest_values, s_latest_value_count);
        ui_append_data((const uint8_t *)s_latest_raw, s_latest_raw_len);
    }

    static ble_state_t last_state = BLE_STATE_IDLE;
    static uint16_t last_scan_count = 0;
    static uint8_t loop_count = 0;  // For periodic debug output
    ble_state_t state = ble_manager_get_state();

    // Print state change
    if (state != last_state) {
        ESP_LOGI("LOOP", "State changed: %d -> %d", last_state, state);
        Serial.printf("[LOOP] State: %d -> %d\n", last_state, state);
    }

    if (state == BLE_STATE_IDLE && last_state == BLE_STATE_SCANNING) {
        uint16_t count = 0;
        ble_manager_get_scan_results(NULL, &count);
        ESP_LOGI("LOOP", "Scan complete: %d devices found", count);
        Serial.printf("[LOOP] Scan complete: %d devices found, updating UI...\n", count);
        ui_update_scan_results();
        last_scan_count = count;
    } else {
        uint16_t count = 0;
        ble_manager_get_scan_results(NULL, &count);
        if (count != last_scan_count) {
            ESP_LOGI("LOOP", "Scan results changed: %d -> %d", last_scan_count, count);
            ui_update_scan_results();
            last_scan_count = count;
        }
    }

    // Periodic debug output (every 256 loops)
    loop_count++;
    if (loop_count == 0) {
        uint16_t count = 0;
        ble_manager_get_scan_results(NULL, &count);
        ESP_LOGI("LOOP", "Heartbeat: state=%d, scan_count=%d", state, count);
    }

    last_state = state;
    /* 50ms 循环保证 ~20Hz 数据刷新，兼顾实时性与 CPU 占用 */
    delay(50);
}
