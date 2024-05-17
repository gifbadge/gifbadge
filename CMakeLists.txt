# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if (ESP_PLATFORM)
    set(COMPONENTS "esp32" "esp_gdbstub" "esptool_py" "main")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(esp32badge_software)

    #idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${CMAKE_SOURCE_DIR}/main/include/lv_conf.h" APPEND)
    #include(cmake_utilities)

else ()
    cmake_minimum_required(VERSION 3.1...3.29)
    project(GifBadge VERSION 1.0 LANGUAGES C CXX)

    add_subdirectory(main)
    add_subdirectory(components/fileutil)
    add_subdirectory(components/font_render)
    add_subdirectory(components/image)
    target_link_libraries(GifBadge image FileUtil FontRender)
endif ()