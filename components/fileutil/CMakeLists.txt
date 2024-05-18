if (ESP_PLATFORM)
    idf_component_register(SRCS dirname.c directory.c path.c
        INCLUDE_DIRS include
        REQUIRES unity)


    target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)

else ()
    cmake_minimum_required(VERSION 3.1...3.29)
    project(FileUtil VERSION 1.0 LANGUAGES C)
    add_library(FileUtil "dirname.c" "directory.c" "path.c")
    target_include_directories(FileUtil PUBLIC "include")
    set_property(TARGET FileUtil PROPERTY C_STANDARD 11)
    set_property(TARGET FileUtil PROPERTY C_EXTENSIONS ON)
endif ()