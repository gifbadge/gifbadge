cmake_minimum_required(VERSION 3.5)
project(gifbadge_hal C CXX)
add_library(gifbadge_hal keys.cpp)
target_include_directories(gifbadge_hal PUBLIC include)
