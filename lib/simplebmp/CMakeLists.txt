cmake_minimum_required(VERSION 3.1...3.29)
project(simplebmp VERSION 1.0 LANGUAGES C CXX)
add_library(simplebmp "simplebmp.cpp" "include/simplebmp.h")
target_include_directories(simplebmp PUBLIC "include")
#add_library(miniz "miniz.c")
#target_link_libraries(simplebmp miniz)
#target_include_directories(miniz PUBLIC "include")



