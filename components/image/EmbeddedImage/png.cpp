#include "png.h"
#include <string>
#include "bitbank2.h"

PNGImage::~PNGImage() {
    printf("PNG DELETED\n");
    png.close();
}

int PNGImage::loop(uint8_t *outBuf) {
    pnguser config = {.png = &png, .buffer = outBuf};
    png.decode((void *) &config, 0);
    return -1;
}

std::pair<int, int> PNGImage::size() {
    return {png.getWidth(), png.getHeight()};
}

Image *PNGImage::create() {
    return new PNGImage();
}

typedef int32_t (*readfile)(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(PNGFILE *pFile, int32_t iPosition);


int PNGImage::open(const char *path) {
    return png.open(path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGDraw);
}

int PNGImage::open(uint8_t *bin, int size) {
    png.openRAM(bin, size, PNGDraw);
    return -1;
}

void PNGImage::PNGDraw(PNGDRAW *pDraw) {
    auto *config = (pnguser *) pDraw->pUser;
    auto *buffer = (uint16_t *) config->buffer;
    uint16_t *line = &buffer[pDraw->y * pDraw->iWidth];
    config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
}

std::string PNGImage::getLastError() {
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
