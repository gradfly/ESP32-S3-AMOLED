# 1 "C:\\Users\\xx.mu\\AppData\\Local\\Temp\\tmp2ly0wxai"
#include <Arduino.h>
# 1 "D:/OneDrive/project/机械手/穿戴康复外骨骼/显示屏/ble_lvgl_arduino/esp32_ble_gesture/esp32_ble_gesture.ino"
#include <Arduino.h>
#include <lvgl.h>
#include "esp_log.h"
#include "lcd_bsp.h"
#include "FT3168.h"
#include "ble_manager.h"
#include "ui_main.h"
#include "pwm_manager.h"




static int16_t s_latest_values[BLE_DATA_VALUE_COUNT] = {0};
static uint8_t s_latest_value_count = 0;
static char s_latest_raw[128] = "";
static uint16_t s_latest_raw_len = 0;
static bool s_data_dirty = false;






#define BLE_CMD_ESTOP_ON 9001
#define BLE_CMD_ESTOP_OFF 9000
#define BLE_CMD_THRESHOLD 9000
static void ble_data_callback(uint8_t *data, uint16_t len);
static void ble_state_callback(ble_state_t state, const char *message);
void setup();
void loop();
#line 28 "D:/OneDrive/project/机械手/穿戴康复外骨骼/显示屏/ble_lvgl_arduino/esp32_ble_gesture/esp32_ble_gesture.ino"
static void ble_data_callback(uint8_t *data, uint16_t len)
{

    int16_t tmp_values[BLE_DATA_VALUE_COUNT] = {0};
    uint8_t n = ble_manager_parse_frame(data, len, tmp_values, BLE_DATA_VALUE_COUNT);
    if (n == 0) return;


    if (tmp_values[0] >= BLE_CMD_THRESHOLD) {
        int16_t cmd = tmp_values[0];
        Serial.printf("[BLE] Received control cmd: %d\n", cmd);
        switch (cmd) {
        case BLE_CMD_ESTOP_ON:
            pwm_manager_set_estop(true);
            Serial.println("[BLE] ESTOP -> ON (all channels -> 1500us)");
            break;
        case BLE_CMD_ESTOP_OFF:
            pwm_manager_set_estop(false);
            Serial.println("[BLE] ESTOP -> OFF (resume normal output)");
            break;
        default:
            Serial.printf("[BLE] Unknown control cmd: %d, ignored\n", cmd);
            return;
        }

        pwm_manager_update(s_latest_values, s_latest_value_count);
        ui_update_pwm_values(s_latest_values, s_latest_value_count);


        uint16_t c = (len < sizeof(s_latest_raw) - 1) ? len : sizeof(s_latest_raw) - 1;
        memcpy(s_latest_raw, data, c);
        s_latest_raw[c] = '\0';
        s_latest_raw_len = c;

        ui_append_data((const uint8_t *)s_latest_raw, s_latest_raw_len);
        return;
    }





    if (pwm_manager_get_estop()) {
        pwm_manager_set_estop(false);
        Serial.println("[BLE] Normal data frame received while ESTOP ON -> auto OFF");
    }

    s_latest_value_count = n;
    memcpy(s_latest_values, tmp_values, sizeof(int16_t) * n);


    uint16_t c = (len < sizeof(s_latest_raw) - 1) ? len : sizeof(s_latest_raw) - 1;
    memcpy(s_latest_raw, data, c);
    s_latest_raw[c] = '\0';
    s_latest_raw_len = c;

    s_data_dirty = true;
}

static void ble_state_callback(ble_state_t state, const char *message)
{
    ui_update_state(state, message);
}

void setup()
{
    Serial.begin(115200);

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

    Serial.println("[STEP 5] Initializing PWM outputs...");
    pwm_manager_init();
    Serial.println("[STEP 5] PWM outputs initialized");

    Serial.println("========================================");
    Serial.println("  System Ready!");
    Serial.println("========================================");
    Serial.println();
}

void loop()
{



    ble_manager_process_state();


    ble_manager_process_data();


    if (s_data_dirty) {
        s_data_dirty = false;
        ui_update_data_values(s_latest_values, s_latest_value_count);
        ui_append_data((const uint8_t *)s_latest_raw, s_latest_raw_len);

        ui_update_pwm_values(s_latest_values, s_latest_value_count);

        ui_update_gesture_recv(s_latest_values, s_latest_value_count);

        pwm_manager_update(s_latest_values, s_latest_value_count);
    }

    static ble_state_t last_state = BLE_STATE_IDLE;
    static uint16_t last_scan_count = 0;
    static uint8_t loop_count = 0;
    ble_state_t state = ble_manager_get_state();


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


    loop_count++;
    if (loop_count == 0) {
        uint16_t count = 0;
        ble_manager_get_scan_results(NULL, &count);
        ESP_LOGI("LOOP", "Heartbeat: state=%d, scan_count=%d", state, count);
    }

    last_state = state;

    delay(50);
}