cmake_minimum_required(VERSION 3.1...3.29)
project(FileUtil VERSION 1.0 LANGUAGES C CXX)
add_library(FileUtil "dirname.c" "directory.c" "path.c" "file.cpp")
target_include_directories(FileUtil PUBLIC "include")
set(C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(C_EXTENSIONS ON)
set(CXX_EXTENSIONS ON)


set_property(TARGET FileUtil PROPERTY C_STANDARD 11)
set_property(TARGET FileUtil PROPERTY C_EXTENSIONS ON)
set_property(TARGET FileUtil PROPERTY CXX_STANDARD 23)
set_property(TARGET FileUtil PROPERTY CXX_EXTENSIONS ON)
target_compile_options(FileUtil PUBLIC -D_GNU_SOURCE)

if(ESP_PLATFORM)
    target_link_libraries(FileUtil PRIVATE idf::newlib)
else ()
    include(CTest)
    add_subdirectory(test)
#    target_compile_options(FileUtil PUBLIC -g -fprofile-arcs -ftest-coverage)
endif()


