#pragma once

#include "image.h"
#include <PNGdec.h>

struct pnguser {
    PNG *png;
    uint8_t *buffer;
};

class PNGImage : public Image {

public:
    PNGImage() = default;

    ~PNGImage() override;

    int loop(uint8_t *outBuf) override;

    std::pair<int16_t, int16_t> size() override;

    static Image *create();

    int open(const char *path) override;

    int open(uint8_t *bin, int size);

    const char * getLastError() override;


protected:
    PNG png;

    static void PNGDraw(PNGDRAW *pDraw);
};