#ifndef USER_APP_H
#define USER_APP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
typedef struct  
{
  uint8_t Hours;
  uint8_t minutes;
  uint8_t seconds;
  uint16_t out_Hours;
  uint16_t out_minutes;
  uint16_t out_seconds;
}ClockModule; //时间的结构体

void user_top_init(void);

#endif

