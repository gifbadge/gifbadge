if (ESP_PLATFORM)
    idf_component_register(SRCS
            "i2c.cpp"
            "hal_usb.cpp"
            "drivers/touch_ft5x06.cpp"
            "drivers/battery_analog.cpp"
            "drivers/battery_max17048.cpp"
            "boards/board_data.c"
            "boards/board_esp32s3.cpp"
            "boards/board_esp32s3_sdmmc.cpp"
            "boards/board_v0.cpp"
            "boards/b2_1_v0_2_v0_4_common.cpp"
            "boards/b2_1_v0_5.cpp"
            "boards/b2_1_v0_2.cpp"
            "boards/b2_1_v0_4.cpp"
            "boards/board_1_28_v0_1.cpp"
            "boards/board_1_28_v0_3.cpp"
            "drivers/keys_gpio.cpp"
            "drivers/keys_esp_io_expander.cpp"
            "drivers/display_gc9a01.cpp"
            "drivers/display_st7701s.cpp"
            "drivers/backlight_ledc.cpp"
            "drivers/config_nvs.cpp"
            "drivers/keys_generic.cpp"
            "drivers/esp_io_expander_gpio.cpp"
            "drivers/mfd_npm1300.cpp"
            "esp_efuse_custom_table.c"
            "hw_init.cpp"
            "ota.cpp"
            INCLUDE_DIRS public_include
            PRIV_INCLUDE_DIRS include
            REQUIRES
            esp_psram
            hal
            soc
            esp_timer
            esp_lcd
            esp_io_expander
            esp_io_expander_tca95xx_16bit
            tinyusb
            esp_tinyusb
            esp_io_expander_cat9532
            esp_adc
            nvs_flash
            efuse
            fatfs
            sdmmc
            app_update
            esp_lcd_gc9a01
            esp_lcd_panel_io_additions)
    target_link_libraries(${COMPONENT_LIB} PUBLIC "-u custom_app_desc")
    target_link_libraries(${COMPONENT_LIB} PUBLIC gifbadge_hal npmx)
endif ()