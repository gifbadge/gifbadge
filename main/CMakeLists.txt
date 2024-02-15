idf_component_register(SRCS
        "main.cpp"
        "ui/lvgl.cpp"
        "keys.cpp"
        "display.cpp"
        "config.cpp"
        "file_util.cpp"
        "ota.cpp"
        "hal/i2c.cpp"
        "hal/hal_usb.cpp"
        "hal/drivers/touch_ft5x06.cpp"
        "hal/drivers/battery_analog.cpp"
        "hal/drivers/battery_max17048.cpp"
        "hw_init.cpp"
        "hal/boards/board_v0.cpp"
        "hal/boards/board_2_1_v0_1.cpp"
        "hal/drivers/keys_gpio.cpp"
        "hal/drivers/display_gc9a01.cpp"
        "hal/drivers/display_st7701s.cpp"
        "hal/drivers/backlight_ledc.cpp"
        "material_icons.c"
        "material_icons_56.c"
        "esp_efuse_custom_table.c"
        "ui/lvgl.cpp"
        "ui/device_group.c"
        "ui/file_select.cpp"
        "ui/main_menu.cpp"
        "ui/file_options.cpp"
        "ui/widgets/file_list/file_list.c"
        "ui/style.c"
        "ui/storage.cpp"
        INCLUDE_DIRS "include")
#target_compile_options(${COMPONENT_LIB} PRIVATE -DGC9A01=1 -DLV_USE_PERF_MONITOR=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -DST7706=1 -DLV_USE_PERF_MONITOR=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
#fatfs_create_spiflash_image(data fatfs FLASH_IN_PROJECT)
#nvs_create_partition_image(nvs nvs_default.csv FLASH_IN_PROJECT)
