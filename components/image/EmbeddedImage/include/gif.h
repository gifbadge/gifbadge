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

class GIF : public Image {

public:
    GIF();

    ~GIF() override;

    int open(const char *path, void *buffer) override;

  frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> size() final;

    static Image * create();

    bool animated() override {return true;};

    const char *getLastError() override;


private:
    GIFIMAGE _gif;
    int playFrame(bool bSync, int *delayMilliseconds, GIFUser *pUser);

  static void GIFDraw(GIFDRAW *pDraw);
    int32_t width = 0; //GIF width, needed for GIFDraw
};