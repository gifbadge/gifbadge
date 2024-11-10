#include "png.h"
#include <string>
#include "bitbank2.h"
#include "image.h"

image::PNGImage::~PNGImage() {
    printf("PNG DELETED\n");
    png.close();
}

image::frameReturn image::PNGImage::loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
    pnguser config = {.png = &png, .buffer = outBuf, .x = x, .y = y, .width = width};
    png.decode((void *) &config, 0);
  return {image::frameStatus::END, 0};
}

std::pair<int16_t, int16_t> image::PNGImage::size() {
    return {png.getWidth(), png.getHeight()};
}

image::Image *image::PNGImage::create() {
    return new PNGImage();
}

typedef int32_t (*readfile)(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(PNGFILE *pFile, int32_t iPosition);


int image::PNGImage::open(const char *path, void *buffer) {
    return png.open(path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGDraw);
}

int image::PNGImage::open(uint8_t *bin, int size) {
    png.openRAM(bin, size, PNGDraw);
    return -1;
}

void image::PNGImage::PNGDraw(PNGDRAW *pDraw) {
    auto *config = (pnguser *) pDraw->pUser;
    auto *buffer = (uint16_t *) config->buffer;
    uint32_t y = (pDraw->y+config->y) * config->width;
    uint16_t *line = &buffer[y+config->x];
#ifdef ESP_PLATFORM
  config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
#else
  config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
#endif
}

const char * image::PNGImage::getLastError() {
    switch(png.getLastError()){
        case PNG_SUCCESS:
            return "PNG_SUCCESS";
        case PNG_INVALID_PARAMETER:
            return "PNG_INVALID_PARAMETER";
        case PNG_DECODE_ERROR:
            return "PNG_DECODE_ERROR";
        case PNG_MEM_ERROR:
            return "PNG_MEM_ERROR";
        case PNG_NO_BUFFER:
            return "PNG_NO_BUFFER";
        case PNG_UNSUPPORTED_FEATURE:
            return "PNG_UNSUPPORTED_FEATURE";
        case PNG_INVALID_FILE:
            return "PNG_INVALID_FILE";
        case PNG_TOO_BIG:
            return "PNG_TOO_BIG";
        default:
            return "Unknown";
    }
}
