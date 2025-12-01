
file(GLOB PNGdec_src ${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src/*.c ${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src/*.cpp)
file(GLOB JPEGDEC_src ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.c ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.cpp ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.S)


SET(SRC "EmbeddedImage/image.cpp"
        "EmbeddedImage/bitbank2.cpp"
        "EmbeddedImage/jpeg.cpp"
        "EmbeddedImage/png.cpp"
        "EmbeddedImage/gif.cpp"
        ${PNGdec_src}
        ${JPEGDEC_src}
)

SET(INCLUDE "EmbeddedImage/include"
        "${CMAKE_SOURCE_DIR}/external-deps/AnimatedGIF/src"
        "${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src"
        "${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src"
)

if (ESP_PLATFORM)
    idf_component_register(SRCS ${SRC}
            INCLUDE_DIRS ${INCLUDE} REQUIRES esp-dsp)

    add_compile_definitions(ALLOWS_UNALIGNED)
    target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__ -DARDUINO_ARCH_ESP32=1)
    target_link_libraries(${COMPONENT_LIB} FileBuffer)
else()
    cmake_minimum_required(VERSION 3.1...3.29)
    project(Image VERSION 1.0 LANGUAGES C CXX ASM)
    add_library(image ${SRC} ${INCLUDE})
    target_include_directories(image PUBLIC ${INCLUDE})
    target_compile_options(image PUBLIC -D__LINUX__)
    set_property(TARGET image PROPERTY C_STANDARD 11)
    set_property(TARGET image PROPERTY C_EXTENSIONS ON)
    set_property(TARGET image PROPERTY CXX_STANDARD 23)
    set_property(TARGET image PROPERTY CXX_EXTENSIONS ON)

endif ()