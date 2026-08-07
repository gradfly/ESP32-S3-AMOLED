#include <stdio.h>
#include "user_app.h"
#include "sd_card_bsp.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include "esp_timer.h"
#include "adc_bsp.h"
#include "esp_wifi_bsp.h"
#include "ble_scan_bsp.h"
#include "freertos/event_groups.h"
#include "qmi8658c.h"

lv_ui guider_ui;
TaskHandle_t pxBleTask;
TaskHandle_t pxWifiTask;
EventGroupHandle_t TaskEven;
void lv_stop_roll(lv_obj_t *obj,uint8_t value,uint8_t mode);
void user_gui_screen(lv_ui *ui);
void user_app_init(lv_ui *ui);
void example_app_task(void *pro);
void esp_wifi_scan_w(void *arg);
void esp_ble_scan_w(void *arg);
void esp_wifi_ble_setscan(uint8_t mode);
void color_user(void *arg);
//void example_ble_scan_w(void *arg);
void lv_clear_list(lv_obj_t *obj,uint8_t value);
ClockModule clock_iniput;
void SetTheClock_start(lv_ui *ui);
void out_time(ClockModule * clock);
extern void setBrightnes(uint8_t brig);
/*事件*/
static void screen_btn_event_handler (lv_event_t *e);
void user_top_init(void)
{
  TaskEven = xEventGroupCreate();
  xEventGroupSetBits( TaskEven,(0x01<<2) ); //wifi
  xEventGroupSetBits( TaskEven,(0x01<<1) ); //ble
  SD_card_Init();
  adc_bsp_init();
  nvs_flash_Init();
  ble_scan_class_init();
  ble_scan_Init();
  setup_ui(&guider_ui);
  events_init(&guider_ui);
  user_gui_screen(&guider_ui);
  user_app_init(&guider_ui);
  clock_iniput.Hours = 7;
  clock_iniput.minutes = 30;
  clock_iniput.seconds = 30;
  out_time(&clock_iniput);          //初始化时钟
  SetTheClock_start(&guider_ui);    //启动时钟
  lv_clear_list(guider_ui.screen_list_1,21); //清空
  lv_clear_list(guider_ui.screen_list_2,21); //清空
}
void user_gui_screen(lv_ui *ui)
{
  lv_obj_add_event_cb(ui->screen_btn_1, screen_btn_event_handler, LV_EVENT_ALL, ui);    //事件
  lv_obj_add_event_cb(ui->screen_btn_2, screen_btn_event_handler, LV_EVENT_ALL, ui);    //事件
  lv_obj_add_event_cb(ui->screen_imgbtn_1, screen_btn_event_handler, LV_EVENT_ALL, ui); //事件
  lv_obj_add_event_cb(ui->screen_imgbtn_2, screen_btn_event_handler, LV_EVENT_ALL, ui); //事件
  lv_obj_add_event_cb(ui->screen_btn_3, screen_btn_event_handler, LV_EVENT_ALL, ui); //事件
  lv_obj_add_event_cb(ui->screen_btn_4, screen_btn_event_handler, LV_EVENT_ALL, ui); //事件
  lv_obj_add_event_cb(ui->screen_slider_1, screen_btn_event_handler, LV_EVENT_ALL, ui); 
}
void user_app_init(lv_ui *ui)
{
  xTaskCreate(example_app_task, "example_app_task", 3000, (void *)ui, 2, NULL);
  xTaskCreate(esp_wifi_scan_w, "esp_wifi_scan_w", 3000, ui, 2, &pxWifiTask);
  xTaskCreate(esp_ble_scan_w, "esp_ble_scan_w", 3000, ui, 2, &pxBleTask);
  xTaskCreate(color_user, "color_user", 2000, ui, 6, NULL); //切换颜色
}
void clock_task_callback(void *arg)
{
  static uint8_t bat = 0;
  lv_ui *ui = (lv_ui *)arg;
  clock_iniput.out_seconds++;
  int16_t Seconds_ars = clock_iniput.out_seconds * 6 - 90;
  lv_img_set_angle(ui->screen_img_3, Seconds_ars * 10);
  if(clock_iniput.out_seconds == 60)
  {
    clock_iniput.out_seconds = 0;
    clock_iniput.out_minutes++;
    int16_t Minutes_ars = clock_iniput.out_minutes * 6 - 90;
    lv_img_set_angle(ui->screen_img_2, Minutes_ars * 10);
    if( (clock_iniput.out_minutes == 12) || (clock_iniput.out_minutes == 24) || (clock_iniput.out_minutes == 36) || (clock_iniput.out_minutes == 48) || (clock_iniput.out_minutes == 60))
    bat = 1;
    else
    bat = 0;
  }
  if(clock_iniput.out_minutes == 60)
  {
    clock_iniput.minutes = 0;
  }
  if( bat == 1 )
  {
    bat = 0;
    clock_iniput.out_Hours++;
    int16_t Hours_ars = clock_iniput.out_Hours * 6 - 90;
    lv_img_set_angle(ui->screen_img_1, Hours_ars * 10);
  }
  if(clock_iniput.out_Hours == 60)
  {
    clock_iniput.out_Hours = 0;
  }
}
void SetTheClock_start(lv_ui *ui)
{
  const esp_timer_create_args_t clock_tick_timer_args = 
  {
    .callback = &clock_task_callback,
    .name = "clock_task",
    .arg = ui,
  };
  esp_timer_handle_t clock_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 1000));              //1s
}
void out_time(ClockModule * clock)
{
  clock->out_Hours = clock->Hours * 5;
  clock->out_minutes = clock->minutes;
  clock->out_seconds = clock->seconds;
  uint8_t bat = clock->out_minutes / 12;
  clock->out_Hours += bat;

  int16_t Hours_ars = clock_iniput.out_Hours * 6 - 90;
  int16_t Minutes_ars = clock_iniput.out_minutes * 6 - 90;
  int16_t Seconds_ars = clock_iniput.out_seconds * 6 - 90;
  lv_img_set_angle(guider_ui.screen_img_1, Hours_ars * 10);
  lv_img_set_angle(guider_ui.screen_img_2, Minutes_ars * 10);
  lv_img_set_angle(guider_ui.screen_img_3, Seconds_ars * 10);
}

