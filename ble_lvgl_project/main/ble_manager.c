#include "ble_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include <string.h>

static const char *TAG = "BLE_MANAGER";

#define GATT_MAX_CHAR_COUNT 20
#define SCAN_DURATION_SECONDS 5

static ble_state_t s_state = BLE_STATE_IDLE;
static ble_device_info_t s_scan_results[BLE_SCAN_RESULT_MAX];
static uint16_t s_scan_result_count = 0;
static ble_data_callback_t s_data_cb = NULL;
static ble_state_callback_t s_state_cb = NULL;

static esp_gattc_char_elem_t s_char_elems[GATT_MAX_CHAR_COUNT];
static uint16_t s_char_count = 0;

static esp_bd_addr_t s_remote_bda = {0};
static int s_conn_id = 0;
static esp_gattc_if_t s_gattc_if = 0;
static bool s_is_connected = false;

static uint16_t s_rx_char_handle = 0;
static uint16_t s_tx_char_handle = 0;
static bool s_notify_enabled = false;

static uint16_t s_start_handle = 0;
static uint16_t s_end_handle = 0;

typedef struct {
    uint8_t data[BLE_DATA_BUFFER_SIZE];
    uint16_t len;
} ble_data_msg_t;

static QueueHandle_t s_data_queue = NULL;

static esp_ble_scan_params_t s_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
};

static void update_state(ble_state_t state, const char *message)
{
    s_state = state;
    if (s_state_cb) {
        s_state_cb(state, message);
    }
}

static void gap_cb(esp_ble_gap_cb_param_t *param)
{
    switch (param->gap_cb_evt) {
    case ESP_BLE_SCAN_START_COMPLETE_EVT:
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Scan started successfully");
            update_state(BLE_STATE_SCANNING, "Scanning...");
        } else {
            ESP_LOGE(TAG, "Scan start failed: %d", param->scan_start_cmpl.status);
            update_state(BLE_STATE_IDLE, "Scan failed");
        }
        break;

    case ESP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = param;
        if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            if (s_scan_result_count < BLE_SCAN_RESULT_MAX) {
                ble_device_info_t *dev = &s_scan_results[s_scan_result_count];
                memcpy(dev->mac, scan_result->scan_rst.bda, 6);
                dev->rssi = scan_result->scan_rst.rssi;

                if (scan_result->scan_rst.adv_data_len > 0) {
                    char *name = esp_ble_resolve_adv_data(scan_result->scan_rst.adv_data,
                                                          scan_result->scan_rst.adv_data_len,
                                                          ESP_BLE_AD_TYPE_NAME_CMPL);
                    if (name && strlen(name) > 0) {
                        strncpy(dev->name, name, sizeof(dev->name) - 1);
                        dev->name[sizeof(dev->name) - 1] = '\0';
                        esp_ble_free_resolved_adv_data(name);
                    } else {
                        strcpy(dev->name, "(no name)");
                    }
                } else {
                    strcpy(dev->name, "(no name)");
                }

                ESP_LOGI(TAG, "Found device: %s, RSSI: %d", dev->name, dev->rssi);
                s_scan_result_count++;
            }
        } else if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            ESP_LOGI(TAG, "Scan complete, found %d devices", s_scan_result_count);
            if (s_scan_result_count > 0) {
                update_state(BLE_STATE_IDLE, "Scan complete");
            } else {
                update_state(BLE_STATE_IDLE, "No devices found");
            }
        }
        break;
    }

    case ESP_BLE_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "Scan stopped");
        break;

    case ESP_BLE_AUTH_CMPL_EVT:
        ESP_LOGI(TAG, "Authentication complete");
        break;

    case ESP_BLE_GAP_CONGEST_EVT:
        break;

    case ESP_BLE_GAP_PASS_KEY_NOTIFY_EVT:
        break;

    case ESP_BLE_GAP_PIN_CODE_NOTIFY_EVT:
        break;

    case ESP_BLE_GAP_OOB_LEGACY_SET_COMPLETE_EVT:
        break;

    case ESP_BLE_GAP_ENCRYPT_EVT:
        break;

    case ESP_BLE_GAP_SEC_RECVD_EVT:
        break;

    case ESP_BLE_GAP_ENC_CHANGE_EVT:
        break;

    default:
        break;
    }
}

