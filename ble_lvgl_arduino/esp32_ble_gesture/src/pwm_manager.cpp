#include "pwm_manager.h"
#include <Arduino.h>
#include "esp_log.h"

static const char *TAG = "PWM";

/* 50Hz 周期 = 20000us，14bit 分辨率 = 16384 ticks
 * 1us = 16384 / 20000 = 0.8192 ticks
 * 1000us -> 819, 1500us -> 1228, 2000us -> 1638
 * 注意：Arduino-ESP32 3.x 的 ledcAttach() 对 ESP32-S3 的分辨率上限
 *       为 14 位（仅原版 ESP32 支持 1~20 位）。使用 16 位会导致
 *       ledcAttach() 返回 false，PWM 无输出。 */
#define PWM_FREQ_HZ         50
#define PWM_RESOLUTION_BITS 14
#define PWM_PERIOD_US       20000
#define PWM_MAX_DUTY        16383

/* 6 路 PWM 引脚表（与 PWM_PIN_CH1..CH6 一一对应） */
static const uint8_t s_pwm_pins[PWM_CHANNEL_COUNT] = {
    PWM_PIN_CH1,
    PWM_PIN_CH2,
    PWM_PIN_CH3,
    PWM_PIN_CH4,
    PWM_PIN_CH5,
    PWM_PIN_CH6,
};

/* 当前每路输出脉宽（us），用于调试/日志输出 */
static uint16_t s_pwm_us[PWM_CHANNEL_COUNT] = {0};

/* ledcAttach may fail during startup if another peripheral still owns a
 * channel. Keep the state so a later BLE frame can recover the output. */
static bool s_pwm_attached[PWM_CHANNEL_COUNT] = {false};

/* 手动覆盖标志：为 true 的通道直接输出 1500us，不受 CH 值影响 */
static bool s_override[PWM_CHANNEL_COUNT] = {false};

/* 全局急停标志：为 true 时所有通道强制输出 1500us，优先级最高 */
static bool s_estop = false;

/* 手势（自主训练）模式：直接指定 6 路输出脉宽，忽略 BLE 输入与单通道覆盖。
 * 优先级：急停 > 手势模式 > 单通道覆盖 > BLE 自动映射。 */
static bool s_gesture_mode = false;
static uint16_t s_gesture_us[PWM_CHANNEL_COUNT] = {0};

/* 力度调节：1~10，默认 10。
 * 高档 = 1500 + level*50，低档 = 1500 - level*50 */
static uint8_t s_force_level = 10;

/* 行程调节：1~10，默认 8。
 * 持续时间 = (level + 1) * 500 ms  → 1:1s 2:1.5s ... 10:5.5s */
static uint8_t s_stroke_level = 8;

/* 手势定时：点击手势后按行程时间输出，到期后 6 路回归 1500us。
 * rest_pose（初始姿态/回归姿势）不启动定时。 */
static bool s_gesture_timed = false;
static uint32_t s_gesture_deadline_ms = 0;
static pwm_gesture_expired_cb_t s_expired_cb = NULL;

/* 递归互斥锁：保护 s_gesture_us[] / s_gesture_timed 等共享状态。
 * LVGL 任务（手势点击回调）与主循环（pwm_manager_tick 定时到期）
 * 在 ESP32-S3 双核上并发运行，若不加锁，定时到期会将 s_gesture_us[]
 * 部分覆盖为 1500us，导致手势切换后部分通道不生效。
 * 使用递归锁：pwm_manager_tick() 内部调用 pwm_manager_set_gesture_outputs()。 */
static SemaphoreHandle_t s_mutex = NULL;

static void pwm_lock(void)
{
    if (s_mutex) xSemaphoreTakeRecursive(s_mutex, portMAX_DELAY);
}

static void pwm_unlock(void)
{
    if (s_mutex) xSemaphoreGiveRecursive(s_mutex);
}

/* 微秒 -> 14bit 占空比 tick 数 */
static inline uint32_t pwm_us_to_duty(uint16_t us)
{
    if (us > PWM_PERIOD_US) us = PWM_PERIOD_US;
    return (uint32_t)us * (PWM_MAX_DUTY + 1) / PWM_PERIOD_US;
}

