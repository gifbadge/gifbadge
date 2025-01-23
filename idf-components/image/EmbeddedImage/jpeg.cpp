#include "jpeg.h"

#include "bitbank2.h"
#include "png.h"
#include "image.h"
#include "resize.h"
#include "simplebmp.h"

struct pnguser {
  PNG *png;
  uint8_t *buffer;
  int16_t x;
  int16_t y;
  int16_t width;
};

bool image::JPEG::resizable() {
  if (_buffer) {
    return true;
  }
  return false;
}

image::JPEG::JPEG(const char *path): Image(path) {
}
image::JPEG::~JPEG() {
  printf("JPEG DELETED\n");
  jpeg.close();
}

std::pair<int16_t, int16_t> image::JPEG::Size() {
    return {jpeg.getWidth(), jpeg.getHeight()};
}

image::Image *image::JPEG::Create(const char *path) {
    return new image::JPEG(path);
}

int JPEGDraw(JPEGDRAW *pDraw){
  auto *config = static_cast<pnguser *>(pDraw->pUser);
  auto *buffer = reinterpret_cast<uint16_t *>(config->buffer);
  for(int iY = 0; iY < pDraw->iHeight ; iY++){
    int y = iY + pDraw->y + config->y; // current line
    uint16_t *d = &buffer[config->x + pDraw->x + (y * config->width)];
    memcpy(d, &pDraw->pPixels[iY*pDraw->iWidth], pDraw->iWidth * 2);
  }
return 1;
}


typedef int32_t (*readfile)(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(JPEGFILE *pFile, int32_t iPosition);

int image::JPEG::Open(void *buffer) {
  _buffer = buffer;
  int ret = jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  return ret==0; //Invert the return value
}

image::frameReturn image::JPEG::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  if (decoded) {
    jpeg.close();
    jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
  }
  decoded = true;
  pnguser config = {.png = nullptr, .buffer = outBuf, .x = x, .y = y, .width = width};
  jpeg.setUserPointer(&config);
  jpeg.decode(0, 0, 0);
  return {image::frameStatus::END, 0};
}

const char * image::JPEG::GetLastError() {
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

struct jpgresize {
  uint16_t *buffer;
  int width;
  int height;
  Resize *resize;
  int y;
};

int JPEGResize(JPEGDRAW *pDraw){
  auto *config = static_cast<jpgresize *>(pDraw->pUser);
  auto *buffer = config->buffer;
  for(int iY = 0; iY < pDraw->iHeight ; iY++){
    uint16_t *d = &buffer[pDraw->x + (iY * config->width)];
    memcpy(d, &pDraw->pPixels[iY*pDraw->iWidth], pDraw->iWidth * 2);
  }
  if (pDraw->x + pDraw->iWidth == config->width) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      config->resize->line(pDraw->y+y, &config->buffer[y*config->width]);
    }
  }
  return 1;
}

int image::JPEG::resize(int16_t x, int16_t y) {
  jpeg.close();
  jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGResize);
  jpeg.setPixelType(RGB565_BIG_ENDIAN);

  auto *outBuf = static_cast<uint16_t *>(malloc(x*y*2));
  if (outBuf == nullptr) {
    return -1;
  }
  Resize resize(jpeg.getWidth(), jpeg.getHeight(), x, y, outBuf);
  decoded = true;
  jpgresize config = {static_cast<uint16_t *>(_buffer), jpeg.getWidth(), jpeg.getHeight(), &resize, 0};
  jpeg.setUserPointer(&config);
  jpeg.decode(0, 0, 0);
  BMP bmp;
  bmp.width = x;
  bmp.height = y;
  bmp.planes = 1;
  bmp.bits = 16;
  bmp.compression = BMP_BITFIELDS;
  bmp.colors = 0;
  bmp.importantcolors = 0;
  bmp.header_size = 124;
  bmp.imagesize = bmp.width * bmp.height * 2;
  bmp.red_mask = 0xF800;
  bmp.green_mask = 0x07E0;
  bmp.blue_mask = 0x001F;
  char cachepath[255];
  CachedPath(_path, cachepath);
  strcat(cachepath, ".bmp");
  FILE *fo = fopen(cachepath, "wb");
  if (fo == nullptr) {
    return -1;
  }
  bmp_write_header(&bmp, fo);
  bmp_write(&bmp, reinterpret_cast<uint8_t *>(outBuf), fo);
  fclose(fo);
  free(outBuf);
  return 0;
  }