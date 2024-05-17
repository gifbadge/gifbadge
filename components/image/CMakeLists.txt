
file(GLOB PNGdec_src EmbeddedImage/PNGdec/src/*.c EmbeddedImage/PNGdec/src/*.cpp)

SET(SRC "EmbeddedImage/image.cpp"
        "EmbeddedImage/bitbank2.cpp"
        "EmbeddedImage/jpeg.cpp"
        "EmbeddedImage/png.cpp"
        "EmbeddedImage/gif.cpp"
        "EmbeddedImage/AnimatedGIF/src/AnimatedGIF.cpp"
        ${PNGdec_src}
        "EmbeddedImage/tjpgd/tjpgd.c")

SET(INCLUDE "EmbeddedImage/include"
        "EmbeddedImage/AnimatedGIF/src"
        "EmbeddedImage/PNGdec/src"
        "EmbeddedImage/tjpgd")

if (ESP_PLATFORM)
    idf_component_register(SRCS ${SRC}
            INCLUDE_DIRS ${INCLUDE})


    target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
else()
    cmake_minimum_required(VERSION 3.1...3.29)
    project(Image VERSION 1.0 LANGUAGES C CXX)
    add_library(image ${SRC} ${INCLUDE})
    target_include_directories(image PRIVATE ${INCLUDE})
    target_compile_options(image PRIVATE -D__LINUX__)

endif ()