#pragma once

#include "image.h"
#include <PNGdec.h>

struct pnguser {
    PNG *png;
    uint8_t *buffer;
    int16_t x;
    int16_t y;
    int16_t width;
};

class PNGImage : public Image {

public:
    PNGImage() = default;

    ~PNGImage() override;

    int loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> size() override;

    static Image *create();

    int open(const char *path, void *buffer) override;

    int open(uint8_t *bin, int size);

    const char * getLastError() override;


protected:
    PNG png;

    static void PNGDraw(PNGDRAW *pDraw);
};