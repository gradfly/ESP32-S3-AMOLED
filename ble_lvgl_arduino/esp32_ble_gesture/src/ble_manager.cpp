#include "ble_manager.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include <algorithm>
#include <cctype>
#include <string.h>

static const char *TAG = "BLE_MANAGER";

static ble_state_t s_state = BLE_STATE_IDLE;
static ble_device_info_t s_scan_results[BLE_SCAN_RESULT_MAX];
static uint16_t s_scan_result_count = 0;
static ble_data_callback_t s_data_cb = NULL;
static ble_state_callback_t s_state_cb = NULL;
static uint32_t s_scan_start_ms = 0;

/* 延迟状态更新：BLE 回调在 NimBLE 任务中运行，不能直接调用 ui_update_state（需获取
 * LVGL 互斥锁），否则可能死锁。改为在回调中记录待处理状态，由主 loop 统一处理。 */
static volatile bool s_state_pending = false;
static ble_state_t s_pending_state = BLE_STATE_IDLE;
static char s_pending_message[64] = "";

static NimBLEScan *s_scan = nullptr;
static NimBLEClient *s_client = nullptr;
static NimBLERemoteCharacteristic *s_rx_char = nullptr;
static NimBLERemoteCharacteristic *s_tx_char = nullptr;

/* BLE Server：让手机能连接 ESP32 并通过 Write(FFE1) 发送手势数据 */
static NimBLEServer          *s_server = nullptr;
static NimBLEService         *s_server_service = nullptr;
static NimBLECharacteristic  *s_server_notify_char = nullptr;   /* FFE2 */
static NimBLECharacteristic  *s_server_write_char = nullptr;    /* FFE1 */

static bool s_is_connected = false;
static uint8_t s_remote_mac[6] = {0};
static TaskHandle_t s_connect_task_handle = NULL;
static ble_device_info_t s_connect_device;
static ble_uuid_config_t s_connect_uuids;   /* 用户指定的目标 GATT UUID */
static bool s_have_uuids = false;           /* 是否按指定 UUID 连接 */

typedef struct {
    uint8_t data[BLE_DATA_BUFFER_SIZE];
    uint16_t len;
} ble_data_msg_t;

static QueueHandle_t s_data_queue = NULL;

/* 帧重组缓冲区：BLE notify 单次最多 20 字节（MTU=23 时），
 * 一帧 "1800,1600,...,0000;" 约 53 字节会被拆成多次 notify 到达，
 * 必须按帧尾 ';' 拼接成完整帧后再交给上层解析。
 */
static char s_frame_buf[128];
static uint16_t s_frame_len = 0;

static void update_state(ble_state_t state, const char *message)
{
    s_state = state;
    if (s_state_cb) {
        s_state_cb(state, message);
    }
}

static void notify_callback(NimBLERemoteCharacteristic *pCharacteristic,
                             uint8_t *data, size_t length, bool isNotify)
{
    if (isNotify && s_data_queue && s_data_cb) {
        ble_data_msg_t msg;
        uint16_t copy_len = (length > BLE_DATA_BUFFER_SIZE) ? BLE_DATA_BUFFER_SIZE : length;
        memcpy(msg.data, data, copy_len);
        msg.len = copy_len;
        BaseType_t higher_priority_task_woken = pdFALSE;
        xQueueSendFromISR(s_data_queue, &msg, &higher_priority_task_woken);
    }
}

/* BLE Server 连接/断开回调：手机连接上来时记录待处理状态，
 * 由主 loop 中的 ble_manager_process_state() 统一通知 UI 层。
 * 不能在此直接调用 update_state() → ui_update_state()，因为本回调运行在
 * NimBLE 任务上下文，ui_update_state 需获取 LVGL 互斥锁，可能导致死锁。 */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        s_is_connected = true;
        const uint8_t* mac = connInfo.getAddress().getVal();
        memcpy(s_remote_mac, mac, 6);
        ESP_LOGI(TAG, "BLE connected: MAC=%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
        Serial.printf("BLE connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
        /* 设置待处理状态，主 loop 中统一处理 UI 更新 */
        s_pending_state = BLE_STATE_CONNECTED;
        strncpy(s_pending_message, "BLE Connected", sizeof(s_pending_message) - 1);
        s_pending_message[sizeof(s_pending_message) - 1] = '\0';
        s_state_pending = true;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        s_is_connected = false;
        ESP_LOGI(TAG, "BLE disconnected, reason=%d", reason);
        Serial.printf("BLE disconnected, reason=%d\n", reason);
        /* 清空帧重组缓冲区 + 数据队列 */
        s_frame_len = 0;
        s_frame_buf[0] = '\0';
        if (s_data_queue) {
            ble_data_msg_t msg;
            while (xQueueReceive(s_data_queue, &msg, 0) == pdPASS) {}
        }
        /* 设置待处理状态，主 loop 中统一处理 UI 更新 */
        s_pending_state = BLE_STATE_DISCONNECTED;
        strncpy(s_pending_message, "BLE Disconnected", sizeof(s_pending_message) - 1);
        s_pending_message[sizeof(s_pending_message) - 1] = '\0';
        s_state_pending = true;
        /* 重新开启广播，允许下一次连接 */
        NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
        if (pAdv && !pAdv->isAdvertising()) {
            pAdv->start();
            Serial.println("BLE Advertising restarted for next connection");
        }
    }
};

