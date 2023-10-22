file(GLOB PNGdec_src PNGdec/src/*.c PNGdec/src/*.cpp)

idf_component_register(SRCS "image.cpp" "bitbank2.cpp" "jpeg.cpp" "png.cpp" "gif.cpp" "AnimatedGIF/src/AnimatedGIF.cpp" ${PNGdec_src} "tjpgd/tjpgd.c"
        INCLUDE_DIRS "include" "AnimatedGIF/src" "PNGdec/src")

include_directories("tjpgd")
include_directories("AnimatedGIF/src")
include_directories("PNGdec/src")


target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)