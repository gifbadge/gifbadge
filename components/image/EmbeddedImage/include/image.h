#pragma once

#include <cstdio>
#include <cstdint>
#include <tuple>
#include <memory>
#include <map>


class Image {

public:
    Image() = default;

    virtual ~Image() = default;

    virtual int loop(uint8_t *outBuf) {return 0;};

    virtual std::pair<int, int> size() {return{0,0};};

    virtual int open(const char *path) {return 0;};
    virtual bool animated() {return false;};
    virtual const char * getLastError() = 0;
};

Image *ImageFactory(const char *path);