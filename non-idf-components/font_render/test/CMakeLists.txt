add_executable(test_font
        test_font.c
)

target_compile_options(test_font PUBLIC -g -fprofile-arcs -ftest-coverage)


target_link_libraries(test_font
        FontRender
        unity
        gcov
)

add_test(NAME font_test COMMAND test_font WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})