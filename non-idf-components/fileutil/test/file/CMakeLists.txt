add_executable(test_file
        file.cpp
)

target_compile_options(test_file PUBLIC -g -fprofile-arcs -ftest-coverage)


target_link_libraries(test_file
        FileUtil
        unity
        gcov
)

add_test(NAME file_test COMMAND test_file WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})