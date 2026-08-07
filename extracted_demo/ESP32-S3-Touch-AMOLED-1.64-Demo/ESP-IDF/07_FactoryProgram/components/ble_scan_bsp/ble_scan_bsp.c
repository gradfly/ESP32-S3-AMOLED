#include <stdio.h>
#include "ble_scan_bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define GATTC_TAG "GATTC_DEMO"
EventGroupHandle_t ble_Even;
QueueHandle_t ble_Queue;
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0


/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_ble_scan_params_t ble_scan_params = {                //ble扫描参数设置
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE        //过滤重复的广告
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};


/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* 没有得到初始值,所以是这个ESP_GATT_IF_NONE */
    },
};


static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) // 该函数由esp_gattc_cb回调函数调用而具备执行机会
{
    //esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event)
    {
        case ESP_GATTC_REG_EVT: //通过esp_gattc_cb回调函数执行的这个回调函数传入事件参数
            ESP_LOGI(GATTC_TAG, "REG_EVT");
            //esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params); //设置扫描参数
            //if (scan_ret)
            //{
            //    ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
            //}
            break;
        case ESP_GATTC_CFG_MTU_EVT: //当 GATT 客户端通过调用 esp_ble_gattc_set_mtu() 函数成功配置了 MTU 时，系统会触发此事件
            if (param->cfg_mtu.status != ESP_GATT_OK)
            {
                ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
            }
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
            break;
        default:
            break;
    }
}
static uint8_t value = 0;
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    //uint8_t *adv_name = NULL;
    //uint8_t adv_name_len = 0;
    switch (event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: //当调用 esp_ble_gap_set_scan_params() 函数成功设置扫描参数后，会触发此事件
        {
            //esp_ble_gap_start_scanning(duration); //30秒的扫描时间
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: //当调用 esp_ble_gap_start_scanning() 函数成功启动扫描后，会触发此事件
        {
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) //监测是否启动成功,需要等于ESP_BT_STATUS_SUCCESS
            {
                ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "scan start success");
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: //每扫描到一个就进入一次,不是等待扫描完才进入
        {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) //    搜索事件类型      
            {
                case ESP_GAP_SEARCH_INQ_RES_EVT:
                    esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6); //蓝牙设备地址  adv_data_len 是扫描到的设备的广告数据的长度。用途：广告数据是设备在进行广播时发送的消息。这些数据通常包含设备的名称、服务 UUID、制造商数据等。广告数据的长度由 adv_data_len 指定
                    //扫描响应数据是设备在接收到来自扫描设备的请求后返回的附加信息。通常，它包含更多关于设备的信息，比如完整的设备名称或其他服务信息。场景：当一个设备（例如手机或其他 BLE 设备）扫描到一个广告包时，它可以发送一个扫描请求以请求更多信息。目标设备会响应这个请求，并发送扫描响应数据
                    //响应的消息会更加多
                    ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len); 
                    //adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len); //如果没有扫描到名字会返回NULL
                    if(xQueueSend(ble_Queue,scan_result->scan_rst.bda,0) == pdTRUE)
                    {
                        value++;
                        if(value == 20)
                        {
                            value = 0;
                            esp_ble_gap_stop_scanning(); //停止扫描
                        }
                    }
                    /*if (adv_name != NULL) 
                    {
                        //ESP_LOGI(GATTC_TAG, "adv_name: %s", adv_name);
                        //esp_ble_gap_stop_scanning();                          //停止扫描
                        //esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true); //打开gattc,开始连接
                    }*/
                    break;
                case ESP_GAP_SEARCH_INQ_CMPL_EVT: //事件用于通知 BLE 设备的扫描操作已完成 时间到达而完成的扫描时间
                    value = 0;
                    break;
                default:
                    break;
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: //通常在您调用 esp_ble_gap_stop_scanning() 函数停止扫描后触发 与扫描相关
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "stop scan successfully");

            break;
        default:
            break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT)              // 当GATT客户端注册时，事件发生
    {
        if (param->reg.status == ESP_GATT_OK)    // 获取运行状态
        {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if; // param->reg.app_id获取ID,就是注册时的那个gattc_if是系统自动生成
            printf("gattc_if:%d\n",gattc_if);
        } 
        else 
        {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",param->reg.app_id,param->reg.status); // 否则输出对应的错误结构
            return;                                                                                            //返回,不执行后面的
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == gl_profile_tab[idx].gattc_if) //如果回调报告 gattc_if/gatts_if 为ESP_GATT_IF_NONE宏，则表示此事件不对应任何应用程序
            {
                if (gl_profile_tab[idx].gattc_cb) 
                {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param); //调用回调函数 
                }
            }
        }
    } while (0);
}

void ble_scan_mem_init(void)
{
  ble_Even = xEventGroupCreate();
  ble_Queue = xQueueCreate( 20 , sizeof(uint8_t) * 6); 
}
void ble_scan_class_init(void)
{
    ble_scan_mem_init();
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}
void ble_scan_Init(void)
{
  esp_err_t ret;
  //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) 
  {
    ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) 
  {
    ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_init();
  if (ret)
  {
    ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret)
  {
    ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_ble_gap_register_callback(esp_gap_cb);
  if (ret)
  {
    ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
    return;
  }
  ret = esp_ble_gattc_register_callback(esp_gattc_cb);
  if(ret)
  {
    ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
    return;
  }
  ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
  if (ret)
  {
    ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
  }
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret)
  {
    ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
  }
}
void ble_scan_setconf(void)
{
    esp_ble_gap_set_scan_params(&ble_scan_params);//set scan
    esp_ble_gap_start_scanning(3);         //3秒的扫描时间
}
void ble_scan_Deinit(void)
{
    if(value != 0)esp_ble_gap_stop_scanning();
    esp_ble_gattc_app_unregister(gl_profile_tab[0].gattc_if);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
}