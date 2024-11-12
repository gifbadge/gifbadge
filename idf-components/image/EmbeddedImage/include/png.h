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

namespace image {

class PNGImage : public image::Image {

public:
    PNGImage() = default;

    ~PNGImage() override;

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> Size() override;

    static Image *Create();

    int Open(const char *path, void *buffer) override;

    int Open(uint8_t *bin, int size);

    const char * GetLastError() override;


protected:
    PNG png;

    static void PNGDraw(PNGDRAW *pDraw);
};
}

