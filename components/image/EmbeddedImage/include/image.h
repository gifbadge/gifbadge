#pragma once

#include <cstdio>
#include <cstdint>
#include <tuple>
#include <memory>
#include <map>
#include <array>



class Image {

public:
    Image() = default;

    virtual ~Image() = default;

    virtual int loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {return 0;};

    virtual std::pair<int16_t, int16_t> size() {return{0, 0};};

    virtual int open(const char *path, void *buffer) {return 0;};
    virtual bool animated() {return false;};
    virtual const char * getLastError() = 0;
};

extern std::array<const char*, 4> extensions;

Image *ImageFactory(const char *path);