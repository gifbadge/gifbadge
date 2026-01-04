project(zmk_debounce C )
add_library(zmk_debounce STATIC "debounce.c")
target_include_directories(zmk_debounce PUBLIC include)
