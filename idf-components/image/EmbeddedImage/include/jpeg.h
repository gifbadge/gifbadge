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

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;

    std::pair<int16_t, int16_t> Size() override;

    static Image* Create();

    int Open(const char *path, void *buffer) override;

    const char * GetLastError() override;

private:
    JPEGDEC jpeg;
    bool decoded = false;
    char _path[255];
};
}