/* BLE Server 写特征回调：手机通过 Write(FFE1) 发送的数据进入与 Notify 相同的处理管线 */
class ServerWriteCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string val = pCharacteristic->getValue();
        if (s_data_queue && val.length() > 0) {
            ble_data_msg_t msg;
            uint16_t copy_len = (val.length() > BLE_DATA_BUFFER_SIZE) ? BLE_DATA_BUFFER_SIZE : val.length();
            memcpy(msg.data, val.data(), copy_len);
            msg.len = copy_len;
            xQueueSend(s_data_queue, &msg, 0);
            Serial.printf("BLE Write: %u bytes\n", (unsigned)copy_len);
        }
    }
};

class MyAdvertisedDeviceCallbacks : public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
        /* 解析广播包中的 Manufacturer Data，提取自定义 UUID + SN
         * 数据格式: [MFR_ID(2)] [UUID(4)] [SN(4)] = 10 字节
         * MFR_ID = BLE_CUSTOM_MFR_ID (0xFF00)
         */
        bool has_custom_id = false;
        uint32_t custom_uuid = 0;
        uint32_t sn = 0;

        if (advertisedDevice->haveManufacturerData()) {
            std::string mfrData = advertisedDevice->getManufacturerData();
            if (mfrData.length() >= 10) {
                uint16_t mfrId = (uint8_t)mfrData[0] | ((uint8_t)mfrData[1] << 8);
                if (mfrId == BLE_CUSTOM_MFR_ID) {
                    has_custom_id = true;
                    memcpy(&custom_uuid, mfrData.data() + 2, 4);
                    memcpy(&sn, mfrData.data() + 6, 4);
                }
            }
        }

        /* 去重：如果设备带自定义 ID，按 UUID+SN 去重；
         * 否则按 MAC 去重（随机 MAC 场景下可能重复，但无更好方案）
         */
        for (uint16_t i = 0; i < s_scan_result_count; i++) {
            ble_device_info_t *existing = &s_scan_results[i];
            bool is_same = false;
            if (has_custom_id && existing->has_custom_id) {
                /* 按 UUID+SN 去重（忽略随机 MAC 变化） */
                is_same = (existing->custom_uuid == custom_uuid &&
                           existing->sn == sn);
            } else {
                /* 按 MAC 去重 */
                is_same = (memcmp(existing->mac, advertisedDevice->getAddress().getVal(), 6) == 0);
            }
            if (is_same) {
                /* 更新 RSSI（取更强的信号） */
                if (advertisedDevice->getRSSI() > existing->rssi) {
                    existing->rssi = advertisedDevice->getRSSI();
                }
                /* 更新 MAC（随机 MAC 可能已变化，保存最新的用于连接） */
                memcpy(existing->mac, advertisedDevice->getAddress().getVal(), 6);
                existing->addr_type = advertisedDevice->getAddressType();
                return;
            }
        }

        /* 新设备，添加到列表 */
        if (s_scan_result_count < BLE_SCAN_RESULT_MAX) {
            ble_device_info_t *dev = &s_scan_results[s_scan_result_count];

            const NimBLEAddress &addr = advertisedDevice->getAddress();
            const uint8_t *mac = addr.getVal();
            memcpy(dev->mac, mac, 6);
            dev->addr_type = advertisedDevice->getAddressType();
            dev->rssi = advertisedDevice->getRSSI();
            dev->has_custom_id = has_custom_id;
            dev->custom_uuid = custom_uuid;
            dev->sn = sn;

            if (advertisedDevice->haveName()) {
                std::string name = advertisedDevice->getName();
                strncpy(dev->name, name.c_str(), sizeof(dev->name) - 1);
                dev->name[sizeof(dev->name) - 1] = '\0';
            } else {
                strcpy(dev->name, "(no name)");
            }

            if (has_custom_id) {
                ESP_LOGI(TAG, "Found device: %s, UUID: %08X, SN: %08X, MAC: %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %d",
                         dev->name, custom_uuid, sn,
                         mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], dev->rssi);
                Serial.printf("[BLE] Found device %d: %s, UUID:%08X SN:%08X, RSSI: %d\n",
                              s_scan_result_count + 1, dev->name, custom_uuid, sn, dev->rssi);
            } else {
                ESP_LOGI(TAG, "Found device: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %d",
                         dev->name, mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], dev->rssi);
                Serial.printf("[BLE] Found device %d: %s, RSSI: %d\n",
                              s_scan_result_count + 1, dev->name, dev->rssi);
            }

            s_scan_result_count++;
        }
    }

    void onScanEnd(const NimBLEScanResults &scanResults, int reason) override
    {
        uint32_t elapsed = millis() - s_scan_start_ms;
        ESP_LOGI(TAG, "Scan ended after %lu ms, found %d devices, reason=%d", (unsigned long)elapsed, s_scan_result_count, reason);
        Serial.printf("[BLE] Scan complete after %lu ms! Found %d devices (reason=%d)\n", (unsigned long)elapsed, s_scan_result_count, reason);

        // 输出NimBLE内部扫描结果数量，用于对比
        int internal_count = scanResults.getCount();
        ESP_LOGI(TAG, "NimBLE internal scan results: %d", internal_count);
        Serial.printf("[BLE] NimBLE internal count: %d\n", internal_count);

        /* 按信号强度 RSSI 从高到低排序（数值越大信号越强，-30 > -80）。
         * 使用插入排序：设备数很少（<=20），无需额外内存，稳定且足够快。
         * 必须在 update_state(SCANNING->IDLE) 之前完成，因为 UI 刷新在
         * 该状态切换时触发，确保 UI 拿到的就是已排序的结果。
         */
        for (uint16_t i = 1; i < s_scan_result_count; i++) {
            ble_device_info_t key = s_scan_results[i];
            int16_t j = (int16_t)i - 1;
            while (j >= 0 && s_scan_results[j].rssi < key.rssi) {
                s_scan_results[j + 1] = s_scan_results[j];
                j--;
            }
            s_scan_results[j + 1] = key;
        }

        ESP_LOGI(TAG, "Scan results sorted by RSSI (desc), %d devices", s_scan_result_count);
        Serial.println("[BLE] Devices sorted by RSSI (high -> low):");
        for (uint16_t i = 0; i < s_scan_result_count; i++) {
            Serial.printf("[BLE]   #%u  RSSI=%d  %s\n",
                          (unsigned)(i + 1), s_scan_results[i].rssi, s_scan_results[i].name);
        }

        update_state(BLE_STATE_IDLE, "Scan complete");
    }
};

