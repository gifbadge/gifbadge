cmake_minimum_required(VERSION 3.5)
project(gifbadge_hal_linux C CXX)
add_library(gifbadge_hal_linux boards/board_linux.cpp drivers/backlight_dummy.cpp drivers/battery_dummy.cpp drivers/display_sdl.cpp drivers/console.c drivers/config_mem.cpp drivers/key_sdl.cpp hw_init.cpp portable_time.c)
target_include_directories(gifbadge_hal_linux PUBLIC public_include)
target_include_directories(gifbadge_hal_linux PRIVATE include)
target_link_libraries(gifbadge_hal_linux PUBLIC gifbadge_hal)
target_link_libraries(gifbadge_hal_linux PRIVATE freertos_kernel)
target_link_libraries(gifbadge_hal_linux PRIVATE ${SDL3_LIBRARIES})

