file(GLOB PNGdec_src EmbeddedImage/PNGdec/src/*.c EmbeddedImage/PNGdec/src/*.cpp)

idf_component_register(SRCS "EmbeddedImage/image.cpp" "EmbeddedImage/bitbank2.cpp" "EmbeddedImage/jpeg.cpp" "EmbeddedImage/png.cpp" "EmbeddedImage/gif.cpp" "EmbeddedImage/AnimatedGIF/src/AnimatedGIF.cpp" ${PNGdec_src} "EmbeddedImage/tjpgd/tjpgd.c"
        INCLUDE_DIRS "EmbeddedImage/include" "EmbeddedImage/AnimatedGIF/src" "EmbeddedImage/PNGdec/src")

include_directories("EmbeddedImage/tjpgd")
include_directories("EmbeddedImage/AnimatedGIF/src")
include_directories("EmbeddedImage/PNGdec/src")


#target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