static MyAdvertisedDeviceCallbacks s_advertised_cb;

static bool connect_to_server(NimBLEAddress address);

/* ESP-NimBLE 内部以 LSB-first 存储 MAC（getVal() 返回的字节序），
 * 而 NimBLEAddress(uint8_t*, uint8_t) 构造函数期望 MSB-first（canonical）字节序，
 * 因此连接前必须反转 MAC 字节：native[0..5] -> canonical[5..0]。
 * 例：扫描报告 LSB-first = B3:97:1A:91:38:F9，
 *     反转到 canonical = F9:38:91:1A:97:B3，即为设备真实 MAC。 */
static void mac_reverse(const uint8_t *src, uint8_t *dst)
{
    for (int i = 0; i < 6; i++) {
        dst[i] = src[5 - i];
    }
}

/* 便捷：用 canonical 字节构造 NimBLEAddress
 * 输入：存储在 s_connect_device.mac 的 LSB-first 字节
 * 输出：可用于 connect() 的正确 NimBLEAddress */
static NimBLEAddress make_connect_address(const uint8_t *lsb_mac, uint8_t type)
{
    uint8_t canonical_mac[6];
    mac_reverse(lsb_mac, canonical_mac);
    return NimBLEAddress(canonical_mac, type);
}

static bool uuid_string_matches(const NimBLEUUID &uuid, const char *pattern)
{
    if (!pattern || !*pattern) {
        return false;
    }

    const std::string uuid_str = uuid.toString();
    const std::string pat = pattern;
    std::string a = uuid_str;
    std::string b = pat;

    a.erase(std::remove(a.begin(), a.end(), '-'), a.end());
    b.erase(std::remove(b.begin(), b.end(), '-'), b.end());

    // 统一转小写，兼容 16/32/128 位 UUID 字符串
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return std::tolower(c); });

    // 允许写法为 "FFE0" / "0000FFE0" / "0000FFE0-0000-1000-8000-00805F9B34FB"
    if (a == b) return true;
    if (a.size() == 32 && b.size() == 4 && a.substr(12, 4) == b) return true;
    if (a.size() == 32 && b.size() == 8 && a.substr(8, 8) == b) return true;
    if (a.size() == 32 && b.size() == 32 && a == b) return true;
    return false;
}

static void connect_task(void *arg)
{
    ESP_LOGI(TAG, "Connect task started");
    Serial.println("[BLE] Connection task starting...");

    /* 停掉任何正在运行的扫描，确保 BLE 控制器空闲。 */
    if (s_scan && s_scan->isScanning()) {
        ESP_LOGI(TAG, "Stopping active scan before connect...");
        s_scan->stop();
        delay(100);
        s_scan->clearResults();
    }
    delay(100);

    /* 保存原始扫描缓存的地址作为兜底（LSB-first 字节序）。 */
    const uint8_t orig_mac[6] = {
        s_connect_device.mac[0], s_connect_device.mac[1], s_connect_device.mac[2],
        s_connect_device.mac[3], s_connect_device.mac[4], s_connect_device.mac[5]
    };
    const uint8_t orig_type = s_connect_device.addr_type;

    /* 用反转后的 canonical 字节构造连接地址（NimBLEAddress 构造函数会再反转回 LSB-first 供 connect() 使用）。
     * 这样确保传递给 connect() 的是正确的 native 格式。 */
    NimBLEAddress address = make_connect_address(orig_mac, orig_type);
    update_state(BLE_STATE_CONNECTING, "Connecting...");

    bool ok = connect_to_server(address);
    if (!ok) {
        ESP_LOGE(TAG, "Connection failed, cleaning up...");
        Serial.println("[BLE] Connection failed!");
        if (s_client) {
            s_client->disconnect();
            NimBLEDevice::deleteClient(s_client);
            s_client = nullptr;
        }
        update_state(BLE_STATE_DISCONNECTED, "Connection failed");
    }

    s_connect_task_handle = NULL;
    ESP_LOGI(TAG, "Connect task finished");
    vTaskDelete(NULL);
}

/* 连接失败/特征查找失败时的统一清理：断开已建立的连接、释放 client、
 * 清空特征指针。由 connect_to_server 在失败路径调用后 return false，
 * 让 connect_task 走 DISCONNECTED 状态切换 UI。 */
static void abort_connection(void)
{
    s_is_connected = false;
    s_rx_char = nullptr;
    s_tx_char = nullptr;
    if (s_client) {
        s_client->disconnect();
        NimBLEDevice::deleteClient(s_client);
        s_client = nullptr;
    }
    Serial.println("[BLE] Connection aborted (char lookup failed or cleanup)");
}

/* 快速扫描（1 秒）用于在连接前刷新设备地址。
 * 手机 RPA 可能在短时间内轮换，扫描结束后立即连接可降低概率。
 * 返回 true 表示在扫描结果中找到了 s_connect_device.name 对应的设备，
 * 并将最新的 MAC/type 写回 s_connect_device。 */
