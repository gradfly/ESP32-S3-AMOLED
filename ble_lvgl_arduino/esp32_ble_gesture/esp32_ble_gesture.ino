#include <Arduino.h>
#include <lvgl.h>
#include "esp_log.h"
#include "lcd_bsp.h"
#include "FT3168.h"
#include "ble_manager.h"
#include "ui_main.h"
#include "pwm_manager.h"

/* 最新一帧解析结果（11 路数值）+ 脏标志。
 * ble_data_callback 在 ble_manager_process_data 内被调用（主任务上下文），
 * 仅记录最新值；loop() 中按脏标志统一刷新 UI，避免高频 notify 刷屏。 */
static int16_t s_latest_values[BLE_DATA_VALUE_COUNT] = {0};
static uint8_t s_latest_value_count = 0;
static char s_latest_raw[128] = "";
static uint16_t s_latest_raw_len = 0;
static bool s_data_dirty = false;

/* BLE 协议扩展：首值 ≥ 9000 表示"带外控制命令"（不会与正常 0~3000 的 CH 值范围冲突）
 *   CMD_ESTOP_ON  = 9001  →  开启全局急停（等效于 ESP32 端按 E-STOP 按钮）
 *   CMD_ESTOP_OFF = 9000  →  关闭急停（保留兼容，小程序不再使用此命令）
 * 小程序"恢复按钮"直接发送急停前的 CH 数组（正常数据帧），ESP32 收到后
 * 自动关闭急停并按新 CH 值刷新输出（见下方正常数据帧路径的 auto-off 逻辑）。 */
#define BLE_CMD_ESTOP_ON    9001
#define BLE_CMD_ESTOP_OFF   9000
#define BLE_CMD_THRESHOLD   9000

static void ble_data_callback(uint8_t *data, uint16_t len)
{
    /* data 是已按 ';' 重组的完整帧，直接解析为 11 个数值 */
    int16_t tmp_values[BLE_DATA_VALUE_COUNT] = {0};
    uint8_t n = ble_manager_parse_frame(data, len, tmp_values, BLE_DATA_VALUE_COUNT);
    if (n == 0) return;

    /* ======= 命令帧：首值 ≥ 9000 ======= */
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
        /* 命令也刷新 PWM 硬件输出 + UI（让 E-STOP 按钮状态 / 汇总行立即更新） */
        pwm_manager_update(s_latest_values, s_latest_value_count);
        ui_update_pwm_values(s_latest_values, s_latest_value_count);

        /* 命令帧保存一份原始数据（让数据屏可以看到命令帧），但不覆盖数值缓存 */
        uint16_t c = (len < sizeof(s_latest_raw) - 1) ? len : sizeof(s_latest_raw) - 1;
        memcpy(s_latest_raw, data, c);
        s_latest_raw[c] = '\0';
        s_latest_raw_len = c;
        /* 只刷新原始帧显示，不刷新数值网格（避免把命令值 9001 显示进 CH1） */
        ui_append_data((const uint8_t *)s_latest_raw, s_latest_raw_len);
        return;
    }

    /* ======= 正常数据帧：11 路 CH 映射 ======= */
    /* 小程序"恢复按钮"直接发送急停前的 CH 数组（正常数据帧），不再发送
     * CMD_ESTOP_OFF 命令帧。因此 ESP32 收到正常数据帧时若急停仍开启，
     * 自动关闭急停，随后按新 CH 值正常刷新输出。 */
    if (pwm_manager_get_estop()) {
        pwm_manager_set_estop(false);
        Serial.println("[BLE] Normal data frame received while ESTOP ON -> auto OFF");
    }

    s_latest_value_count = n;
    memcpy(s_latest_values, tmp_values, sizeof(int16_t) * n);

    /* 同步保存最新原始帧用于调试显示 */
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
    /* 处理 BLE 回调中设置的待处理状态变更（连接/断开通知）。
     * 必须在 ble_manager_process_data() 之前调用，确保连接状态
     * 先于数据更新通知到 UI 层。 */
    ble_manager_process_state();

    /* 处理 BLE 接收队列 + 帧重组，触发 ble_data_callback */
    ble_manager_process_data();

    /* 有新帧时统一刷新一次 UI（数值网格 + 原始帧）+ 更新 PWM 输出 */
    if (s_data_dirty) {
        s_data_dirty = false;
        ui_update_data_values(s_latest_values, s_latest_value_count);
        ui_append_data((const uint8_t *)s_latest_raw, s_latest_raw_len);
        /* PWM 屏：显示 CH1~CH5 输入值 + 对应输出脉宽 */
        ui_update_pwm_values(s_latest_values, s_latest_value_count);
        /* 手势识别屏：根据前 5 路数据匹配并显示手势图形 */
        ui_update_gesture_recv(s_latest_values, s_latest_value_count);
        /* 用 CH1~CH5（索引 0~4）驱动 5 路 PWM 硬件输出 */
        pwm_manager_update(s_latest_values, s_latest_value_count);
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
