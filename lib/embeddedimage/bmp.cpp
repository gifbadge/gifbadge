#include "bmp.h"
#include "simplebmp.h"

image::bmpImage::~bmpImage() {
  if (fp) {
    fclose(fp);
  }
}
image::frameReturn image::bmpImage::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  bmp_read_pdata(&_bmp, outBuf, fp);
  return {image::frameStatus::END, 0};
}
std::pair<int16_t, int16_t> image::bmpImage::Size() {
  return {_bmp.width, _bmp.height};
}
int image::bmpImage::Open(const char *path, void *buffer) {
  fp = fopen(path, "rb");
  if (fp == nullptr) {
    return -1;
  }
  bmp_read_header(&_bmp, fp);
  if (_bmp.compression != BMP_BITFIELDS) {
    _error = -1;
    return -1;
  }
  return 0;
}
bool image::bmpImage::Animated() {
  return false;
}
const char * image::bmpImage::GetLastError() {
  switch (_error) {
    case 0:
      return "No error";
    case -1:
      return "Incompatible BMP Format";
    default:
      return "Unknown error";
  }
}
image::Image *image::bmpImage::Create(screenResolution res) {
  return new bmpImage(res);
}
