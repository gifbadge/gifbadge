#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#include "bitbank2.h"

struct mem_buf {
    uint8_t *buf;
    FILE *fp;
    uint8_t *pos;
    uint8_t *read;
    size_t size;
};

void *bb2OpenFile(const char *fname, int32_t *pSize) {
    FILE *infile = fopen(fname, "r");
    setvbuf(infile, nullptr, _IOFBF, 4096);
    struct stat stats{};

    if (fstat(fileno(infile), &stats) != 0) {
        return nullptr;
    }

    *pSize = stats.st_size;

    auto *mem = static_cast<mem_buf *>(malloc(sizeof(mem_buf)));
    mem->buf = static_cast<uint8_t *>(malloc(stats.st_size));
    if (mem->buf == nullptr) {
        printf("Not enough memory to buffer image\n");
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

void bb2CloseFile(void *pHandle) {
    auto *mem = (mem_buf *) (pHandle);
    fclose(mem->fp);
    if (mem->buf != nullptr) {
        free(mem->buf);
    }
    free(mem);
}

int32_t bb2ReadFile(bb2_file_tag *pFile, uint8_t *pBuf, int32_t iLen) {
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
        size_t bytes = fread(mem->read, 1, (mem->pos + iLen) - mem->read, mem->fp);
        int32_t len = mem->read - mem->pos + bytes;
        mem->read += bytes;
        memcpy(pBuf, mem->pos, len);
        mem->pos += len;
        pFile->iPos = mem->pos - mem->buf;
        return len;
    } else {
        memcpy(pBuf, mem->pos, iLen);
        mem->pos += iLen;
        pFile->iPos = mem->pos - mem->buf;
        return iLen;
    }
}

int32_t bb2SeekFile(bb2_file_tag *pFile, int32_t iPosition) {
    auto *mem = (mem_buf *) (pFile->fHandle);
    if (mem->buf == nullptr) {
        FILE *infile = (FILE *) (pFile->fHandle);
        fseek(infile, iPosition, SEEK_SET);
        pFile->iPos = (int32_t) ftell(infile);
        return pFile->iPos;
    } else {
        mem->pos = mem->buf + iPosition;
        pFile->iPos = iPosition;
        return iPosition;
    }
}
