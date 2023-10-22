#pragma once
#include <cstdint>

typedef struct {
    int32_t iPos; // current file position
    int32_t iSize; // file size
    uint8_t *pData; // memory file pointer
    void *fHandle; // class pointer to File/SdFat or whatever you want
} bb2_file_tag;

void *bb2OpenFile(const char *fname, int32_t *pSize);

void bb2CloseFile(void *pHandle);

int32_t bb2ReadFile(bb2_file_tag *pFile, uint8_t *pBuf, int32_t iLen);

int32_t bb2SeekFile(bb2_file_tag *pFile, int32_t iPosition);