static void gattc_cb(esp_gattc_cb_param_t *p_data)
{
    switch (p_data->gattc_cb_evt) {
    case ESP_GATTC_CONNECT_EVT: {
        ESP_LOGI(TAG, "GATTC CONNECT, conn_id: %d", p_data->connect.conn_id);
        s_conn_id = p_data->connect.conn_id;
        memcpy(s_remote_bda, p_data->connect.remote_bda, 6);
        s_is_connected = true;
        update_state(BLE_STATE_CONNECTED, "Connected, discovering...");
        break;
    }

    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG, "GATTC DISCONNECT, reason: %d", p_data->disconnect.reason);
        s_is_connected = false;
        s_notify_enabled = false;
        s_rx_char_handle = 0;
        s_tx_char_handle = 0;
        s_start_handle = 0;
        s_end_handle = 0;
        update_state(BLE_STATE_DISCONNECTED, "Disconnected");
        break;

    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "Service search complete");

            uint16_t count = 0;
            esp_ble_gattc_get_attr_count(s_gattc_if, ESP_GATT_DB_CHAR,
                                         p_data->search_cmpl.start_handle,
                                         p_data->search_cmpl.end_handle,
                                         0, &count);

            s_start_handle = p_data->search_cmpl.start_handle;
            s_end_handle = p_data->search_cmpl.end_handle;

            if (count > 0 && count <= GATT_MAX_CHAR_COUNT) {
                memset(s_char_elems, 0, sizeof(s_char_elems));
                esp_ble_gattc_get_all_char(s_gattc_if,
                                           s_start_handle,
                                           s_end_handle,
                                           s_char_elems, &count, 0);
                s_char_count = count;

                for (int i = 0; i < s_char_count; i++) {
                    ESP_LOGI(TAG, "Char[%d]: handle=%x, prop=%x",
                             i, s_char_elems[i].handle, s_char_elems[i].properties);

                    if (s_char_elems[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        s_rx_char_handle = s_char_elems[i].handle;
                        esp_ble_gattc_register_for_notify(s_gattc_if, s_remote_bda, s_rx_char_handle);
                        ESP_LOGI(TAG, "Registered for notify on char handle: %x", s_rx_char_handle);
                    }
                    if (s_char_elems[i].properties & ESP_GATT_CHAR_PROP_BIT_WRITE) {
                        s_tx_char_handle = s_char_elems[i].handle;
                        ESP_LOGI(TAG, "Found writable char handle: %x", s_tx_char_handle);
                    }
                }
                update_state(BLE_STATE_CONNECTED, "Connected");
            } else {
                ESP_LOGW(TAG, "No characteristics found or count exceeds limit: %d", count);
                update_state(BLE_STATE_CONNECTED, "Connected (no chars)");
            }
        } else {
            ESP_LOGE(TAG, "Service search failed: %d", p_data->search_cmpl.status);
        }
        break;

    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
        if (p_data->reg_for_notify.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "Notify registration OK");
            esp_gattc_descr_elem_t descr_elem;
            uint16_t count = 0;
            esp_ble_gattc_get_descr_by_char_handle(s_gattc_if, s_rx_char_handle,
                                                    ESP_GATT_UUID_CHAR_DESCRIPTION,
                                                    &descr_elem);
            count = 1;
            if (count > 0 && descr_elem.type == ESP_GATT_DB_DESCRIPTOR) {
                uint16_t cccd_value = 1;
                esp_ble_gattc_write_char_descr(s_gattc_if, descr_elem.handle,
                                                sizeof(cccd_value), (uint8_t *)&cccd_value,
                                                ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_BUSY_TIMEOUT);
                s_notify_enabled = true;
                ESP_LOGI(TAG, "Notifications enabled via CCCD");
            } else {
                esp_ble_gattc_set_ctrl_value(s_gattc_if, s_rx_char_handle,
                                              ESP_GATT_CCC_NOTIFY, 1);
                s_notify_enabled = true;
                ESP_LOGI(TAG, "Notifications enabled via set_ctrl_value");
            }
        } else {
            ESP_LOGE(TAG, "Notify registration failed: %d", p_data->reg_for_notify.status);
        }
        break;

    case ESP_GATTC_SET_NOTIFY_ENABLE_EVT:
        if (p_data->set_notify_ena.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "Notify enabled successfully");
        }
        break;

    case ESP_GATTC_NOTIFY_EVT: {
        uint16_t value_len = p_data->notify.value_len;
        uint8_t *value = p_data->notify.value;

        if (s_data_queue && value_len > 0) {
            ble_data_msg_t msg;
            uint16_t copy_len = (value_len > BLE_DATA_BUFFER_SIZE) ? BLE_DATA_BUFFER_SIZE : value_len;
            memcpy(msg.data, value, copy_len);
            msg.len = copy_len;
            BaseType_t higher_priority_task_woken = pdFALSE;
            xQueueSendFromISR(s_data_queue, &msg, &higher_priority_task_woken);
        }
        break;
    }

    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write_char.status == ESP_GATT_OK) {
            ESP_LOGD(TAG, "Write characteristic OK");
        } else {
            ESP_LOGE(TAG, "Write characteristic failed: %d", p_data->write_char.status);
        }
        break;

    case ESP_GATTC_READ_CHAR_EVT:
        break;

    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write_descr.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "CCCD write OK");
        }
        break;

    default:
        break;
    }
}

