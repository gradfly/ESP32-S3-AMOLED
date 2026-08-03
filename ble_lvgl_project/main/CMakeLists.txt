idf_component_register(SRCS "main.c"
                              "ble_manager.c"
                              "ui/ui_main.c"
                              "bsp/lcd_bsp.c"
                              "bsp/FT3168.c"
                              "bsp/esp_lcd_sh8601.c"
                    INCLUDE_DIRS "." "ui" "bsp"
                    REQUIRES lvgl
                    COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1")
