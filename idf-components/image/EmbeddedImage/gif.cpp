#include "gif.h"
#include <string>
#include "bitbank2.h"
#include "image.h"

#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

#define PICO_BUILD
#include <AnimatedGIF.h>
#define ALLOWS_UNALIGNED
// #undef LZW_HIGHWATER_TURBO
// #define LZW_HIGHWATER_TURBO ((LZW_BUF_SIZE_TURBO * 13) / 16)
#include <esp_log_buffer.h>
#include <gif.inl>

struct mem_buf {
  uint8_t *buf;
  FILE *fp;
  uint8_t *pos;
  uint8_t *read;
  size_t size;
};

static void * prev_buffer = nullptr;

static void *OpenFile(const char *fname, int32_t *pSize) {
  FILE *infile = fopen(fname, "r");
  // setvbuf(infile, nullptr, _IOFBF, 4096);

  if (infile == nullptr) {
    printf("Couldn't open file %s\n", fname);
  }

  struct stat stats{};

  if (fstat(fileno(infile), &stats) != 0) {
    printf("Couldn't stat file\n");
    return nullptr;
  }

  *pSize = stats.st_size;
  auto *mem = static_cast<mem_buf *>(malloc(sizeof(mem_buf)));
#ifdef ESP_PLATFORM
  if ((stats.st_size+480*480*3) <= heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)) {
    mem->buf = static_cast<uint8_t *>(heap_caps_malloc(stats.st_size, MALLOC_CAP_SPIRAM));
  } else {
    printf("Not enough memory to buffer image\n");
    mem->buf = nullptr;
  }
  if (mem->buf == nullptr) {
    printf("Couldn't allocate buffer\n");
  }
#else
  mem->buf = nullptr;
#endif
  mem->fp = infile;
  mem->pos = mem->buf;
  mem->read = mem->buf;
  mem->size = stats.st_size;

  if (infile) {
    return mem;
  }
  return nullptr;
}

static void CloseFile(void *pHandle) {
  auto *mem = (mem_buf *) (pHandle);
  fclose(mem->fp);
  if (mem->buf != nullptr) {
    free(mem->buf);
  }
  free(mem);
}

static int32_t ReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  if (iLen <= 0) {
    return 0;
  }
  int32_t iBytesRead;
  iBytesRead = iLen;
  auto *mem = (mem_buf *) (pFile->fHandle);
  if (mem->buf == nullptr) {
    iBytesRead = (int32_t) fread(pBuf, 1, iBytesRead, mem->fp);
    pFile->iPos = ftell(mem->fp);
    return iBytesRead;
  }
  if (mem->pos + iLen > mem->read) {
    //We don't have enough in the buffer, read and add it
    size_t bytes = fread(mem->read, 1, (mem->pos + iLen) - mem->read, mem->fp);
    int32_t len = mem->read - mem->pos + bytes;
    mem->read += bytes;
    memcpy(pBuf, mem->pos, len);
    mem->pos += len;
    pFile->iPos = mem->pos - mem->buf;
    return len;
  } else {
    //Already in the buffer, copy it out
    memcpy(pBuf, mem->pos, iLen);
    mem->pos += iLen;
    pFile->iPos = mem->pos - mem->buf;
    return iLen;
  }
}

static int32_t SeekFile(GIFFILE *pFile, int32_t iPosition) {
  auto *mem = (mem_buf *) (pFile->fHandle);
  if (mem->buf == nullptr) {
    fseek(mem->fp, iPosition, SEEK_SET);
    pFile->iPos = (int32_t) ftell(mem->fp);
    return pFile->iPos;
  } else {
    mem->pos = mem->buf + iPosition;
    pFile->iPos = iPosition;
    return iPosition;
  }
}

image::GIF::GIF() = default;

image::GIF::~GIF() {
  printf("GIF DELETED\n");
  free(_gif.pFrameBuffer);
  _gif.pFrameBuffer = nullptr;
  (*_gif.pfnClose)(_gif.GIFFile.fHandle);
}

int image::GIF::playFrame(bool bSync, int *delayMilliseconds, GIFUser *pUser)
{
  int rc;

  if (_gif.GIFFile.iPos >= _gif.GIFFile.iSize-1) // no more data exists
  {
    (*_gif.pfnSeek)(&_gif.GIFFile, 0); // seek to start
  }
  if (GIFParseInfo(&_gif, 0))
  {
    _gif.pUser = pUser;
    // if((_gif.iHeight != _gif.iCanvasHeight)|(_gif.iCanvasWidth != _gif.iWidth)){
    //   auto *previous = static_cast<uint16_t *>(prev_buffer);
    //   previous += pUser->y * pUser->width;
    //   auto * current = (uint16_t *)pUser->buffer;
    //   current += pUser->y * pUser->width;
    //   memcpy(current, previous, _gif.iCanvasHeight*pUser->width*2);
    // }
    if (_gif.iError == GIF_EMPTY_FRAME) // don't try to decode it
      return 0;
    if (_gif.pTurboBuffer) {
      rc = DecodeLZWTurbo(&_gif, 0);
    } else {
      rc = DecodeLZW(&_gif, 0);
    }
    if (rc != 0) // problem
      return -1;
  }
  else
  {
    // The file is "malformed" in that there is a bunch of non-image data after
    // the last frame. Return as if all is well, though if needed getLastError()
    // can be used to see if a frame was actually processed:
    // GIF_SUCCESS -> frame processed, GIF_EMPTY_FRAME -> no frame processed
    if (_gif.iError == GIF_EMPTY_FRAME)
    {
      if (delayMilliseconds)
        *delayMilliseconds = 0;
      return 0;
    }
    return -1; // error parsing the frame info, we may be at the end of the file
  }
  if (delayMilliseconds) // if not NULL, return the frame delay time
    *delayMilliseconds = _gif.iFrameDelay;
  if (!_gif.pfnDraw) {
    memcpy(pUser->buffer, &_gif.pFrameBuffer[480 * 480], 480 * 480 * 2);
  }
  return (_gif.GIFFile.iPos < _gif.GIFFile.iSize-10);
} /* playFrame() */

