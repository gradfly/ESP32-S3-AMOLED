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

/* 手动覆盖标志：为 true 的通道直接输出 1500us，不受 CH 值影响 */
static bool s_override[PWM_CHANNEL_COUNT] = {false};

/* 全局急停标志：为 true 时所有通道强制输出 1500us，优先级最高 */
static bool s_estop = false;

/* 手势（自主训练）模式：直接指定 6 路输出脉宽，忽略 BLE 输入与单通道覆盖。
 * 优先级：急停 > 手势模式 > 单通道覆盖 > BLE 自动映射。 */
static bool s_gesture_mode = false;
static uint16_t s_gesture_us[PWM_CHANNEL_COUNT] = {0};

/* 微秒 -> 14bit 占空比 tick 数 */
static inline uint32_t pwm_us_to_duty(uint16_t us)
{
    if (us > PWM_PERIOD_US) us = PWM_PERIOD_US;
    return (uint32_t)us * (PWM_MAX_DUTY + 1) / PWM_PERIOD_US;
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
    if (s_gesture_mode == on) return;
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
    ESP_LOGI(TAG, "Gesture mode %s", on ? "ON" : "OFF");
}

bool pwm_manager_get_gesture_mode(void)
{
    return s_gesture_mode;
}

void pwm_manager_set_gesture_outputs(const uint16_t *us, uint8_t count)
{
    if (!us) return;
    uint8_t n = (count < PWM_CHANNEL_COUNT) ? count : PWM_CHANNEL_COUNT;
    for (uint8_t i = 0; i < n; i++) {
        s_gesture_us[i] = us[i];
    }
    /* 触发硬件刷新：手势模式开启时按 s_gesture_us 输出；
     * 未开启时仅缓存（update 会走 BLE/覆盖分支，不读 s_gesture_us）。 */
    pwm_manager_update(NULL, 0);
}

void pwm_manager_init(void)
{
    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        uint8_t pin = s_pwm_pins[i];
        /* ledcAttach(pin, freq, resolution) 返回 bool（Arduino-ESP32 3.x API） */
        bool ok = ledcAttach(pin, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
        if (!ok) {
            ESP_LOGE(TAG, "ledcAttach failed: ch=%u pin=%u", i + 1, pin);
            continue;
        }
        /* 上电默认输出各自"高"脉宽，避免舵机抖动到中位：
         * CH1~CH5 -> 2000us，CH6 -> 1750us */
        uint16_t default_us = (i < PWM_DIRECT_CH_COUNT) ? PWM_OUT_HIGH_US
                                                        : PWM_OUT_CH6_HIGH_US;
        uint32_t duty = pwm_us_to_duty(default_us);
        ledcWrite(pin, duty);
        s_pwm_us[i] = default_us;
        ESP_LOGI(TAG, "CH%u init: pin=%u, 50Hz/%ubit, default=%uus(duty=%lu)",
                 i + 1, pin, PWM_RESOLUTION_BITS, default_us, (unsigned long)duty);
    }
}

void pwm_manager_update(const int16_t *values, uint8_t count)
{
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
            us = s_gesture_us[i];       /* 手势模式：直接指定脉宽 */
            tag = "[GESTURE]";
        } else if (s_override[i]) {
            us = PWM_OUT_MID_US;        /* 单通道覆盖：1500us */
            tag = "[OVR]";
        } else if (i < PWM_DIRECT_CH_COUNT) {
            /* CH1~CH5：直接映射各自 BLE 输入值
             * <650 -> 2000us，>650 -> 1000us，=650 -> 1500us */
            if (!has_5) continue;       /* 数据不足，保持上一次输出 */
            int16_t v = values[i];
            if (v < PWM_VALUE_THRESHOLD) {
                us = PWM_OUT_HIGH_US;
            } else if (v > PWM_VALUE_THRESHOLD) {
                us = PWM_OUT_LOW_US;
            } else {
                us = PWM_OUT_MID_US;
            }
            tag = "auto";
        } else {
            /* CH6：由 CH1 输入值派生（1750/1400us）。
             * 基于 CH1 输入值而非输出值，故点击 CH1 覆盖不影响 CH6。 */
            if (!has_ch1) continue;     /* 无 CH1 数据，保持上一次输出 */
            int16_t v0 = values[0];
            us = (v0 > PWM_VALUE_THRESHOLD) ? PWM_OUT_CH6_HIGH_US : PWM_OUT_CH6_LOW_US;
            tag = "auto6";
        }

        if (us != s_pwm_us[i]) {
            /* 仅在脉宽变化时写硬件，减少 LEDC 寄存器访问 */
            uint32_t duty = pwm_us_to_duty(us);
            ledcWrite(s_pwm_pins[i], duty);
            s_pwm_us[i] = us;
            ESP_LOGI(TAG, "CH%u %s -> %uus (duty=%lu)",
                     i + 1, tag, us, (unsigned long)duty);
        }
    }
}
