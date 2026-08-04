#ifndef PWM_MANAGER_H
#define PWM_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== PWM 输出引脚定义 ======
 * 本工程已占用引脚：
 *   LCD  : 9, 10, 11, 12, 13, 14, 21(RST)
 *   Touch: 47(SDA), 48(SCL)
 *   PSRAM: 26~32（ESP32-S3 PSRAM 引脚，不可用）
 * 以下 5 路选用 GPIO4~8（通用 IO，无 strapping/flash 冲突），
 * 如需改用其它引脚，直接修改下面的宏即可。 */
#define PWM_PIN_CH1   4
#define PWM_PIN_CH2   5
#define PWM_PIN_CH3   6
#define PWM_PIN_CH4   7
#define PWM_PIN_CH5   8

/* PWM 通道数（CH1 ~ CH5） */
#define PWM_CHANNEL_COUNT   5

/* 阈值与输出脉宽（单位：微秒 us）
 * CH 值 > 阈值   -> 输出 2000us（满量程正向）
 * CH 值 <= 阈值  -> 输出 1000us（反向/最低）
 * 1000~2000us 为标准舵机 / ESC PWM 脉宽范围 */
#define PWM_VALUE_THRESHOLD   1650
#define PWM_OUT_HIGH_US       2000
#define PWM_OUT_LOW_US        1000

/* 初始化 5 路 PWM 输出（50Hz / 16bit 分辨率，舵机 PWM）。
 * 必须在 setup() 中调用一次。 */
void pwm_manager_init(void);

/* 根据 11 路通道数据更新 5 路 PWM 输出。
 * values: ble_manager_parse_frame 解析出的数组（11 路）
 * count : 数组有效元素个数（需 >= PWM_CHANNEL_COUNT 才会更新）
 * 取 values[0..4] 对应 CH1~CH5。 */
void pwm_manager_update(const int16_t *values, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif
