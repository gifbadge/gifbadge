add_executable(test_embedded_image
        test_png.cpp test_jpeg.cpp main.cpp
)

target_include_directories(test_embedded_image PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(test_embedded_image PUBLIC -g -fprofile-arcs -ftest-coverage)

get_filename_component(TEST_DATA_PATH testdata ABSOLUTE)
configure_file(config.h.in config.h)
target_include_directories(test_embedded_image PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

set_property(TARGET test_embedded_image PROPERTY C_STANDARD 11)
set_property(TARGET test_embedded_image PROPERTY C_EXTENSIONS ON)
set_property(TARGET test_embedded_image PROPERTY CXX_STANDARD 23)
set_property(TARGET test_embedded_image PROPERTY CXX_EXTENSIONS ON)

target_link_libraries(test_embedded_image
        EmbeddedImage
        unity
        gcov
        freertos_kernel
)

add_test(NAME embedded_image_test COMMAND test_embedded_image WORKING_DIRECTORY .)