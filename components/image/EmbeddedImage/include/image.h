#pragma once

#include <cstdio>
#include <cstdint>
#include <tuple>
#include <memory>
#include <map>
#include <array>
#include <span>

enum class frameStatus {
  OK,
  END,
  ERROR
};

typedef std::pair<frameStatus, int> frameReturn;

class Image {

public:
    Image() = default;

    virtual ~Image() = default;

  virtual frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) { return {frameStatus::OK, 0}; };

    virtual std::pair<int16_t, int16_t> size() {return{0, 0};};

    virtual int open(const char *path, void *buffer) {return 0;};
    virtual bool animated() {return false;};
    virtual const char * getLastError() = 0;
};

extern std::array<const char *, 4> extensionArray;
extern std::span<const char *> extensions;
Image *ImageFactory(const char *path);