#pragma once
#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif
  void FileBufferTask(void *);
  void openFile(const char *path);
  void closeFile();
  // void readFile(uint8_t *pBuf, int32_t iLen);
  size_t readFile(uint8_t *pBuf, int32_t iLen);
  void seekFile(int32_t pos);

#ifdef __cplusplus
}
#endif