static bool quick_rescan_address(uint32_t duration_ms)
{
    if (!s_scan) return false;

    s_scan->stop();
    delay(50);
    s_scan->clearResults();
    s_scan_result_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));

    s_scan->start(duration_ms, false, true);

    uint32_t t0 = millis();
    while (s_scan && s_scan->isScanning() && millis() - t0 < duration_ms + 1000) {
        delay(50);
    }
    delay(150);

    for (uint16_t i = 0; i < s_scan_result_count; i++) {
        if (strcmp(s_scan_results[i].name, s_connect_device.name) == 0) {
            memcpy(s_connect_device.mac, s_scan_results[i].mac, 6);
            s_connect_device.addr_type = s_scan_results[i].addr_type;
            s_connect_device.rssi = s_scan_results[i].rssi;
            return true;
        }
    }
    return false;
}

/* 尝试用给定的地址连接。返回 true 表示成功（后续 GATT 发现由调用者处理）。
 * 失败时不会修改 s_client 指针（调用者负责清理）。 */
static bool try_connect_once(NimBLEAddress &address)
{
    ESP_LOGI(TAG, "Attempting connect to %s (type=%d)...",
             address.toString().c_str(), address.getType());
    Serial.printf("[BLE]    Trying: %s (type=%d)\n",
                  address.toString().c_str(), address.getType());

    /* 额外诊断：打印 native 和 canonical 字节 */
    const uint8_t *native = address.getVal();
    Serial.printf("[BLE]      native bytes : %02X:%02X:%02X:%02X:%02X:%02X\n",
                  native[0], native[1], native[2], native[3], native[4], native[5]);
    Serial.printf("[BLE]      canonical   : %02X:%02X:%02X:%02X:%02X:%02X\n",
                  native[5], native[4], native[3], native[2], native[1], native[0]);

    if (s_client->connect(address)) {
        ESP_LOGI(TAG, "SUCCESS: Connected to %s", address.toString().c_str());
        Serial.println("[BLE]    -> SUCCESS!");
        return true;
    }

    int err = s_client->getLastError();
    ESP_LOGW(TAG, "Connect failed, err=%d", err);
    Serial.printf("[BLE]    -> Failed, err=%d\n", err);
    return false;
}

