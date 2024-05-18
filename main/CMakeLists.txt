SET(SRC "main.cpp"
        "portable_time.c"
        "ui/menu.cpp"
        "display.cpp"
        "file_util.cpp"
        "ota.cpp"
        "input.cpp"
        "hal/keys.cpp"
        "hw_init.cpp"
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
        ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
        ${CMAKE_CURRENT_BINARY_DIR}/battery_symbols_14.c
        "ui/menu.cpp"
        "ui/device_group.c"
        "ui/file_select.cpp"
        "ui/main_menu.cpp"
        "ui/file_options.cpp"
        "ui/widgets/file_list/file_list.c"
        "ui/widgets/battery/lv_battery.c"
        "ui/style.c"
        "ui/storage.cpp"
        "ui/usb_connected.cpp"
        "ui/device_info.cpp"
)

if (ESP_PLATFORM)

    idf_component_register(SRCS
            ${SRC}
            "hal/esp32/i2c.cpp"
            "hal/esp32/hal_usb.cpp"
            "hal/esp32/drivers/touch_ft5x06.cpp"
            "hal/esp32/drivers/battery_analog.cpp"
            "hal/esp32/drivers/battery_max17048.cpp"
            "hal/esp32/boards/board_data.c"
            "hal/esp32/boards/board_v0.cpp"
            "hal/esp32/boards/board_2_1_v0_2.cpp"
            "hal/esp32/boards/board_2_1_v0_4.cpp"
            "hal/esp32/boards/board_1_28_v0_1.cpp"
            "hal/esp32/drivers/keys_gpio.cpp"
            "hal/esp32/drivers/keys_esp_io_expander.cpp"
            "hal/esp32/drivers/display_gc9a01.cpp"
            "hal/esp32/drivers/display_st7701s.cpp"
            "hal/esp32/drivers/backlight_ledc.cpp"
            "hal/esp32/drivers/config_nvs.cpp"
            "hal/esp32/esp_efuse_custom_table.c"
            INCLUDE_DIRS "include"
            REQUIRES esp_psram esp_adc nvs_flash efuse app_update image font_render lvgl esp_io_expander_cat9532 esp_io_expander tinyusb esp_tinyusb esp_timer fileutil)
    target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
    target_link_libraries(${COMPONENT_TARGET} "-u custom_app_desc")
    else ()
    cmake_minimum_required(VERSION 3.1...3.29)
    project(Main VERSION 1.0 LANGUAGES C CXX)
    add_executable(GifBadge ${SRC} hal/linux/boards/board_linux.cpp hal/linux/drivers/backlight_dummy.cpp hal/linux/drivers/battery_dummy.cpp hal/linux/drivers/display_sdl.cpp hal/linux/drivers/console.c hal/linux/drivers/config_mem.cpp hal/linux/drivers/key_sdl.cpp)
    target_include_directories(GifBadge PUBLIC "include")
endif ()

set(font_symbols "0xe3f4,0xe897,0xe41b,0xe41b,0xe425,0xe161,0xe5c9,0xe3ab,0xe2c7,0xe2c8,0xe623,0xf80e,0xf568,0xe5d8")
message(font_symbols="${font_symbols}")

set(special_symbols "0xe1e0,0xe3f4")
message(special_symbols="${special_symbols}")

set(battery_symbols "0xea0b,0xf22f,0xe5cd") #Charging,!, X

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
        COMMAND lv_font_conv --bpp 1 --size 28 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" --no-compress -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons.c
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
        COMMAND lv_font_conv --bpp 1 --size 56 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons_56.c
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/battery_symbols_14.c
        COMMAND lv_font_conv --bpp 1 --size 14 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght]-Filled.ttf -r"${battery_symbols}" --no-compress -o ${CMAKE_CURRENT_BINARY_DIR}/battery_symbols_14.c
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
        COMMAND lv_font_conv --bpp 1 --size 180 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${special_symbols}" -o ${CMAKE_CURRENT_BINARY_DIR}/material_icons_special.c
)