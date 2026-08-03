#include "FT3168.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lcd_config.h"
#include "driver/i2c.h"

#define TEST_I2C_PORT I2C_NUM_0

static const char *TAG = "FT3168";

static esp_err_t I2C_write_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t payload[8];
    if (len + 1 > sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    payload[0] = reg;
    for (uint8_t i = 0; i < len; i++) {
        payload[i + 1] = buf[i];
    }
    return i2c_master_write_to_device(TEST_I2C_PORT, I2C_ADDR_FT3168, payload, len + 1, 100);
}

static esp_err_t I2C_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_master_write_read_device(TEST_I2C_PORT, I2C_ADDR_FT3168, &reg, 1, buf, len, 100);
}

void Touch_Init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {.clk_speed = 400000},
        .clk_flags = 0,
    };

    esp_err_t ret = i2c_param_config(TEST_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = i2c_driver_install(TEST_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C driver installed on SDA=%d SCL=%d",
             EXAMPLE_PIN_NUM_TOUCH_SDA, EXAMPLE_PIN_NUM_TOUCH_SCL);

    ret = I2C_write_reg(0x00, (uint8_t[]){0x00}, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FT3168 reset failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "FT3168 reset OK");
    }
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
    uint8_t data = 0;
    uint8_t buf[4] = {0};

    esp_err_t ret = I2C_read_reg(0x02, &data, 1);
    if (ret != ESP_OK) {
        return 0;
    }

    if (data) {
        ret = I2C_read_reg(0x03, buf, 4);
        if (ret != ESP_OK) {
            return 0;
        }

        *x = (((uint16_t)buf[0] & 0x0f) << 8) | (uint16_t)buf[1];
        *y = (((uint16_t)buf[2] & 0x0f) << 8) | (uint16_t)buf[3];

        static uint8_t s_touch_log_count = 0;
        if (s_touch_log_count < 10) {
            ESP_LOGI(TAG, "Touch raw: status=%d, buf=[%02X,%02X,%02X,%02X], x=%d, y=%d",
                     data, buf[0], buf[1], buf[2], buf[3], *x, *y);
            s_touch_log_count++;
        }

        if (*x > EXAMPLE_LCD_H_RES)
            *x = EXAMPLE_LCD_H_RES;
        if (*y > EXAMPLE_LCD_V_RES)
            *y = EXAMPLE_LCD_V_RES;

        return 1;
    }

    return 0;
}
