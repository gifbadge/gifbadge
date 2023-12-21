project(EmbeddedImage)

file(GLOB PNGdec_src PNGdec/src/*.c PNGdec/src/*.cpp)

add_library(EmbeddedImage STATIC "image.cpp" "bitbank2.cpp" "jpeg.cpp" "png.cpp" "gif.cpp" "AnimatedGIF/src/AnimatedGIF.cpp" ${PNGdec_src} "tjpgd/tjpgd.c")
include_directories("include" "AnimatedGIF/src" "PNGdec/src")

include_directories("tjpgd")
include_directories("AnimatedGIF/src")
include_directories("PNGdec/src")


target_compile_options(EmbeddedImage PRIVATE -D__LINUX__)
