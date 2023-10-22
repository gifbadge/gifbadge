#pragma once

#include <cstdint>
#include <sys/types.h>

/* When Tiny JPG Decoder is not in ROM or selected external code */
#include "tjpgd.h"

#define JPEG_WORK_BUF_SIZE 65472
#define ESP_JPEG_COLOR_BYTES 2

/* The TJPGD outside the ROM code is newer and has different return type in decode callback */
typedef int jpeg_decode_out_t;

#include "image.h"

struct JPGuser {
    FILE *infile;
    uint8_t *outBuf;
    size_t size;
};

class JPEG : public Image {
public:
    JPEG() = default;

    ~JPEG() override;

    int loop(uint8_t *outBuf) override;

    std::pair<int, int> size() override;

    static Image* create();

    int open(const char *path) override;

private:
    JPGuser jpguser{};
    JDEC _dec{};

    static unsigned int jpeg_decode_in_cb(JDEC *dec, uint8_t *buff, unsigned int nbyte);

    static jpeg_decode_out_t jpeg_decode_out_cb(JDEC *dec, void *bitmap, JRECT *rect);

    uint8_t *workbuf{};
};