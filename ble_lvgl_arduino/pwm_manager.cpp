#include "pwm_manager.h"
#include <Arduino.h>
#include "esp_log.h"

static const char *TAG = "PWM";

/* 50Hz 周期 = 20000us，16bit 分辨率 = 65536 ticks
 * 1us = 65536 / 20000 = 3.2768 ticks
 * 1000us -> 3277, 2000us -> 6554 */
#define PWM_FREQ_HZ         50
#define PWM_RESOLUTION_BITS 16
#define PWM_PERIOD_US       20000
#define PWM_MAX_DUTY        65535

/* 5 路 PWM 引脚表（与 PWM_PIN_CH1..CH5 一一对应） */
static const uint8_t s_pwm_pins[PWM_CHANNEL_COUNT] = {
    PWM_PIN_CH1,
    PWM_PIN_CH2,
    PWM_PIN_CH3,
    PWM_PIN_CH4,
    PWM_PIN_CH5,
};

/* 当前每路输出脉宽（us），用于调试/日志输出 */
static uint16_t s_pwm_us[PWM_CHANNEL_COUNT] = {0};

/* 微秒 -> 16bit 占空比 tick 数 */
static inline uint32_t pwm_us_to_duty(uint16_t us)
{
    if (us > PWM_PERIOD_US) us = PWM_PERIOD_US;
    return (uint32_t)us * (PWM_MAX_DUTY + 1) / PWM_PERIOD_US;
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
        /* 上电默认输出低脉宽 1000us（避免舵机抖动到中位） */
        uint32_t duty = pwm_us_to_duty(PWM_OUT_LOW_US);
        ledcWrite(pin, duty);
        s_pwm_us[i] = PWM_OUT_LOW_US;
        ESP_LOGI(TAG, "CH%u init: pin=%u, 50Hz/%ubit, default=%uus(duty=%lu)",
                 i + 1, pin, PWM_RESOLUTION_BITS, PWM_OUT_LOW_US, (unsigned long)duty);
    }
}

void pwm_manager_update(const int16_t *values, uint8_t count)
{
    if (!values) return;
    if (count < PWM_CHANNEL_COUNT) {
        /* 数据不足 5 路，跳过本次更新（保持上一次输出） */
        return;
    }

    for (uint8_t i = 0; i < PWM_CHANNEL_COUNT; i++) {
        int16_t v = values[i];
        uint16_t us = (v > PWM_VALUE_THRESHOLD) ? PWM_OUT_HIGH_US : PWM_OUT_LOW_US;

        if (us != s_pwm_us[i]) {
            /* 仅在脉宽变化时写硬件，减少 LEDC 寄存器访问 */
            uint32_t duty = pwm_us_to_duty(us);
            ledcWrite(s_pwm_pins[i], duty);
            s_pwm_us[i] = us;
            ESP_LOGI(TAG, "CH%u v=%d -> %uus (duty=%lu)",
                     i + 1, v, us, (unsigned long)duty);
        }
    }
}
