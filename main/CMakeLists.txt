idf_component_register(SRCS
        "main.cpp"
        "ui/menu.cpp"
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
        "hal/boards/board_data.c"
        "hal/boards/board_v0.cpp"
        "hal/boards/board_2_1_v0_1.cpp"
        "hal/boards/board_2_1_v0_2.cpp"
        "hal/boards/board_1_28_v0_1.cpp"
        "hal/drivers/keys_gpio.cpp"
        "hal/drivers/keys_esp_io_expander.cpp"
        "hal/drivers/display_gc9a01.cpp"
        "hal/drivers/display_st7701s.cpp"
        "hal/drivers/backlight_ledc.cpp"
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
        "esp_efuse_custom_table.c"
        "ui/menu.cpp"
        "ui/device_group.c"
        "ui/file_select.cpp"
        "ui/main_menu.cpp"
        "ui/file_options.cpp"
        "ui/widgets/file_list/file_list.c"
        "ui/style.c"
        "ui/storage.cpp"
        "ui/usb_connected.cpp"
        "ui/device_info.cpp"
        INCLUDE_DIRS "include")
#target_compile_options(${COMPONENT_LIB} PRIVATE -DGC9A01=1 -DLV_USE_PERF_MONITOR=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -DST7706=1 -DLV_USE_PERF_MONITOR=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
#fatfs_create_spiflash_image(data fatfs FLASH_IN_PROJECT)
#nvs_create_partition_image(nvs nvs_default.csv FLASH_IN_PROJECT)


set(font_symbols "0xe3f4,0xe897,0xe41b,0xe41b,0xe425,0xe161,0xe5c9,0xe3ab,0xe2c7,0xe2c8,0xe623,0xf80e,0xf568")
message(font_symbols="${font_symbols}")

set(special_symbols "0xe1e0,0xe3f4")
message(special_symbols="${special_symbols}")

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
        COMMAND lv_font_conv --bpp 1 --size 28 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" --no-compress -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
        COMMAND lv_font_conv --bpp 1 --size 56 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" --no-compress -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
        COMMAND lv_font_conv --bpp 1 --size 180 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${special_symbols}" --no-compress -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
)

target_link_libraries(${COMPONENT_TARGET} "-u custom_app_desc")