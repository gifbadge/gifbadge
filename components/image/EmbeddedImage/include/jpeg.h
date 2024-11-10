#pragma once

#include <cstdint>
#include <sys/types.h>

#include <JPEGDEC.h>

#include "image.h"

namespace image {

class JPEG : public image::Image {
public:
    JPEG() = default;

    ~JPEG() override;

  frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> size() override;

    static Image* create();

    int open(const char *path, void *buffer) override;

    const char * getLastError() override;

private:
    JPEGDEC jpeg;
};
}

