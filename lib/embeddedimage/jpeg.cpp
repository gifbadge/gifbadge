/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "jpeg.h"
#include <string>

#include "bitbank2.h"
#include "png.h"
#include "image.h"

image::JPEG::~JPEG() {
  printf("JPEG DELETED\n");
  jpeg.close();
}

std::pair<int16_t, int16_t> image::JPEG::Size() {
    return {jpeg.getWidth(), jpeg.getHeight()};
}

image::Image *image::JPEG::Create(screenResolution res, const char *path) {
    return new image::JPEG(res, path);
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

int image::JPEG::Open(void *buffer) {
  int ret = jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  if (jpeg.getJPEGType() == JPEG_MODE_PROGRESSIVE) {
    // We don't want to support Progressive JPEGs. JPEGDec only supports reading their thumbnails
    _lastError = 255;
    return 1;
  }
  _lastError = jpeg.getJPEGType();
  return ret==0; //Invert the return value
}

image::frameReturn image::JPEG::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  if (decoded) {
    jpeg.close();
    jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  }
  decoded = true;
  pnguser config = {.png = nullptr, .buffer = outBuf, .x = x, .y = y, .width = width};
  jpeg.setUserPointer(&config);
  jpeg.decode(0, 0, 0);
  _lastError = jpeg.getLastError();
  return {image::frameStatus::END, 0};
}

const char * image::JPEG::GetLastError() {
    switch(_lastError){
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
      case 255:
        return "JPEG_PROGRESSIVE_NOT_SUPPORTED";
      default:
        return "UNKNOWN";
    };
}
