#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define LCD_SPI_NUM        SPI3_HOST
#define ESP32_I2C_DEV_NUM  I2C_NUM_0

#define LCD_WIDTH  280 //宽度 水平分辨率
#define LCD_HEIGHT 456 //高度 竖直分辨率

/*lcd port Init*/
#define LCD_D0_PIN      GPIO_NUM_11
#define LCD_D1_PIN      GPIO_NUM_12
#define LCD_D2_PIN      GPIO_NUM_13
#define LCD_D3_PIN      GPIO_NUM_14
#define LCD_CS_PIN      GPIO_NUM_9  
#define LCD_SCL_PIN     GPIO_NUM_10
#define LCD_RST_PIN     GPIO_NUM_21

/*ESP32 I2C Init*/
#define ESP32_I2C_SDA_PIN GPIO_NUM_47
#define ESP32_I2C_SCL_PIN GPIO_NUM_48


#endif // !USER_CONFIG_H