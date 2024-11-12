cmake_minimum_required(VERSION 3.1...3.29)
project(FontRender VERSION 1.0 LANGUAGES C CXX)
file(GLOB mcufont_src ${CMAKE_SOURCE_DIR}/external-deps/mcufont/decoder/*.c)
add_library(FontRender "font_render.cpp" ${mcufont_src} "include/font_render.h")
target_include_directories(FontRender PUBLIC "${CMAKE_SOURCE_DIR}/external-deps/mcufont/decoder" "include")

if (NOT ESP_PLATFORM)
    include(CTest)
    add_subdirectory(test)
#    target_compile_options(FontRender PUBLIC -g -fprofile-arcs -ftest-coverage)
endif()


