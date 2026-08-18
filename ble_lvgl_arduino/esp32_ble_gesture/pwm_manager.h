#ifndef PWM_MANAGER_H
#define PWM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== PWM 输出引脚定义 ======
 * 本工程已占用引脚：
 *   LCD  : 9, 10, 11, 12, 13, 14, 21(RST)
 *   Touch: 47(SDA), 48(SCL)
 *   PSRAM: 26~32（ESP32-S3 PSRAM 引脚，不可用）
 * PWM 占用 GPIO2,3,5,6,7,8（通用 IO，无 flash 冲突）。
 * 注意：GPIO3 为 strapping 引脚（仅决定 JTAG 源），上电后即可作普通
 *   PWM 输出；保证上电瞬间不被外部强下拉即可。
 * 如需改用其它引脚，直接修改下面的宏即可。 */
#define PWM_PIN_CH1   3
#define PWM_PIN_CH2   5
#define PWM_PIN_CH3   6
#define PWM_PIN_CH4   7
#define PWM_PIN_CH5   8
#define PWM_PIN_CH6   2

/* PWM 硬件输出通道总数（CH1 ~ CH6） */
#define PWM_CHANNEL_COUNT       6
/* 直接映射 BLE 输入值的通道数（CH1~CH5）。
 * CH6 不在此列：其输出由 CH1 输入值派生（见下）。 */
#define PWM_DIRECT_CH_COUNT     5

/* 阈值与输出脉宽（单位：微秒 us）
 * CH1~CH5：CH 值 < 阈值  -> 2000us；CH 值 > 阈值 -> 1000us；CH 值 = 阈值 -> 1500us
 * CH6    ：由 CH1 输入值派生 —— CH1 高(>阈值) -> 1750us；CH1 低(<=阈值) -> 1400us
 *          （CH6 基于 CH1 的输入值而非输出值，故点击 CH1 覆盖不影响 CH6）
 * 手动覆盖 / 全局急停 -> 输出 1500us（中位/停转）
 * 1000~2000us 为标准舵机 / ESC PWM 脉宽范围
 * 注意：PWM_VALUE_THRESHOLD 必须写成十进制 650，不能写成 0650
 *       （C/C++ 中前导 0 表示八进制，0650 = 424 十进制，会导致阈值错误）。 */
#define PWM_VALUE_THRESHOLD   650
#define PWM_OUT_HIGH_US       2000
#define PWM_OUT_LOW_US        1000
#define PWM_OUT_MID_US        1500
#define PWM_OUT_CH6_HIGH_US   1750
#define PWM_OUT_CH6_LOW_US    1400

/* 初始化 6 路 PWM 输出（50Hz / 16bit 分辨率，舵机 PWM）。
 * 必须在 setup() 中调用一次。 */
void pwm_manager_init(void);

/* 根据 11 路通道数据更新 6 路 PWM 输出。
 * values: ble_manager_parse_frame 解析出的数组（11 路）
 * count : 数组有效元素个数
 * CH1~CH5 取 values[0..4] 直接映射（需 count >= 5）。
 * CH6 取 values[0]（CH1 输入值）派生为 1750/1400us（需 count >= 1）。
 * 若某通道处于手动覆盖状态，则直接输出 1500us，不受 CH 值影响。 */
void pwm_manager_update(const int16_t *values, uint8_t count);

/* 手动覆盖：设置/获取/切换某通道的覆盖状态。
 * ch: 通道索引 0~5（对应 CH1~CH6）。
 * 覆盖时该通道直接输出 1500us（中位），不受 CH 值影响。
 * CH1 与 CH6 覆盖状态相互独立：点击 CH1 不影响 CH6，反之亦然。 */
void pwm_manager_set_override(uint8_t ch, bool override);
bool pwm_manager_get_override(uint8_t ch);
void pwm_manager_toggle_override(uint8_t ch);

/* 全局急停：开启后所有通道强制输出 1500us，优先级高于 CH 值与单通道覆盖。
 * 关闭后恢复各通道正常行为（自动跟随 CH 值或单通道覆盖）。 */
void pwm_manager_set_estop(bool on);
bool pwm_manager_get_estop(void);

/* 自主训练（手势）模式：直接指定 6 路输出脉宽，忽略 BLE 输入映射与单通道覆盖。
 * 优先级：急停 > 手势模式 > 单通道覆盖 > BLE 自动映射。
 * 进入手势屏（"自主训练"按钮）时开启，点击"扫描设备"恢复 BLE 模式时关闭。
 * 开启时保留当前输出（避免突变），后续由 pwm_manager_set_gesture_outputs() 设置。 */
void pwm_manager_set_gesture_mode(bool on);
bool pwm_manager_get_gesture_mode(void);

/* 设置手势模式的 6 路输出脉宽（us）并立即刷新硬件。
 * us  : 6 元素数组（CH1~CH6 脉宽，如 1750/1500/2000/1000 等）。
 * count: 数组有效长度（应 >= 6）。
 * 仅当手势模式开启时由 pwm_manager_update() 读取生效；急停开启时仍强制 1500us。
 * 可在任意时刻调用（未开启手势模式时仅缓存，不刷新硬件）。 */
void pwm_manager_set_gesture_outputs(const uint16_t *us, uint8_t count);

/* ====== 力度调节（Force Level）======
 * 滑块数值 1~10，默认 10。影响 CH1~CH5 的高/低档输出脉宽：
 *   高档 = 1500 + level*50，低档 = 1500 - level*50
 *   (1:1550,1450) (2:1600,1400) ... (10:2000,1000)
 * CH6 的 1750/1400 不受力度调节影响。
 * 初始姿态（回归姿势）和急停也不受理度调节影响。 */
void pwm_manager_set_force_level(uint8_t level);
uint8_t pwm_manager_get_force_level(void);
uint16_t pwm_manager_get_high_us(void);
uint16_t pwm_manager_get_low_us(void);

/* ====== 行程调节（Stroke Level）======
 * 滑块数值 1~8。仅影响训练模式手势输出的持续时间：
 *   (1:1s) (2:1.5s) (3:2s) (4:2.5s) (5:3s) (6:3.5s) (7:4s) (8:4.5s)
 * 到达时间后 6 路 PWM 回归 1500us。
 * 初始姿态和急停不受行程调节影响（无定时）。 */
void pwm_manager_set_stroke_level(uint8_t level);
uint8_t pwm_manager_get_stroke_level(void);
uint32_t pwm_manager_get_stroke_duration_ms(void);

/* 设置手势输出并按行程时间定时（仅 training 模式手势点击使用）。
 * rest_pose=true 时不启动定时（初始姿态/回归姿势不受行程影响）。
 * 到达行程时间后 6 路 PWM 自动回归 1500us，并触发 expired 回调。 */
void pwm_manager_set_gesture_outputs_timed(const uint16_t *us, uint8_t count, bool rest_pose);

/* 手势定时到期回调：在 pwm_manager_tick() 检测到定时到期、
 * 6 路已回归 1500us 后被调用，用于通知 UI 更新显示。 */
typedef void (*pwm_gesture_expired_cb_t)(void);
void pwm_manager_set_gesture_expired_cb(pwm_gesture_expired_cb_t cb);

/* 在主循环中周期调用：检查手势定时是否到期，到期则 6 路回归 1500us。 */
void pwm_manager_tick(void);

#ifdef __cplusplus
}
#endif

#endif