static bool connect_to_server(NimBLEAddress address)
{
    ESP_LOGI(TAG, "=== connect_to_server entered ===");
    Serial.println("[BLE] === connect_to_server ===");

    /* 清理旧 client */
    if (s_client) {
        s_client->disconnect();
        delay(100);
        NimBLEDevice::deleteClient(s_client);
        s_client = nullptr;
        s_rx_char = nullptr;
        s_tx_char = nullptr;
    }

    /* 先停扫描 */
    if (s_scan && s_scan->isScanning()) {
        s_scan->stop();
        delay(50);
        s_scan->clearResults();
    }
    delay(100);

    ESP_LOGI(TAG, "Controller status before connect: %d", esp_bt_controller_get_status());
    Serial.printf("[BLE] BT status: %d\n", esp_bt_controller_get_status());

    /* 创建新 client — 只用默认参数，不设置任何自定义 connection params。
     * 去掉 setMTU / setConnectionParams / setConnectTimeout，让 NimBLE
     * 使用默认配置（30s 超时、标准 interval 范围）。 */
    s_client = NimBLEDevice::createClient();
    if (!s_client) {
        ESP_LOGE(TAG, "createClient failed");
        Serial.println("[BLE] ERROR: createClient failed!");
        return false;
    }
    ESP_LOGI(TAG, "Client created");
    Serial.println("[BLE] Client created");

    /* 诊断：记录连接前 client 状态 */
    ESP_LOGI(TAG, "Client state before connect: %d", s_client->getState());

    /* ===== 连接尝试策略 =====
     * 1) 先用原始地址（来自用户选择的扫描结果）尝试
     * 2) 失败则做 1 秒快速重扫，用最新地址重试
     * 3) 再失败则强制把地址类型改为 0（public）重试
     * 4) 最后兜底：再扫 2 秒后重试
     * 任何一步成功都会立即进入 GATT 发现流程。 */

    /* 原始地址参数（LSB-first 字节序，来自扫描缓存） */
    const uint8_t orig_mac[6] = {
        s_connect_device.mac[0], s_connect_device.mac[1], s_connect_device.mac[2],
        s_connect_device.mac[3], s_connect_device.mac[4], s_connect_device.mac[5]
    };
    const uint8_t orig_type = s_connect_device.addr_type;

    bool connected = false;
    int attempt = 0;

    /* --- 尝试 1：用原始扫描地址（已在 connect_task 中反转过） --- */
    attempt++;
    ESP_LOGI(TAG, "Attempt %d: original address (type=%d)", attempt, orig_type);
    connected = try_connect_once(address);
    if (!connected && attempt < 4) {
        Serial.println("[BLE]    Retrying with quick re-scan...");
        delay(300);
        s_client->disconnect();
        delay(100);

        /* --- 尝试 2：1.5 秒快速重扫后用最新地址 --- */
        attempt++;
        bool refreshed = quick_rescan_address(1500);
        if (refreshed) {
            NimBLEAddress fresh = make_connect_address(s_connect_device.mac, s_connect_device.addr_type);
            ESP_LOGI(TAG, "Attempt %d: refreshed address (type=%d, rssi=%d)",
                     attempt, s_connect_device.addr_type, s_connect_device.rssi);
            Serial.printf("[BLE]    Re-scan found: %s (type=%d, rssi=%d)\n",
                          s_connect_device.name, s_connect_device.addr_type,
                          s_connect_device.rssi);
            connected = try_connect_once(fresh);
            if (!connected) {
                /* --- 尝试 3：强制 type=0（public）作为兜底 --- */
                attempt++;
                NimBLEAddress pub_addr = make_connect_address(s_connect_device.mac, 0);
                ESP_LOGI(TAG, "Attempt %d: forcing addr_type=0 (public)", attempt);
                Serial.println("[BLE]    Trying with addr_type=0 (public)...");
                connected = try_connect_once(pub_addr);
            }
        } else {
            /* 重扫没找到设备，强制用原地址 + type=0 */
            attempt++;
            NimBLEAddress pub_addr = make_connect_address(orig_mac, 0);
            ESP_LOGW(TAG, "Re-scan failed, forcing addr_type=0 on original MAC");
            Serial.println("[BLE]    Re-scan failed, forcing type=0 on original MAC...");
            connected = try_connect_once(pub_addr);
        }
    }

    if (!connected) {
        /* 所有反转字节序的尝试均失败 — 最后兜底：用原始 LSB-first 字节构造地址
         * （如果 NimBLEAddress 构造函数不做内部反转，这才是正确的格式）。
         * 这样无论 NimBLEAddress 构造函数是否反转字节，都能覆盖到。 */
        attempt++;
        ESP_LOGW(TAG, "All reversed-MAC attempts failed, trying original LSB-first bytes...");
        Serial.println("[BLE]    Last resort: trying with original LSB-first MAC (no reversal)...");
        NimBLEAddress fallback_addr(orig_mac, orig_type);
        connected = try_connect_once(fallback_addr);

        if (!connected) {
            /* 最终强制 type=0 再试一次 */
            attempt++;
            NimBLEAddress fallback_pub(orig_mac, 0);
            Serial.println("[BLE]    Last resort: trying with original MAC + type=0...");
            connected = try_connect_once(fallback_pub);
        }
    }

    if (!connected) {
        ESP_LOGE(TAG, "All %d connection attempts failed", attempt);
        Serial.println("[BLE] ERROR: All connection attempts failed!");
        NimBLEDevice::deleteClient(s_client);
        s_client = nullptr;
        return false;
    }

    /* ========== 以下为 GATT 发现流程 ========== */
    ESP_LOGI(TAG, "Connected! Discovering attributes...");
    Serial.println("[BLE] Connected! Discovering attributes...");

    if (!s_client->discoverAttributes()) {
        ESP_LOGE(TAG, "discoverAttributes() failed");
        Serial.println("[BLE] ERROR: discoverAttributes() failed!");
        abort_connection();
        return false;
    }

    const auto &services = s_client->getServices();
    ESP_LOGI(TAG, "Found %d services", (int)services.size());
    Serial.printf("[BLE] Found %d services\n", (int)services.size());

    /* 先打印所有服务/特征，便于核对 UUID */
    for (auto *svc : services) {
        ESP_LOGI(TAG, "Service: %s", svc->getUUID().toString().c_str());
        Serial.printf("[BLE]   Service: %s\n", svc->getUUID().toString().c_str());
        const auto &chars = svc->getCharacteristics();
        for (auto *ch : chars) {
            ESP_LOGI(TAG, "  Char: %s (notify=%d write=%d)",
                     ch->getUUID().toString().c_str(),
                     (ch->canNotify() || ch->canIndicate()),
                     (ch->canWrite() || ch->canWriteNoResponse()));
            Serial.printf("[BLE]     Char: %s (notify=%d write=%d)\n",
                           ch->getUUID().toString().c_str(),
                           (ch->canNotify() || ch->canIndicate()),
                           (ch->canWrite() || ch->canWriteNoResponse()));
        }
    }

    if (s_have_uuids) {
        NimBLEUUID svc_uuid(std::string(s_connect_uuids.service));
        NimBLEUUID notify_uuid(std::string(s_connect_uuids.notify));
        NimBLEUUID write_uuid(std::string(s_connect_uuids.write));
        Serial.printf("[BLE] Looking up Service:%s Notify:%s Write:%s\n",
                      s_connect_uuids.service, s_connect_uuids.notify, s_connect_uuids.write);
        Serial.printf("[BLE] Looking up Service obj: %s Notify obj: %s Write obj: %s\n",
                      svc_uuid.toString().c_str(), notify_uuid.toString().c_str(),
                      write_uuid.toString().c_str());

        NimBLERemoteService *target_svc = nullptr;
        for (auto *svc : services) {
            ESP_LOGI(TAG, "  Comparing: svc=%s == target=%s -> %d",
                     svc->getUUID().toString().c_str(), svc_uuid.toString().c_str(),
                     svc->getUUID() == svc_uuid);
            if (svc->getUUID() == svc_uuid) {
                target_svc = svc;
                break;
            }
        }
        if (!target_svc) {
            ESP_LOGE(TAG, "Service UUID %s not found", s_connect_uuids.service);
            Serial.printf("[BLE] ERROR: Service %s not found! Check the service UUID.\n",
                          s_connect_uuids.service);
            abort_connection();
            return false;
        }

        const auto &chars = target_svc->getCharacteristics();
        for (auto *ch : chars) {
            if (!s_rx_char && (ch->canNotify() || ch->canIndicate()) &&
                ch->getUUID() == notify_uuid) {
                s_rx_char = ch;
            }
            if (!s_tx_char && (ch->canWrite() || ch->canWriteNoResponse()) &&
                ch->getUUID() == write_uuid) {
                s_tx_char = ch;
            }
        }
        if (!s_rx_char) {
            ESP_LOGE(TAG, "Notify char %s not found or not capable", s_connect_uuids.notify);
            Serial.printf("[BLE] ERROR: Notify char %s not found/capable!\n", s_connect_uuids.notify);
            abort_connection();
            return false;
        }
        if (!s_tx_char) {
            ESP_LOGE(TAG, "Write char %s not found or not capable", s_connect_uuids.write);
            Serial.printf("[BLE] ERROR: Write char %s not found/capable!\n", s_connect_uuids.write);
            abort_connection();
            return false;
        }
        Serial.printf("[BLE] Matched: svc=%s notify=%s write=%s\n",
                      s_connect_uuids.service, s_connect_uuids.notify, s_connect_uuids.write);
    } else {
        Serial.println("[BLE] No target UUIDs, auto-picking first notify/write chars...");
        for (auto *svc : services) {
            const auto &chars = svc->getCharacteristics();
            for (auto *ch : chars) {
                if (!s_rx_char && (ch->canNotify() || ch->canIndicate())) s_rx_char = ch;
                if (!s_tx_char && (ch->canWrite() || ch->canWriteNoResponse())) s_tx_char = ch;
            }
        }
    }

    if (s_rx_char && !s_tx_char) {
        Serial.println("[BLE] WARN: notify char matched but write char not found; trying fallback write search across all services...");
        for (auto *svc : services) {
            const auto &chars = svc->getCharacteristics();
            for (auto *ch : chars) {
                if (!s_tx_char && (ch->canWrite() || ch->canWriteNoResponse())) {
                    s_tx_char = ch;
                    break;
                }
            }
            if (s_tx_char) break;
        }
    }

    if (s_rx_char) {
        if (s_rx_char->subscribe(s_rx_char->canNotify(), notify_callback)) {
            ESP_LOGI(TAG, "Subscribed for notifications");
            Serial.println("[BLE] Subscribed for notifications");
        } else {
            Serial.println("[BLE] WARN: Failed to subscribe");
        }
    } else {
        Serial.println("[BLE] WARN: No notify characteristic found");
    }

        if (!s_tx_char) {
        Serial.println("[BLE] WARN: No write characteristic found");
    }

    const uint8_t *addr_native = address.getVal();
    memcpy(s_remote_mac, addr_native, 6);

    /* 标记已连接：ble_manager_disconnect / ble_manager_send_data 都依赖
     * 此标志判断链路状态。之前漏掉这行导致 disconnect 时 s_is_connected
     * 仍为 false，s_client->disconnect() 被跳过，BLE 链路实际未断开。 */
    s_is_connected = true;
    update_state(BLE_STATE_CONNECTED, "Connected");
    Serial.println("[BLE] Connection successful!");
    return true;
}