static bool pwm_write(uint8_t index, uint16_t us)
{
    if (index >= PWM_CHANNEL_COUNT) return false;

    uint8_t pin = s_pwm_pins[index];
    if (!s_pwm_attached[index]) {
        s_pwm_attached[index] = ledcAttach(pin, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        if (!s_pwm_attached[index]) {
            ESP_LOGE(TAG, "LEDC attach failed: CH%u pin=%u", index + 1, pin);
            return false;
        }
        ESP_LOGI(TAG, "LEDC attach recovered: CH%u pin=%u", index + 1, pin);
    }

    uint32_t duty = pwm_us_to_duty(us);
    if (!ledcWrite(pin, duty)) {
        s_pwm_attached[index] = false;
        ESP_LOGE(TAG, "LEDC write failed: CH%u pin=%u duty=%lu",
                 index + 1, pin, (unsigned long)duty);
        return false;
    }
    return true;
}

void pwm_manager_set_override(uint8_t ch, bool override)
{
    if (ch < PWM_CHANNEL_COUNT) {
        s_override[ch] = override;
        /* 覆盖状态变化后，强制下次 update 重写硬件（即使脉宽恰好相同） */
        s_pwm_us[ch] = 0;
        ESP_LOGI(TAG, "CH%u override %s", ch + 1, override ? "ON -> 1500us" : "OFF");
    }
}

bool pwm_manager_get_override(uint8_t ch)
{
    return (ch < PWM_CHANNEL_COUNT) ? s_override[ch] : false;
}

void pwm_manager_toggle_override(uint8_t ch)
{
    if (ch < PWM_CHANNEL_COUNT) {
        pwm_manager_set_override(ch, !s_override[ch]);
    }
}

void pwm_manager_set_estop(bool on)
{
    if (s_estop != on) {
        s_estop = on;
        /* 急停状态变化后，强制下次 update 重写所有硬件（即使脉宽恰好相同） */
        for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
            s_pwm_us[i] = 0;
        }
        ESP_LOGI(TAG, "E-STOP %s -> all channels 1500us", on ? "ON" : "OFF");
    }
}

bool pwm_manager_get_estop(void)
{
    return s_estop;
}

void pwm_manager_set_gesture_mode(bool on)
{
    pwm_lock();
    if (s_gesture_mode == on) { pwm_unlock(); return; }
    s_gesture_mode = on;
    if (on) {
        /* 进入手势模式：保留当前输出，避免舵机突变 */
        for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
            s_gesture_us[i] = s_pwm_us[i];
        }
    }
    /* 强制下次 update 重写所有硬件（模式切换） */
    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        s_pwm_us[i] = 0;
    }
    pwm_unlock();
    ESP_LOGI(TAG, "Gesture mode %s", on ? "ON" : "OFF");
}

bool pwm_manager_get_gesture_mode(void)
{
    return s_gesture_mode;
}

uint16_t pwm_manager_get_gesture_us(uint8_t ch)
{
    return (ch < PWM_CHANNEL_COUNT) ? s_gesture_us[ch] : PWM_OUT_MID_US;
}

void pwm_manager_set_gesture_outputs(const uint16_t *us, uint8_t count)
{
    if (!us) return;
    uint8_t n = (count < PWM_CHANNEL_COUNT) ? count : PWM_CHANNEL_COUNT;
    pwm_lock();

    /* 检测任意通道 PWM 值是否发生变化 */
    bool any_changed = false;
    for (uint8_t i = 0; i < n; i++) {
        if (s_gesture_us[i] != us[i]) {
            any_changed = true;
            break;
        }
    }

    /* 更新期望输出值 */
    for (uint8_t i = 0; i < n; i++) {
        s_gesture_us[i] = us[i];
    }

    /* 直接写硬件，确保 PWM 输出实时生效 */
    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        uint16_t out_us = s_estop ? PWM_OUT_MID_US : s_gesture_us[i];
        if (pwm_write(i, out_us)) {
            s_pwm_us[i] = out_us;
        }
    }

    /* 任意 PWM 值发生变化时重新计时 */
    if (any_changed && s_gesture_mode && !s_estop) {
        s_gesture_deadline_ms = millis() + pwm_manager_get_stroke_duration_ms();
        s_gesture_timed = true;
        ESP_LOGI(TAG, "PWM changed -> restart gesture timer: %lu ms",
                 (unsigned long)pwm_manager_get_stroke_duration_ms());
    }

    pwm_unlock();
}

