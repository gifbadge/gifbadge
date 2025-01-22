#pragma once
#include "image.h"
#include "simplebmp.h"

namespace image {
class bmpImage: public Image {
  public:
    bmpImage(const char *path);
    ~bmpImage() override;

    frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override;
    std::pair<int16_t, int16_t> Size() override;
    int Open(void *buffer) override;
    bool Animated() override;

    const char * GetLastError() override;
    static Image *Create(const char *path);

  private:
  FILE *fp = nullptr;
  BMP _bmp = {};
  int _error = 0;
};
}
