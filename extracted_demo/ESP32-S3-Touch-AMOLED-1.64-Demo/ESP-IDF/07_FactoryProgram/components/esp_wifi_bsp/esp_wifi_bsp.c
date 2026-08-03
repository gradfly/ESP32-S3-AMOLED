#include <stdio.h>
#include "esp_wifi_bsp.h"
#include "esp_event.h" //事件
#include "nvs_flash.h" //NVS区

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
//TaskHandle_t pxWIFIreadTask;
//
//QueueHandle_t WIFI_QueueHandle;
void nvs_flash_Init(void)
{
  nvs_flash_init();                    // 初始化默认的nvs区
  esp_netif_init();                    // 初始化TCP/IP栈
  esp_event_loop_create_default();     // 建立默认事件循环
  esp_netif_create_default_wifi_sta(); // 把TCP/IP协议栈加到默认事件循环
}
void espwifi_Init(void)
{
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // 默认的config
  esp_wifi_init(&cfg);                                 // 初始化
  esp_event_handler_instance_t Instance_WIFI_IP;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &Instance_WIFI_IP);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &Instance_WIFI_IP);
  wifi_config_t wifi_config = {
      .sta = {
        .ssid = "bsp_esp_demo",
        .password = "emqx123456",
      },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);               // 模式STA
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config); // 设置WIFI
  esp_wifi_start();                               // 启动WIFI
  //WIFI_QueueHandle = xQueueCreate(30,sizeof(wifi_scan_config_t));         //创建队列项目30，字节20
  //printf("wifi_scan_config_t:%d\n",sizeof(wifi_scan_config_t));
}
void espwifi_Deinit(void)
{
  esp_wifi_stop();
  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler);
  esp_wifi_deinit();
}
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_id == WIFI_EVENT_STA_START)
  {
    //esp_wifi_connect(); // 连接WIFI
    //esp_wifi_scan_u();
  }
  else if (event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    char ip[25];
    uint32_t pxip = event->ip_info.ip.addr;
    sprintf(ip, "%d.%d.%d.%d", (uint8_t)(pxip), (uint8_t)(pxip >> 8), (uint8_t)(pxip >> 16), (uint8_t)(pxip >> 24));
    printf("IP:%s\n", ip);
  }
  else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    printf("diconst\n");
  }
}


