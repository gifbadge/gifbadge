#include "gif.h"
#include <string>
#include "bitbank2.h"

GIF::GIF() = default;

GIF::~GIF() {
    printf("GIF DELETED\n");
    gif.freeFrameBuf(free);
    gif.close();
}

int GIF::loop(uint8_t *outBuf) {
    GIFUser gifuser = {outBuf, width};
    int frameDelay;
    if (gif.playFrame(false, &frameDelay,  (void *) &gifuser) == -1) {
        return -1;
    }
    return frameDelay;
//    return gif->playFrame(false, nullptr, (void *) &gifuser);
}

std::pair<int, int> GIF::size() {
    return {gif.getCanvasWidth(), gif.getCanvasHeight()};
}

void GIF::GIFDraw(GIFDRAW *pDraw) {
    int y;
    uint16_t *d;

    auto *gifuser = static_cast<GIFUser *>(pDraw->pUser);
    auto *buffer = (uint16_t *) gifuser->buffer;

    y = pDraw->iY + pDraw->y; // current line
    d = &buffer[pDraw->iX + (y * gifuser->width)];
    memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
}

Image *GIF::create() {
    return new GIF();
}

void *GIFAlloc(uint32_t u32Size) {
    return malloc(u32Size);
} /* GIFAlloc() */


typedef int32_t (*readfile)(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);

typedef int32_t (*seekfile)(GIFFILE *pFile, int32_t iPosition);

int GIF::open(const char *path) {
#ifdef CONFIG_IMAGE_BIG_ENDIAN
    gif.begin(BIG_ENDIAN_PIXELS);
#else
    gif.begin(LITTLE_ENDIAN_PIXELS);
#endif
    if (gif.open(path, bb2OpenFile, bb2CloseFile, (readfile) bb2ReadFile, (seekfile) bb2SeekFile, GIFDraw)) {
        gif.allocFrameBuf(GIFAlloc);
        gif.setDrawType(GIF_DRAW_COOKED);
        width = gif.getCanvasWidth();
        return 0;
    }
    return -1;
}