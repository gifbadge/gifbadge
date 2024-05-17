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

    add_compile_options( -D_WINDOWS_ )

#    if( TRACE_ON_ENTER )
#        add_compile_options( -DTRACE_ON_ENTER=1 )
#    else()
#        add_compile_options( -DTRACE_ON_ENTER=0 )
#    endif()

#    if( COVERAGE_TEST )
#        set( COVERAGE_TEST 1 )
#        set( NO_TRACING 1 )
#        add_compile_options( -DprojCOVERAGE_TEST=1 )
#        add_compile_options( -DprojENABLE_TRACING=0 )
#    else()
#        if( NO_TRACING )
            set( NO_TRACING 1 )
            add_compile_options( -DprojENABLE_TRACING=0 )
#        else()
#            set( NO_TRACING 0 )
#            add_compile_options( -DprojENABLE_TRACING=1 )
#        endif()
#        set( COVERAGE_TEST 0 )
        add_compile_options( -DprojCOVERAGE_TEST=0 )
#    endif()

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
    add_subdirectory(FreeRTOS-Kernel)

    target_compile_options( freertos_kernel
            PRIVATE
            # Trace macro cast pointer to int to store memory management event
            $<IF:${NO_TRACING},,-Wno-pointer-to-int-cast>
    )

    add_subdirectory(main)
    add_subdirectory(components/fileutil)
    add_subdirectory(components/font_render)
    add_subdirectory(components/image)
    add_subdirectory(components/lvgl__lvgl)
    target_link_libraries(GifBadge freertos_kernel image FileUtil FontRender lvgl)
endif ()