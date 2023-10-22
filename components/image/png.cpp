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
//    png = new PNG();
    png.open(path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGDraw);

    return -1;
}

void PNGImage::PNGDraw(PNGDRAW *pDraw) {
    auto *config = (pnguser *) pDraw->pUser;
    auto *buffer = (uint16_t *) config->buffer;
    uint16_t *line = &buffer[pDraw->y * pDraw->iWidth];
#ifdef CONFIG_IMAGE_BIG_ENDIAN
    config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
#else
    config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
#endif

}
