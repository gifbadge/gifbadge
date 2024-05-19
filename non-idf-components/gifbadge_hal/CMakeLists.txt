cmake_minimum_required(VERSION 3.5)
project(gifbadge_hal C CXX)
add_library(gifbadge_hal keys.cpp)
target_include_directories(gifbadge_hal PUBLIC include)

## Include CMake file that provides ESP-IDF CMake build system APIs.
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
#
## Include ESP-IDF components in the build, may be thought as an equivalent of
## add_subdirectory() but with some additional processing and magic for ESP-IDF build
## specific build processes.
#idf_build_process(esp32)
#
## Create the project executable and plainly link the newlib component to it using
## its alias, idf::newlib.
#add_executable(${CMAKE_PROJECT_NAME}.elf main.c)
#target_link_libraries(${CMAKE_PROJECT_NAME}.elf idf::newlib)
#
## Let the build system know what the project executable is to attach more targets, dependencies, etc.
#idf_build_executable(${CMAKE_PROJECT_NAME}.elf)
#if (ESP_PLATFORM)
#    idf_component_register(SRCS keys.cpp INCLUDE_DIRS include)
#endif ()