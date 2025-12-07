cmake_minimum_required(VERSION 3.1...3.29)
project(FileBuffer VERSION 1.0 LANGUAGES C CXX)
add_library(FileBuffer "filebuffer.cpp")
target_include_directories(FileBuffer PUBLIC "include")
set(C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(C_EXTENSIONS ON)
set(CXX_EXTENSIONS ON)


set_property(TARGET FileBuffer PROPERTY C_STANDARD 11)
set_property(TARGET FileBuffer PROPERTY C_EXTENSIONS ON)
set_property(TARGET FileBuffer PROPERTY CXX_STANDARD 23)
set_property(TARGET FileBuffer PROPERTY CXX_EXTENSIONS ON)
target_compile_options(FileBuffer PUBLIC -D_GNU_SOURCE)


if(ESP_PLATFORM)
    target_link_libraries(FileBuffer PRIVATE idf::newlib)
    target_link_libraries(FileBuffer PRIVATE idf::freertos)
    target_compile_definitions(FileBuffer PUBLIC ESP_PLATFORM=1)

else ()
    target_link_libraries(FileBuffer PRIVATE freertos_kernel)
#    include(CTest)
#    add_subdirectory(test)
#    target_compile_options(FileBuffer PUBLIC -g -fprofile-arcs -ftest-coverage)
endif()


