#include "lcd_bsp.h"
#include "esp_lcd_sh8601.h"
#include "lcd_config.h"
#include "FT3168.h"

static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_t *s_disp = NULL;
#define LCD_HOST SPI2_HOST

static esp_lcd_panel_io_handle_t amoled_panel_io_handle = NULL;

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] =
{
  {0x11, (uint8_t []){0x00}, 0, 80},
  {0xC4, (uint8_t []){0x80}, 1, 0},
  {0x35, (uint8_t []){0x00}, 1, 0},
  {0x53, (uint8_t []){0x20}, 1, 1},
  {0x63, (uint8_t []){0xFF}, 1, 1},
  {0x51, (uint8_t []){0x00}, 1, 1},
  {0x29, (uint8_t []){0x00}, 0, 10},
  {0x51, (uint8_t []){0xFF}, 1, 0},
};

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                            esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    if (!panel_handle) return;

    const int offsetx1 = area->x1 + 0x14;
    const int offsetx2 = area->x2 + 0x14;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;
    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t tp_x, tp_y;
    uint8_t win = getTouch(&tp_x, &tp_y);
    if (win) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static bool example_lvgl_lock(int timeout_ms)
{
    if (!lvgl_mux) return false;
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
    if (lvgl_mux) {
        xSemaphoreGiveRecursive(lvgl_mux);
    }
}

static void example_lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    for (;;) {
        if (example_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void lcd_lvgl_Init(void)
{
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    ESP_LOGI("LCD", "Initializing SPI bus...");
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        EXAMPLE_PIN_NUM_LCD_PCLK,
        EXAMPLE_PIN_NUM_LCD_DATA0,
        EXAMPLE_PIN_NUM_LCD_DATA1,
        EXAMPLE_PIN_NUM_LCD_DATA2,
        EXAMPLE_PIN_NUM_LCD_DATA3,
        EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI("LCD", "Creating panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(
        EXAMPLE_PIN_NUM_LCD_CS,
        example_notify_lvgl_flush_ready,
        &disp_drv);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    amoled_panel_io_handle = io_handle;

    ESP_LOGI("LCD", "Creating SH8601 panel...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = NULL,
    };

    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    panel_config.vendor_config = &vendor_config;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI("LCD", "Initializing LVGL...");
    lv_init();

    // Try to allocate DMA buffer, fall back to smaller size if needed
    uint32_t buf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t);
    ESP_LOGI("LCD", "Allocating DMA buffer, size: %lu bytes", buf_size);
    
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!buf1) {
        ESP_LOGW("LCD", "DMA buffer 1 allocation failed (%lu bytes), trying smaller size...", buf_size);
        // Try half size
        buf_size = EXAMPLE_LCD_H_RES * (EXAMPLE_LVGL_BUF_HEIGHT / 2) * sizeof(lv_color_t);
        buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
        if (!buf1) {
            ESP_LOGW("LCD", "DMA buffer 1 still failed, trying PSRAM...");
            buf1 = (lv_color_t *)ps_malloc(buf_size);
            if (!buf1) {
                ESP_LOGE("LCD", "Failed to allocate buffer 1 (both DMA and PSRAM)");
                ESP_LOGE("LCD", "Continuing without LVGL display - BLE will still work");
                return;
            }
        }
    }
    ESP_LOGI("LCD", "Buffer 1 allocated at %p, size %lu", buf1, buf_size);

    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!buf2) {
        ESP_LOGW("LCD", "DMA buffer 2 allocation failed, trying PSRAM...");
        buf2 = (lv_color_t *)ps_malloc(buf_size);
        if (!buf2) {
            ESP_LOGE("LCD", "Failed to allocate buffer 2, freeing buffer 1");
            free(buf1);
            ESP_LOGE("LCD", "Continuing without LVGL display - BLE will still work");
            return;
        }
    }
    ESP_LOGI("LCD", "Buffer 2 allocated at %p, size %lu", buf2, buf_size);

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buf_size / sizeof(lv_color_t));

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.rounder_cb = example_lvgl_rounder_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    s_disp = disp;

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (!lvgl_mux) {
        ESP_LOGE("LCD", "Failed to create LVGL mutex");
        return;
    }
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI("LCD", "LCD and LVGL initialized successfully");
}

lv_disp_t *get_display(void)
{
    return s_disp;
}

SemaphoreHandle_t get_lvgl_mutex(void)
{
    return lvgl_mux;
}

esp_err_t set_amoled_backlight(uint8_t brig)
{
    if (!amoled_panel_io_handle) return ESP_ERR_INVALID_STATE;
    uint32_t lcd_cmd = 0x51;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;
    return esp_lcd_panel_io_tx_param(amoled_panel_io_handle, lcd_cmd, &brig, 1);
}
