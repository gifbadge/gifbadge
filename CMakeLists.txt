# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.24)


if (ESP_PLATFORM)
    set(COMPONENT_DIRS "$ENV{IDF_PATH}/components" "${CMAKE_SOURCE_DIR}/idf-components" "${CMAKE_SOURCE_DIR}/managed_components" "${CMAKE_SOURCE_DIR}/main")
    set(COMPONENTS "esp32" "esp_gdbstub" "esptool_py" "main" "log")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
endif ()

project(GifBadge LANGUAGES C CXX)
add_subdirectory(lib/gifbadge_hal)
add_subdirectory(lib/fileutil)
add_subdirectory(lib/font_render)
add_subdirectory(external-deps/npmx)

if (ESP_PLATFORM)
    if(EXISTS ${CMAKE_SOURCE_DIR}/config/esp32s3/lv_conf.h)
        message(LV_CONF_PATH=${CMAKE_SOURCE_DIR}/config/esp32s3/lv_conf.h)
        idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/config/esp32s3" APPEND)
    endif ()
#    idf_build_set_property()
#    target_compile_options(mbedcrypto PRIVATE "-Wno-maybe-uninitialized")
#    target_compile_options(mbedtls PRIVATE "-DMBEDTLS_ECP_LIGHT=1")
else()

    add_compile_options( -D__linux__)
    #FreeRTOS stuff
    set( NO_TRACING 1 )
    add_compile_options( -DprojENABLE_TRACING=0 )
    add_compile_options( -DprojCOVERAGE_TEST=0 )

    if( PROFILE )
        set( CMAKE_BUILD_TYPE "release" )
    else()
        set( CMAKE_BUILD_TYPE "debug" )
    endif()

    if( SANITIZE_ADDRESS )
        add_compile_options( -fsanitize=address -fsanitize=alignment )
    endif()

    if( SANITIZE_LEAK )
        add_compile_options( -fsanitize=leak )
    endif()

    # Add the freertos_config for FreeRTOS-Kernel
    add_library( freertos_config INTERFACE )

    target_include_directories( freertos_config
            INTERFACE
            ${CMAKE_CURRENT_LIST_DIR})

    # Select the heap port
    set( FREERTOS_HEAP "3" CACHE STRING "" FORCE)

    # Select the native compile PORT
    set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)

    # Adding the FreeRTOS-Kernel subdirectory
    add_subdirectory(external-deps/FreeRTOS-Kernel)

    target_compile_options(freertos_kernel
            PRIVATE
            # Trace macro cast pointer to int to store memory management event
            $<IF:${NO_TRACING},,-Wno-pointer-to-int-cast>
    )
    #End of FreeRTOS stuff


    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})

    add_subdirectory(main)
    add_subdirectory(idf-components/image)
    add_subdirectory(lib/gifbadge_hal_linux)
    add_subdirectory(idf-components/lvgl__lvgl)
    target_compile_options(lvgl PUBLIC -DLV_CONF_PATH=${CMAKE_SOURCE_DIR}/main/include/lv_conf_linux.h)
    target_link_libraries(GifBadge gifbadge_hal gifbadge_hal_linux freertos_kernel image FileUtil FontRender lvgl ${SDL2_LIBRARIES})
    set_property(TARGET GifBadge PROPERTY CXX_STANDARD 23)
    target_compile_options(GifBadge PRIVATE -fanalyzer)
    set(C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 23)
    set(C_EXTENSIONS ON)
    set(CXX_EXTENSIONS ON)
endif ()
