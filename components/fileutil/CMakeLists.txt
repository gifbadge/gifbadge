idf_component_register(SRCS dirname.c directory.c path.c
        INCLUDE_DIRS include
        REQUIRES unity)


target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