extern "C" {

void ble_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE (NimBLE)...");
    Serial.println("[BLE] Initializing NimBLE...");

    // 关闭 WiFi，避免与 BLE 共享 2.4GHz 射频前端造成干扰
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    Serial.println("[BLE] WiFi disabled to avoid BLE interference");
    ESP_LOGI(TAG, "WiFi disabled to avoid RF interference with BLE");
    delay(100);

    // 输出内存信息
    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t free_psram = ESP.getFreePsram();
    uint32_t min_free_heap = ESP.getMinFreeHeap();
    Serial.printf("[BLE] Free heap: %lu, Min free heap: %lu, PSRAM: %lu\n", 
                  (unsigned long)free_heap, (unsigned long)min_free_heap, (unsigned long)free_psram);
    ESP_LOGI(TAG, "Free heap: %lu, Min: %lu, PSRAM: %lu",
             (unsigned long)free_heap, (unsigned long)min_free_heap, (unsigned long)free_psram);

    // 检查堆内存是否足够（BLE 控制器至少需要 ~30KB 连续内存）
    if (free_heap < 40000) {
        ESP_LOGE(TAG, "Low memory! Only %lu bytes free. BLE may fail.", (unsigned long)free_heap);
        Serial.println("[BLE] WARNING: Low memory, BLE scan may not work!");
    }

    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    /* MTU 可在连接后动态协商，默认 23 即可满足串口 BLE 模块需求；
     * 此处保留 setMTU(256) 以便后续大数据量传输，但它不影响建连流程。 */
    NimBLEDevice::setMTU(256);

    // 给 NimBLE 初始化一点时间
    delay(500);

    // 检查 BLE 控制器状态
    esp_bt_controller_status_t bt_status = esp_bt_controller_get_status();
    ESP_LOGI(TAG, "BLE controller status: %d (0=IDLE, 1=INITED, 2=ENABLED)", bt_status);
    Serial.printf("[BLE] BT controller status: %d\n", bt_status);
    if (bt_status != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_LOGE(TAG, "BLE controller NOT enabled! Scan will fail.");
        Serial.println("[BLE] ERROR: BT controller not enabled!");
        // 尝试手动启用
        Serial.println("[BLE] Attempting to enable BT controller...");
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        delay(200);
        bt_status = esp_bt_controller_get_status();
        Serial.printf("[BLE] BT status after enable: %d\n", bt_status);
    }

    free_heap = ESP.getFreeHeap();
    Serial.printf("[BLE] Free heap after init: %lu bytes\n", (unsigned long)free_heap);

    s_scan = NimBLEDevice::getScan();
    if (!s_scan) {
        ESP_LOGE(TAG, "Failed to get NimBLEScan instance!");
        Serial.println("[BLE] ERROR: getScan() returned null!");
        return;
    }

    s_scan->setScanCallbacks(&s_advertised_cb, false);
    s_scan->setActiveScan(true);
    // NimBLE setInterval/setWindow 参数单位为毫秒
    // interval=48ms, window=48ms = 100% 占空比，快速切换信道
    s_scan->setInterval(48);
    s_scan->setWindow(48);
    s_scan->setDuplicateFilter(false);
    s_scan->setLimitedOnly(false);  // 不限制为有限可发现模式

    s_data_queue = xQueueCreate(20, sizeof(ble_data_msg_t));
    if (!s_data_queue) {
        ESP_LOGE(TAG, "Failed to create data queue");
    }

    /* 创建 BLE Server：手机通过 Write(FFE1) 发送 11 路 CH 值帧 → onWrite → s_data_queue → 同一管线 */
    s_server = NimBLEDevice::createServer();
    /* 注册连接/断开回调：手机连接/断开时通知 UI 层刷新状态 */
    s_server->setCallbacks(new ServerCallbacks());
    s_server_service = s_server->createService("FFE0");
    s_server_notify_char = s_server_service->createCharacteristic("FFE2", NIMBLE_PROPERTY::NOTIFY);
    s_server_write_char  = s_server_service->createCharacteristic("FFE1", NIMBLE_PROPERTY::WRITE);
    s_server_write_char->setCallbacks(new ServerWriteCallbacks());
    s_server_service->start();
    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    /* 清除可能残留的广播数据，确保从干净状态开始配置 */
    pAdv->clearData();
    /* 先开启 scan response，再 setName：这样设备名会进入 scan response 数据，
     * 手机/小程序主动扫描时才能收到名称。若不开 scan response，
     * setName 会尝试放入 31 字节主广播包，可能因空间不足被静默丢弃。 */
    pAdv->enableScanResponse(true);
    if (!pAdv->setName(BLE_DEVICE_NAME)) {
        ESP_LOGE(TAG, "Failed to set advertising name");
        Serial.println("[BLE] ERROR: setName failed!");
    }
    if (!pAdv->addServiceUUID("FFE0")) {
        ESP_LOGE(TAG, "Failed to add service UUID to advertising");
        Serial.println("[BLE] ERROR: addServiceUUID failed!");
    }
    if (!pAdv->start()) {
        ESP_LOGE(TAG, "Failed to start advertising");
        Serial.println("[BLE] ERROR: advertising start failed!");
    } else {
        Serial.printf("[BLE] Server started: name=%s, FFE0/FFE2(notify)/FFE1(write), advertising...\n", BLE_DEVICE_NAME);
    }
    
    ESP_LOGI(TAG, "BLE initialized successfully (s_scan=%p)", s_scan);
    Serial.printf("[BLE] NimBLE initialized, scan=%p, heap=%lu\n", s_scan, (unsigned long)free_heap);
    update_state(BLE_STATE_IDLE, "BLE Ready");
}

