idf_component_register(
    SRCS 
    "display_bsp.cpp"
    "i2c_bsp.cpp"
    PRIV_REQUIRES
    main
    espressif__esp_lvgl_adapter
    INCLUDE_DIRS "./")