void ble_manager_init(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_LOGI(TAG, "Initializing BLE...");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gap_register_callback(gap_cb);
    if (ret) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gattc_register_callback(gattc_cb);
    if (ret) {
        ESP_LOGE(TAG, "GATTC callback register failed: %s", esp_err_to_name(ret));
        return;
    }

    s_gattc_if = 0;
    ret = esp_ble_gattc_app_register(s_gattc_if);
    if (ret) {
        ESP_LOGE(TAG, "GATTC app register failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gap_set_scan_params(&s_scan_params);
    if (ret) {
        ESP_LOGW(TAG, "Set scan params failed: %s", esp_err_to_name(ret));
    }

    s_data_queue = xQueueCreate(20, sizeof(ble_data_msg_t));
    if (!s_data_queue) {
        ESP_LOGE(TAG, "Failed to create data queue");
    }

    ESP_LOGI(TAG, "BLE initialized successfully");
    update_state(BLE_STATE_IDLE, "BLE Ready");
}

void ble_manager_start_scan(void)
{
    if (s_state == BLE_STATE_SCANNING) {
        ESP_LOGW(TAG, "Already scanning");
        return;
    }

    s_scan_result_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));

    esp_ble_gap_start_scanning(SCAN_DURATION_SECONDS);
}

void ble_manager_stop_scan(void)
{
    esp_ble_gap_stop_scanning();
    update_state(BLE_STATE_IDLE, "Scan stopped");
}

void ble_manager_connect(uint8_t mac[6])
{
    if (s_is_connected) {
        ESP_LOGW(TAG, "Already connected");
        return;
    }

    memcpy(s_remote_bda, mac, 6);
    ESP_LOGI(TAG, "Connecting to %02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    update_state(BLE_STATE_CONNECTING, "Connecting...");
    esp_ble_gattc_open(s_remote_bda, ESP_BLE_ADDR_PUBLIC, true);
}

void ble_manager_disconnect(void)
{
    if (s_is_connected) {
        esp_ble_gattc_close(s_conn_id);
    }
}

void ble_manager_send_data(uint8_t *data, uint16_t len)
{
    if (!s_is_connected || s_tx_char_handle == 0) {
        ESP_LOGW(TAG, "Cannot send: not connected or no TX char");
        return;
    }

    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = (len - offset > 20) ? 20 : (len - offset);
        esp_ble_gattc_write_char(s_gattc_if, s_tx_char_handle, chunk,
                                  data + offset, ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_BUSY_TIMEOUT);
        offset += chunk;
    }
}

ble_state_t ble_manager_get_state(void)
{
    return s_state;
}

void ble_manager_get_scan_results(ble_device_info_t *results, uint16_t *count)
{
    uint16_t c = (s_scan_result_count < BLE_SCAN_RESULT_MAX) ? s_scan_result_count : BLE_SCAN_RESULT_MAX;
    for (uint16_t i = 0; i < c; i++) {
        memcpy(&results[i], &s_scan_results[i], sizeof(ble_device_info_t));
    }
    *count = c;
}

void ble_manager_set_data_callback(ble_data_callback_t cb)
{
    s_data_cb = cb;
}

void ble_manager_set_state_callback(ble_state_callback_t cb)
{
    s_state_cb = cb;
}

void ble_manager_process_data(void)
{
    if (!s_data_queue || !s_data_cb) return;

    ble_data_msg_t msg;
    while (xQueueReceive(s_data_queue, &msg, 0) == pdPASS) {
        s_data_cb(msg.data, msg.len);
    }
}