void ble_manager_start_scan(void)
{
    if (!s_scan) {
        ESP_LOGE(TAG, "s_scan is NULL, cannot start scan");
        Serial.println("[BLE] ERROR: s_scan is null!");
        return;
    }

    if (s_state == BLE_STATE_SCANNING) {
        ESP_LOGW(TAG, "Already scanning");
        return;
    }

    // 停止并清理之前可能残留的扫描状态
    s_scan->stop();
    vTaskDelay(pdMS_TO_TICKS(50));  // 让出 CPU，不阻塞 LVGL 任务
    s_scan->clearResults();

    s_scan_result_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));

    update_state(BLE_STATE_SCANNING, "Scanning...");

    Serial.println("[BLE] Starting BLE scan (1 seconds)...");
    ESP_LOGI(TAG, "Starting BLE scan for 1 seconds...");

    // NimBLE start() 的 duration 参数单位为毫秒，1s = 1000ms
    bool ok = s_scan->start(1000, false, true);
    if (!ok) {
        ESP_LOGE(TAG, "Scan failed to start");
        Serial.println("[BLE] ERROR: Scan failed to start!");
        update_state(BLE_STATE_IDLE, "Scan failed");
        return;
    }

    ESP_LOGI(TAG, "Scan started (1000ms)");
    s_scan_start_ms = millis();
    Serial.printf("[BLE] Scan started at %lu ms, waiting for results...\n", (unsigned long)s_scan_start_ms);
}

void ble_manager_stop_scan(void)
{
    if (s_scan) {
        s_scan->stop();
    }
    update_state(BLE_STATE_IDLE, "Scan stopped");
}

void ble_manager_connect(const ble_device_info_t *device, const ble_uuid_config_t *uuids)
{
    if (s_is_connected || !device) {
        ESP_LOGW(TAG, "Already connected or null device");
        return;
    }

    if (s_connect_task_handle) {
        ESP_LOGW(TAG, "Connection already in progress");
        return;
    }

    memcpy(&s_connect_device, device, sizeof(ble_device_info_t));

    /* 保存目标 UUID 配置；NULL 表示自动选取第一个可通知/可写特征 */
    if (uuids) {
        memcpy(&s_connect_uuids, uuids, sizeof(ble_uuid_config_t));
        s_have_uuids = true;
        ESP_LOGI(TAG, "Target UUIDs: svc=%s notify=%s write=%s",
                 uuids->service, uuids->notify, uuids->write);
        Serial.printf("[BLE] Target UUIDs: svc=%s notify=%s write=%s\n",
                      uuids->service, uuids->notify, uuids->write);
    } else {
        s_have_uuids = false;
        ESP_LOGI(TAG, "No target UUIDs, will auto-pick notify/write chars");
        Serial.println("[BLE] Auto-pick notify/write chars (no UUIDs given)");
    }

    Serial.printf("[BLE] Connecting to: %s [native=%02X:%02X:%02X:%02X:%02X:%02X, canonical=%02X:%02X:%02X:%02X:%02X:%02X] type=%d\n",
                  device->name,
                  device->mac[0], device->mac[1], device->mac[2],
                  device->mac[3], device->mac[4], device->mac[5],
                  device->mac[5], device->mac[4], device->mac[3],
                  device->mac[2], device->mac[1], device->mac[0],
                  device->addr_type);
    ESP_LOGI(TAG, "Connecting to: %s, addr_type=%d",
             device->name, device->addr_type);

    // 增加栈大小到 16384 字节，确保 NimBLE GATT 操作有足够空间
    BaseType_t ret = xTaskCreate(connect_task, "BLE_CONN", 16384, NULL, 5, &s_connect_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create connect task (stack size issue)");
        Serial.println("[BLE] ERROR: Failed to create connection task! Try reducing stack size.");
        update_state(BLE_STATE_DISCONNECTED, "Connect task failed");
    }
}