void pwm_manager_init(void)
{
    s_mutex = xSemaphoreCreateRecursiveMutex();
    ESP_LOGI(TAG, "PWM mutex created: %s", s_mutex ? "OK" : "FAIL");

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        uint8_t pin = s_pwm_pins[i];
        /* ledcAttach(pin, freq, resolution) 返回 bool（Arduino-ESP32 3.x API） */
        bool ok = ledcAttach(pin, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        if (!ok) {
            ESP_LOGE(TAG, "ledcAttach failed: ch=%u pin=%u", i + 1, pin);
            continue;
        }
        s_pwm_attached[i] = true;
        /* 上电默认输出中位 1500us（停转），所有 6 路一致 */
        uint16_t default_us = PWM_OUT_MID_US;
        uint32_t duty = pwm_us_to_duty(default_us);
        if (!ledcWrite(pin, duty)) {
            s_pwm_attached[i] = false;
            ESP_LOGE(TAG, "Initial LEDC write failed: ch=%u pin=%u", i + 1, pin);
            /* ledcWrite 失败时不能设置 s_pwm_us[i]，
             * 否则软件认为已输出 1500us 但硬件未写入，
             * 后续 us==s_pwm_us[i] 时跳过写入导致舵机不动 */
            continue;
        }
        s_pwm_us[i] = default_us;
        ESP_LOGI(TAG, "CH%u init: pin=%u, 50Hz/%ubit, default=%uus(duty=%lu)",
                 i + 1, pin, PWM_RESOLUTION_BITS, default_us, (unsigned long)duty);
    }
}

void pwm_manager_update(const int16_t *values, uint8_t count)
{
    /* 加锁保护 s_pwm_us[] / s_gesture_us[] / s_estop / s_gesture_mode 等共享状态，
     * 防止 LVGL 任务中的手势回调与主 loop 并发修改导致竞态。
     * 使用递归锁：pwm_manager_set_gesture_outputs() 等已持锁调用方可安全重入。 */
    pwm_lock();

    /* CH1~CH5 需要 count>=5 才能自动更新；CH6 仅需 CH1 输入值(count>=1)。
     * 覆盖/急停通道不受数据是否就绪影响（直接输出 1500us）。 */
    bool has_5   = (values && count >= PWM_DIRECT_CH_COUNT);
    bool has_ch1 = (values && count >= 1);

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        /* 优先级：急停 > 手势模式 > 单通道覆盖 > 自动跟随 CH 值 */
        uint16_t us;
        const char *tag;
        if (s_estop) {
            us = PWM_OUT_MID_US;        /* 急停：1500us，所有通道 */
            tag = "[ESTOP]";
        } else if (s_gesture_mode) {
            /* 手势模式下，PWM 输出由 s_gesture_us[] 驱动。
             * 无论 values 是否为 NULL 都写硬件：
             * - NULL 调用(来自 set_gesture_outputs)：s_gesture_us[] 刚被更新，s_pwm_us[] 已置 0，强制写入新值
             * - values!=NULL 调用(来自 loop)：s_gesture_us[] 已由 ui_update_gesture_recv 更新，
             *   若 Phase 1 因手势模式刚开启而跳过匹配，此处作为兜底确保硬件写入，
             *   避免"屏幕显示正常但舵机不动"的问题。
             * us==s_pwm_us[i] 时自动跳过(避免重复写寄存器)。 */
            us = s_gesture_us[i];
            tag = "[GESTURE]";
        } else if (s_override[i]) {
            us = PWM_OUT_MID_US;        /* 单通道覆盖：1500us */
            tag = "[OVR]";
        } else if (i < PWM_DIRECT_CH_COUNT) {
            /* CH1~CH5：直接映射各自 BLE 输入值
             * <650 -> 高档，>650 -> 低档，=650 -> 1500us
             * 高/低档由力度调节滑块决定（默认 2000/1000） */
            if (!has_5) continue;       /* 数据不足，保持上一次输出 */
            int16_t v = values[i];
            if (v < PWM_VALUE_THRESHOLD) {
                us = pwm_manager_get_high_us();
            } else if (v > PWM_VALUE_THRESHOLD) {
                us = pwm_manager_get_low_us();
            } else {
                us = PWM_OUT_MID_US;
            }
            tag = "auto";
        } else {
            /* CH6：由 CH1 输入值派生（1350/1800us）。
             * 基于 CH1 输入值而非输出值，故点击 CH1 覆盖不影响 CH6。 */
            if (!has_ch1) continue;     /* 无 CH1 数据，保持上一次输出 */
            int16_t v0 = values[0];
            us = (v0 > PWM_VALUE_THRESHOLD) ? PWM_OUT_CH6_HIGH_US : PWM_OUT_CH6_LOW_US;
            tag = "auto6";
        }

        /* 始终写硬件，确保 PWM 输出实时生效。
         * 之前 us==s_pwm_us[i] 时跳过写入，但 s_pwm_us[i] 可能与硬件
         * 实际状态不一致（如 init 时 ledcWrite 失败但 s_pwm_us 被赋值），
         * 导致期望值变化但舵机不动，必须 toggle override 才能恢复。
         * ledcWrite 仅更新 duty 寄存器（微秒级），6 路×20Hz 开销可忽略。 */
        if (pwm_write(i, us)) {
            uint32_t duty = pwm_us_to_duty(us);
            s_pwm_us[i] = us;
            ESP_LOGI(TAG, "CH%u %s -> %uus (duty=%lu)",
                     i + 1, tag, us, (unsigned long)duty);
        }
    }

    pwm_unlock();
}

