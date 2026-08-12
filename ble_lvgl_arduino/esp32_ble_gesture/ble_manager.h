#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_SCAN_RESULT_MAX  20
#define BLE_DATA_BUFFER_SIZE 512

/* 一帧数据中数值的个数（11 路通道） */
#define BLE_DATA_VALUE_COUNT  11

/* 自定义厂商 ID，用于标识广播包中的 UUID+SN 数据 */
#define BLE_CUSTOM_MFR_ID  0xFF00

/* BLE 广播设备名称：写入广播包，方便手机/小程序扫描时识别本设备 */
#define BLE_DEVICE_NAME    "ExoHand-BLE"

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_SCANNING,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTED,
} ble_state_t;

typedef struct {
    uint8_t mac[6];
    uint8_t addr_type;
    char name[32];
    int8_t rssi;
    /* 自定义设备标识（从广播包 Manufacturer Data 解析） */
    bool has_custom_id;       /* 是否包含自定义 UUID+SN */
    uint32_t custom_uuid;     /* 4 字节自定义 UUID */
    uint32_t sn;              /* 4 字节设备序列号 */
} ble_device_info_t;

typedef void (*ble_data_callback_t)(uint8_t *data, uint16_t len);
typedef void (*ble_state_callback_t)(ble_state_t state, const char *message);

/* 连接目标设备的 GATT UUID 配置。
 * 字段为 hex 字符串：16 位如 "FFE0"、32 位如 "0000FFE0"、
 * 或 128 位如 "0000FFE0-0000-1000-8000-00805F9B34FB"。
 * 默认值（BLE 串口模块常见）：service=FFE0, notify=FFE2, write=FFE1 */
typedef struct {
    char service[40];   /* 服务 UUID */
    char notify[40];    /* 通知特征 UUID（设备 -> 本机，接收数据） */
    char write[40];      /* 写特征 UUID（本机 -> 设备，发送数据） */
} ble_uuid_config_t;

void ble_manager_init(void);
void ble_manager_start_scan(void);
void ble_manager_stop_scan(void);

/* 连接指定设备。
 * uuids 非空时按指定服务/特征 UUID 精确查找；
 * uuids 为空时回退到自动选取第一个可通知/可写特征。 */
void ble_manager_connect(const ble_device_info_t *device, const ble_uuid_config_t *uuids);
void ble_manager_disconnect(void);
void ble_manager_send_data(uint8_t *data, uint16_t len);
ble_state_t ble_manager_get_state(void);
void ble_manager_get_scan_results(ble_device_info_t *results, uint16_t *count);
void ble_manager_set_data_callback(ble_data_callback_t cb);
void ble_manager_set_state_callback(ble_state_callback_t cb);
void ble_manager_process_data(void);

/* 在主 loop 中调用：处理 BLE 回调中设置的待处理状态变更。
 * 将 BLE 任务中的 UI 更新延迟到主任务执行，避免 LVGL 互斥锁死锁。 */
void ble_manager_process_state(void);

/* 解析一帧完整数据 "v1,v2,...,v11;" 为整数数组。
 * data/len 由 ble_manager_process_data 在帧重组完成后通过 data 回调传入。
 * 返回实际解析出的数值个数（<=max_count），出错或无数据返回 0。 */
uint8_t ble_manager_parse_frame(const uint8_t *data, uint16_t len,
                                int16_t *values, uint8_t max_count);

#ifdef __cplusplus
}
#endif

#endif
