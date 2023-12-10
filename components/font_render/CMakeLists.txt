file(GLOB mcufont_src mcufont/decoder/*.c)
idf_component_register(SRCS "font_render.cpp" ${mcufont_src}
        INCLUDE_DIRS "mcufont/decoder" "include")