void ble_manager_disconnect(void)
{
    /* 只要 client 存在就执行真正的链路断开 + 释放，避免依赖 s_is_connected
     * 标志（历史 bug：连接成功后未置 true，导致此处跳过 disconnect）。
     * 与 abort_connection() 保持一致的清理模式。 */
    if (s_client) {
        s_client->disconnect();
        NimBLEDevice::deleteClient(s_client);
        s_client = nullptr;
    }
    s_is_connected = false;
    s_rx_char = nullptr;
    s_tx_char = nullptr;
    /* 清空帧重组缓冲区，避免下次连接时残留半帧污染首帧 */
    s_frame_len = 0;
    s_frame_buf[0] = '\0';
    /* 清空数据队列，避免断开时还在处理旧数据 */
    if (s_data_queue) {
        ble_data_msg_t msg;
        while (xQueueReceive(s_data_queue, &msg, 0) == pdPASS) {}
    }
    update_state(BLE_STATE_DISCONNECTED, "Disconnected");
}

void ble_manager_send_data(uint8_t *data, uint16_t len)
{
    if (!s_is_connected || !s_tx_char) {
        ESP_LOGW(TAG, "Cannot send: not connected or no TX char");
        return;
    }

    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = (len - offset > 20) ? 20 : (len - offset);
        s_tx_char->writeValue(data + offset, chunk, false);
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
    if (results) {
        for (uint16_t i = 0; i < c; i++) {
            memcpy(&results[i], &s_scan_results[i], sizeof(ble_device_info_t));
        }
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

/* 在主 loop 中调用：检查 BLE 回调中设置的待处理状态，在主任务上下文
 * （非 NimBLE 任务）中安全调用 update_state → ui_update_state。
 * 这避免了在 BLE 回调中直接获取 LVGL 互斥锁导致的死锁。 */
void ble_manager_process_state(void)
{
    if (!s_state_pending) return;
    s_state_pending = false;
    update_state(s_pending_state, s_pending_message);
}

void ble_manager_process_data(void)
{
    if (!s_data_queue || !s_data_cb) return;

    ble_data_msg_t msg;
    while (xQueueReceive(s_data_queue, &msg, 0) == pdPASS) {
        /* 将本次 notify 收到的字节喂入帧重组缓冲区，
         * 遇到帧尾 ';' 即输出一帧完整数据。 */
        for (uint16_t i = 0; i < msg.len; i++) {
            char c = (char)msg.data[i];
            /* 忽略换行符：部分 BLE 串口模块在 ';' 后会追加 \r\n，
             * 丢弃它们可保持帧干净，避免原始帧显示出现空行。 */
            if (c == '\r' || c == '\n') continue;

            if (s_frame_len < sizeof(s_frame_buf) - 1) {
                s_frame_buf[s_frame_len++] = c;
            } else {
                /* 缓冲区满仍未见 ';'，丢弃半帧重新同步 */
                ESP_LOGW(TAG, "Frame buffer overflow, resync");
                s_frame_len = 0;
                s_frame_buf[s_frame_len++] = c;
            }

            if (c == ';') {
                /* 一帧完整，交给上层解析（含帧尾 ';'） */
                s_frame_buf[s_frame_len] = '\0';
                s_data_cb((uint8_t *)s_frame_buf, s_frame_len);
                s_frame_len = 0;
            }
        }
    }
}

/* 解析 "v1,v2,...,v11;" 格式帧。容错：跳过非数字前导字符，
 * 按逗号/分号分隔提取数值，支持前导零（如 "0920"→920）。 */
uint8_t ble_manager_parse_frame(const uint8_t *data, uint16_t len,
                                int16_t *values, uint8_t max_count)
{
    if (!data || !values || max_count == 0) return 0;

    uint8_t count = 0;
    uint16_t i = 0;
    while (i < len && count < max_count) {
        /* 跳到下一个数字起点（支持负号） */
        while (i < len) {
            char c = (char)data[i];
            if ((c >= '0' && c <= '9') || c == '-') break;
            i++;
        }
        if (i >= len) break;

        int32_t v = 0;
        int16_t sign = 1;
        if ((char)data[i] == '-') { sign = -1; i++; }

        bool any = false;
        while (i < len && (char)data[i] >= '0' && (char)data[i] <= '9') {
            v = v * 10 + ((char)data[i] - '0');
            i++;
            any = true;
        }
        if (any) {
            values[count++] = (int16_t)(v * sign);
        }

        /* 跳到下一个分隔符（逗号/分号）之后 */
        while (i < len && (char)data[i] != ',' && (char)data[i] != ';') i++;
        if (i < len) i++;  /* 跳过分隔符 */
    }
    return count;
}

}
