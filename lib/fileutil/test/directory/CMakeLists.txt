add_executable(test_directory
        directory.c
)

target_compile_options(test_directory PUBLIC -g -fprofile-arcs -ftest-coverage)


target_link_libraries(test_directory
        FileUtil
        unity
        gcov
)

add_test(NAME directory_test COMMAND test_directory WORKING_DIRECTORY .)