image::frameReturn image::GIF::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  GIFUser gifuser = {outBuf, x, y, width};
  int frameDelay;
  int ret = playFrame(false, &frameDelay, &gifuser);
  prev_buffer = outBuf;
  if (ret == -1) {
    printf("GIF Error: %i\n", _gif.iError);
    return {frameStatus::ERROR, 0};
  } else if (ret == 0) {
    return {frameStatus::END, frameDelay};
  }
  return {frameStatus::OK, frameDelay};
}

std::pair<int16_t, int16_t> image::GIF::Size() {
  return {_gif.iCanvasWidth, _gif.iCanvasHeight};
}

void image::GIF::GIFDraw(GIFDRAW *pDraw) {
  int y;
  uint16_t *d;

  auto *gifuser = static_cast<GIFUser *>(pDraw->pUser);
  auto *buffer = (uint16_t *) gifuser->buffer;

  y = pDraw->iY + pDraw->y + gifuser->y; // current line
  d = &buffer[gifuser->x + pDraw->iX + (y * gifuser->width)];
  memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
}

image::Image *image::GIF::Create() {
  return new image::GIF();
}

int image::GIF::Open(const char *path, void *buffer) {
#ifdef ESP_PLATFORM
  unsigned char ucPaletteType = BIG_ENDIAN_PIXELS;
#else
  unsigned char ucPaletteType = LITTLE_ENDIAN_PIXELS;
#endif

  memset(&_gif, 0, sizeof(_gif));
  if (ucPaletteType != GIF_PALETTE_RGB565_LE && ucPaletteType != GIF_PALETTE_RGB565_BE
      && ucPaletteType != GIF_PALETTE_RGB888)
    _gif.iError = GIF_INVALID_PARAMETER;
  _gif.ucPaletteType = ucPaletteType;
  _gif.ucDrawType = GIF_DRAW_RAW; // assume RAW pixel handling
  _gif.pFrameBuffer = nullptr;

  _gif.iError = GIF_SUCCESS;
  _gif.pfnRead = ReadFile;
  _gif.pfnSeek = SeekFile;
  _gif.pfnDraw = nullptr;
  _gif.pfnOpen = OpenFile;
  _gif.pfnClose = CloseFile;
  _gif.GIFFile.fHandle = (*_gif.pfnOpen)(path, &_gif.GIFFile.iSize);
  if (_gif.GIFFile.fHandle == nullptr) {
    _gif.iError = GIF_FILE_NOT_OPEN;
    return -1;
  }

  if (GIFInit(&_gif)) {
    if (buffer) {
      _gif.pTurboBuffer = (uint8_t *) buffer;
    }
    // Allocate a little extra space for the current line
    // as RGB565 or RGB888
    int iCanvasSize = 480*480 * 3 ;
#ifdef ESP_PLATFORM
    if (iCanvasSize < heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)) {
      _gif.pFrameBuffer = (unsigned char *) heap_caps_malloc(iCanvasSize, MALLOC_CAP_SPIRAM);
    } else {
      _gif.pFrameBuffer = (unsigned char *) heap_caps_malloc(_gif.iCanvasWidth * (_gif.iCanvasHeight + 3), MALLOC_CAP_SPIRAM);
      _gif.pfnDraw = GIFDraw;
    }
#else
    _gif.pFrameBuffer = (unsigned char *)malloc(iCanvasSize);
#endif
    if (_gif.pFrameBuffer == nullptr) {
      _gif.iError = GIF_ERROR_MEMORY;
      return -1;
    }
    _gif.ucDrawType = GIF_DRAW_COOKED;
    width = _gif.iCanvasWidth;
    return 0;
  }
  return -1;
}

const char *image::GIF::GetLastError() {
  switch (_gif.iError) {
    case GIF_SUCCESS:
      return "GIF_SUCCESS";
    case GIF_DECODE_ERROR:
      return "GIF_DECODE_ERROR";
    case GIF_TOO_WIDE:
      return "GIF_TOO_WIDE";
    case GIF_INVALID_PARAMETER:
      return "GIF_INVALID_PARAMETER";
    case GIF_UNSUPPORTED_FEATURE:
      return "GIF_UNSUPPORTED_FEATURE";
    case GIF_FILE_NOT_OPEN:
      return "GIF_FILE_NOT_OPEN";
    case GIF_EARLY_EOF:
      return "GIF_EARLY_EOF";
    case GIF_EMPTY_FRAME:
      return "GIF_EMPTY_FRAME";
    case GIF_BAD_FILE:
      return "GIF_BAD_FILE";
    case GIF_ERROR_MEMORY:
      return "GIF_ERROR_MEMORY";
    default:
      return "Unknown";
  }
}
