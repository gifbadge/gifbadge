#pragma once

#include <AnimatedGIF.h>
#include "image.h"
#include "bitbank2.h"

struct GIFUser{
    uint8_t *buffer;
  int16_t x;
  int16_t y;
    int32_t width;
};

namespace image {
class GIF : public Image {

public:
    GIF(const char *path);

    ~GIF() override;

    int Open(void *buffer) override;

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> Size() final;

    static Image * Create(const char *path);

    bool Animated() override {return true;};

    const char *GetLastError() override;


private:
    GIFIMAGE _gif;
    int playFrame(bool bSync, int *delayMilliseconds, GIFUser *pUser);

  static void GIFDraw(GIFDRAW *pDraw);
    int32_t width = 0; //GIF width, needed for GIFDraw
};
}

