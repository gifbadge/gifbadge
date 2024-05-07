#pragma once

#include <AnimatedGIF.h>
#include "image.h"
#include "bitbank2.h"

struct GIFUser{
    uint8_t *buffer;
    int32_t width;
};

class GIF : public Image {

public:
    GIF();

    ~GIF() override;

    int open(const char *path) override;

    int loop(uint8_t *outBuf) override;

    std::pair<int16_t, int16_t> size() final;

    static Image * create();

    bool animated() override {return true;};

    const char *getLastError() override;


private:
    AnimatedGIF gif;

    static void GIFDraw(GIFDRAW *pDraw);
    int32_t width = 0; //GIF width, needed for GIFDraw
};