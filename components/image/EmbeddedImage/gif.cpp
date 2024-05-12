#include "gif.h"
#include <string>
#include "bitbank2.h"

#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#include <esp_heap_caps.h>

struct mem_buf {
    uint8_t *buf;
    FILE *fp;
    uint8_t *pos;
    uint8_t *read;
    size_t size;
};

static void *OpenFile(const char *fname, int32_t *pSize) {
    FILE *infile = fopen(fname, "r");
    setvbuf(infile, nullptr, _IOFBF, 4096);
    struct stat stats{};

    if (fstat(fileno(infile), &stats) != 0) {
        return nullptr;
    }

    *pSize = stats.st_size;

  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
  auto *mem = static_cast<mem_buf *>(malloc(sizeof(mem_buf)));
    if(stats.st_size <= heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)){
      mem->buf = static_cast<uint8_t *>(heap_caps_malloc(stats.st_size, MALLOC_CAP_SPIRAM));
    }
    else {
      printf("Not enough memory to buffer image\n");
      mem->buf = nullptr;
    }
    if (mem->buf == nullptr) {
    }
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

GIF::GIF() = default;

GIF::~GIF() {
    printf("GIF DELETED\n");
    gif.freeFrameBuf(free);
    gif.close();
}

int GIF::loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
    GIFUser gifuser = {outBuf, x, y, width};
    int frameDelay;
    if (gif.playFrame(false, &frameDelay,  (void *) &gifuser) == -1) {
        printf("GIF Error: %i\n", gif.getLastError());
        return -1;
    }
    return frameDelay;
//    return gif->playFrame(false, nullptr, (void *) &gifuser);
}

std::pair<int16_t, int16_t> GIF::size() {
    return {gif.getCanvasWidth(), gif.getCanvasHeight()};
}

void GIF::GIFDraw(GIFDRAW *pDraw) {
    int y;
    uint16_t *d;

    auto *gifuser = static_cast<GIFUser *>(pDraw->pUser);
    auto *buffer = (uint16_t *) gifuser->buffer;

    y = pDraw->iY + pDraw->y + gifuser->y; // current line
    d = &buffer[gifuser->x + pDraw->iX + (y * gifuser->width)];
    memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
}

Image *GIF::create() {
    return new GIF();
}

void *GIFAlloc(uint32_t u32Size) {
    return heap_caps_malloc(u32Size, MALLOC_CAP_SPIRAM);
} /* GIFAlloc() */


typedef int32_t (*readfile)(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);

typedef int32_t (*seekfile)(GIFFILE *pFile, int32_t iPosition);

int GIF::open(const char *path, void *buffer) {
    gif.begin(BIG_ENDIAN_PIXELS);
    if (gif.open(path, OpenFile, CloseFile, (readfile) ReadFile, (seekfile) SeekFile, GIFDraw)) {
      if(buffer){
        gif.setTurboBuf(buffer);
      }
        gif.allocFrameBuf(GIFAlloc);
        gif.setDrawType(GIF_DRAW_COOKED);
        width = gif.getCanvasWidth();
        return 0;
    }
    return -1;
}

const char *GIF::getLastError() {
    switch (gif.getLastError()) {
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
