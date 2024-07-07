#include "jpeg.h"
#include <string>

#include "bitbank2.h"
#include "png.h"

JPEG::~JPEG() {
  printf("JPEG DELETED\n");
  jpeg.close();
}


frameReturn JPEG::loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  pnguser config = {.png = nullptr, .buffer = outBuf, .x = x, .y = y, .width = width};
  jpeg.setUserPointer(&config);
  jpeg.decode(0, 0, 0);
  return {frameStatus::END, 0};
}

std::pair<int16_t, int16_t> JPEG::size() {
    return {jpeg.getWidth(), jpeg.getHeight()};
}

Image *JPEG::create() {
    return new JPEG();
}

int JPEGDraw(JPEGDRAW *pDraw){
  uint16_t *d;
  int y;
  auto *config = (pnguser *) pDraw->pUser;
  auto *buffer = (uint16_t *) config->buffer;
  for(int iY = 0; iY < pDraw->iHeight ; iY++){
    y = iY+pDraw->y + config->y; // current line
    d = &buffer[config->x + pDraw->x + (y * config->width)];
    memcpy(d, &pDraw->pPixels[iY*pDraw->iWidth], pDraw->iWidth * 2);
  }
return 1;
}


typedef int32_t (*readfile)(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(JPEGFILE *pFile, int32_t iPosition);

int JPEG::open(const char *path, void *buffer) {
  int ret = jpeg.open(path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  return ret==0; //Invert the return value
}

const char * JPEG::getLastError() {
    switch(jpeg.getLastError()){
      case JPEG_SUCCESS:
        return "JPEG_SUCCESS";
      case JPEG_INVALID_PARAMETER:
        return "JPEG_INVALID_PARAMETER";
      case JPEG_DECODE_ERROR:
        return "JPEG_DECODE_ERROR";
      case JPEG_UNSUPPORTED_FEATURE:
        return "JPEG_UNSUPPORTED_FEATURE";
      case JPEG_INVALID_FILE:
        return "JPEG_INVALID_FILE";
      default:
        return "UNKNOWN";
    };
}