void example_app_task(void *pro)
{
  lv_ui *obj = (lv_ui *)pro;
  char adc_values[15] = {0};
  char sd_buff[8] = {0};
  float adc_value;
  float sd_value = 0;
  float acc[3];
  float gyro[3];
  char imu_buf[200] = {""};
  uint32_t stimes = 0;
  uint32_t adc_test = 0;
  uint32_t imu_test = 0;
  sd_value = sd_cadr_get_value();
  if(sd_value != 0)
  {
    sprintf(sd_buff,"%.2fG",sd_value);
    lv_label_set_text(obj->screen_label_7, sd_buff);
  }
  qmi8658_init();
  for(;;)
  {
    if(stimes - adc_test > 0) //1s
    {
      adc_test = stimes;
      adc_get_value(&adc_value);
      if(adc_value)
      {
        sprintf(adc_values,"%.2fV",adc_value);
        lv_label_set_text(obj->screen_label_8, adc_values);
      }
    }
    if(stimes - imu_test > 0) //1s
    {
      imu_test = stimes;
      qmi8658_read_xyz(acc,gyro);
      snprintf(imu_buf,200,"IMU:\n %.2fmg\n %.2fmg\n %.2fmg\n %.2fdps\n %.2fdps\n %.2fdps\n",acc[0],acc[1],acc[2],gyro[0],gyro[1],gyro[2]);
      lv_span_set_text(obj->screen_spangroup_1_span, imu_buf);
      //printf("x:%f y:%f z:%f gyro_x:%f gyro_y:%f gyro_z:%f\n",acc[0],acc[1],acc[2],gyro[0],gyro[1],gyro[2]);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    stimes++;
  }
}



void lv_clear_list(lv_obj_t *obj,uint8_t value) 
{
	for(signed char i = value-1; i>=0; i--)
	{
		lv_obj_t *imte = lv_obj_get_child(obj,i);
		lv_obj_add_flag(imte,LV_OBJ_FLAG_HIDDEN);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}
void lv_stop_roll(lv_obj_t *obj,uint8_t value,uint8_t mode)
{
  if(mode == 1)
  {
    //lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    for(signed char i = value-1; i>=0; i--)
	  {
		  lv_obj_t *imte = lv_obj_get_child(obj,i);
      lv_obj_t *txt = lv_obj_get_child(imte,1);
		  lv_label_set_long_mode(txt, LV_LABEL_LONG_DOT);
    }
	}
  else
  {
    //lv_obj_add_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    for(signed char i = value-1; i>=0; i--)
	  {
		  lv_obj_t *imte = lv_obj_get_child(obj,i);
      lv_obj_t *txt = lv_obj_get_child(imte,1);
		  lv_label_set_long_mode(txt, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
  }
}
/*事件*/
static void screen_btn_event_handler (lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_ui *ui = (lv_ui *)e->user_data;
  lv_obj_t * module = e->current_target;
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      EventBits_t even = xEventGroupWaitBits(TaskEven,(0x01<<1) | (0x01<<2),pdFALSE,pdFALSE,10);
      if(module == ui->screen_btn_1)
      {
        if( even & (0x01<<1) )
        {
          esp_wifi_ble_setscan(0); //ble
          xEventGroupClearBits( TaskEven,(0x01<<1) );
          ble_scan_setconf();
          xTaskNotifyGive(pxBleTask);
        }
      }
      else if(module == ui->screen_btn_2) //wifi
      {
        if( even & (0x01<<2) )
        {
          esp_wifi_ble_setscan(1); //wifi

          xEventGroupClearBits( TaskEven,(0x01<<2) );
          xTaskNotifyGive(pxWifiTask);
        }
      }
      else if(module == ui->screen_imgbtn_1)
      {
        lv_obj_clear_flag(ui->screen_cont_1,LV_OBJ_FLAG_HIDDEN); //显示
        lv_obj_add_flag(ui->screen_cont_2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);   
      }
      else if(module == ui->screen_imgbtn_2)
      {
        lv_obj_clear_flag(ui->screen_cont_3,LV_OBJ_FLAG_HIDDEN); //显示
        lv_obj_add_flag(ui->screen_cont_2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_1, LV_OBJ_FLAG_HIDDEN);   
      }
      else if( (module == ui->screen_btn_3) || (module == ui->screen_btn_4) )
      {
        lv_obj_clear_flag(ui->screen_cont_2,LV_OBJ_FLAG_HIDDEN); //显示
        lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_1, LV_OBJ_FLAG_HIDDEN);
      }
      else if( module == ui->screen_slider_1 )
      {
        uint8_t value = lv_slider_get_value(module);
        setBrightnes(value);
      }
      break;
    }
    default:
      break;
  }
}
//wifi :1 ble:0
void esp_wifi_ble_setscan(uint8_t mode)
{
  static uint8_t wifi_ble_flag = 0;
  if(mode != wifi_ble_flag)
  {
    if(mode) //wifi 需要释放ble
    {
      ble_scan_Deinit();
      espwifi_Init();
    }
    else
    {
      espwifi_Deinit();
      ble_scan_Init();
    }
    wifi_ble_flag = mode;
  }
}
void color_user(void *arg)
{
  lv_ui *ui = (lv_ui *)arg;
  lv_obj_clear_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //不可点击
  lv_obj_clear_flag(ui->screen_cont_5,LV_OBJ_FLAG_HIDDEN); //显示
  lv_obj_add_flag(ui->screen_cont_4, LV_OBJ_FLAG_HIDDEN);   
  lv_obj_clear_flag(ui->screen_img_4,LV_OBJ_FLAG_HIDDEN); //显示
  lv_obj_add_flag(ui->screen_img_5, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_6, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_5,LV_OBJ_FLAG_HIDDEN); //显示
  lv_obj_add_flag(ui->screen_img_4, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_6, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_6,LV_OBJ_FLAG_HIDDEN); //显示
  lv_obj_add_flag(ui->screen_img_4, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_5, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_cont_4,LV_OBJ_FLAG_HIDDEN); //显示
  lv_obj_add_flag(ui->screen_cont_5, LV_OBJ_FLAG_HIDDEN);  
  lv_obj_add_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //可点击
  vTaskDelete(NULL); //删除任务
}
void esp_wifi_scan_w(void *arg)
{
  lv_ui *wifi_obj = (lv_ui *)arg;
	static wifi_ap_record_t recdata;
  static uint16_t rec = 0;
	static const char *imgbox = NULL;
	static lv_obj_t *imte;
	static lv_obj_t *label;
  for(;;)
  {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);                                      //等待任务通知
    lv_stop_roll(wifi_obj->screen_list_2,rec,1);
		lv_clear_list(wifi_obj->screen_list_2,rec);
    lv_stop_roll(wifi_obj->screen_list_2,rec,0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(NULL,true));               //扫描可用AP
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&rec));
    if(rec != 0)
    {
			if(rec > 20)
			{
				rec = 21;
				for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_2,i);
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1);
						if(label != NULL)
            {
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    //显示
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(300));
      	}
				esp_wifi_clear_ap_list();
			}
			else
			{
      	for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_2,i); //获取子对象
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1); //获取子子对象
						if(label != NULL)
            {
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    //显示
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(100));
      	}
			}
    }
    xEventGroupSetBits( TaskEven,(0x01<<2) );
  }
}
void esp_ble_scan_w(void *arg)
{
  lv_ui *wifi_obj = (lv_ui *)arg;
  static uint16_t rec = 0;
  uint8_t mac[6];
  static lv_obj_t *imte;
	static lv_obj_t *label;
  char imgbox[24] = {0};
  for(;;)
  {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    lv_stop_roll(wifi_obj->screen_list_1,rec,1);     
    lv_clear_list(wifi_obj->screen_list_1,rec);
    lv_stop_roll(wifi_obj->screen_list_1,rec,0);
    rec = 0;
    for(;xQueueReceive(ble_Queue,mac,3500) == pdTRUE;)
    {
      imte = lv_obj_get_child(wifi_obj->screen_list_1,rec);
      if (imte != NULL)
			{
				label = lv_obj_get_child(imte,1);
				if(label != NULL)
        {
          sprintf(imgbox,"%d:%d:%d:%d:%d:%d",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    			lv_label_set_text(label,imgbox);
					lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    //显示
				}
			}
			imte = NULL;
			label = NULL;
      rec++;
      vTaskDelay(pdMS_TO_TICKS(100));
      if(rec == 21)
      break;
    }
    xEventGroupSetBits( TaskEven,(0x01<<1) );
  }
}