/* ====== 力度调节 ====== */
void pwm_manager_set_force_level(uint8_t level)
{
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    if (s_force_level == level) return;
    s_force_level = level;
    /* 高/低档变化后，强制下次 update 重写所有硬件（即使脉宽恰好相同） */
    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        s_pwm_us[i] = 0;
    }
    ESP_LOGI(TAG, "Force level -> %u (high=%uus, low=%uus)",
             level, pwm_manager_get_high_us(), pwm_manager_get_low_us());
}

uint8_t pwm_manager_get_force_level(void)
{
    return s_force_level;
}

uint16_t pwm_manager_get_high_us(void)
{
    return (uint16_t)(1500 + s_force_level * 50);
}

uint16_t pwm_manager_get_low_us(void)
{
    return (uint16_t)(1500 - s_force_level * 50);
}

/* ====== 行程调节 ====== */
void pwm_manager_set_stroke_level(uint8_t level)
{
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    if (s_stroke_level == level) return;
    s_stroke_level = level;
    ESP_LOGI(TAG, "Stroke level -> %u (%lu ms)",
             level, (unsigned long)pwm_manager_get_stroke_duration_ms());
}

uint8_t pwm_manager_get_stroke_level(void)
{
    return s_stroke_level;
}

uint32_t pwm_manager_get_stroke_duration_ms(void)
{
    /* 1->1000ms, 2->1500ms, ... 10->5500ms */
    return (uint32_t)(s_stroke_level + 1) * 500;
}

/* ====== 手势定时输出 ====== */
void pwm_manager_set_gesture_expired_cb(pwm_gesture_expired_cb_t cb)
{
    s_expired_cb = cb;
}

void pwm_manager_set_gesture_outputs_timed(const uint16_t *us, uint8_t count, bool rest_pose)
{
    /* 先取消正在运行的定时器，防止 pwm_manager_tick() 在更新过程中
     * 将 s_gesture_us[] 覆盖为 1500us（LVGL 任务与主循环并发的竞态）。 */
    pwm_lock();
    s_gesture_timed = false;
    pwm_unlock();

    /* 设置输出（立即刷新硬件） */
    pwm_manager_set_gesture_outputs(us, count);

    pwm_lock();
    if (rest_pose) {
        /* 初始姿态/回归姿势：不启动定时，持续输出 */
        s_gesture_timed = false;
    } else {
        /* 训练手势：启动定时，到期后 CH1~CH5 回归 1500us，CH6 保持当前值 */
        s_gesture_deadline_ms = millis() + pwm_manager_get_stroke_duration_ms();
        s_gesture_timed = true;
        ESP_LOGI(TAG, "Gesture timed: %lu ms -> then CH1~CH5 1500us, CH6 unchanged",
                 (unsigned long)pwm_manager_get_stroke_duration_ms());
    }
    pwm_unlock();
}

void pwm_manager_tick(void)
{
    pwm_lock();
    if (!s_gesture_timed) { pwm_unlock(); return; }
    /* 急停或手势模式关闭时，定时无意义（硬件已由急停/BLE 控制） */
    if (s_estop || !s_gesture_mode) {
        s_gesture_timed = false;
        pwm_unlock();
        return;
    }
    /* 检查是否到期 */
    if ((int32_t)(millis() - s_gesture_deadline_ms) >= 0) {
        /* CH1~CH5 回归 1500us；CH6 不受行程影响，保持当前值 */
        uint16_t mid[PWM_CHANNEL_COUNT];
        for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
            mid[i] = PWM_OUT_MID_US;
        }
        mid[5] = s_gesture_us[5];  /* CH6 保持当前值 */
        pwm_manager_set_gesture_outputs(mid, PWM_CHANNEL_COUNT);
        /* 定时到期后强制关闭定时器，覆盖 set_gesture_outputs() 内的重新计时 */
        s_gesture_timed = false;
        ESP_LOGI(TAG, "Gesture timed out -> CH1~CH5 1500us, CH6 unchanged");
        /* 释放锁后再回调，避免回调中的 UI 操作与锁产生死锁 */
        pwm_unlock();
        /* 通知 UI 更新 */
        if (s_expired_cb) {
            s_expired_cb();
        }
    } else {
        pwm_unlock();
    }
}
