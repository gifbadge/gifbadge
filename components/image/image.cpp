#include <cstdio>
#include <string>
#include <array>
#include "image.h"
#include "jpeg.h"
#include "./include/png.h"
#include "gif.h"

std::map<std::string, ImageFactory::TCreateMethod> ImageFactory::formats;

ImageFactory::ImageFactory() {
    ImageFactory::addFormat(".gif", GIF::create);
    ImageFactory::addFormat(".jpeg", JPEG::create);
    ImageFactory::addFormat(".jpg", JPEG::create);
    ImageFactory::addFormat(".png", PNGImage::create);
    for (auto &format: formats) {
        printf("%s\n", format.first.c_str());
    }
}

ImageFactory::~ImageFactory() = default;

bool ImageFactory::addFormat(const std::string &extension, TCreateMethod funcCreate) {
    formats[extension] = funcCreate;
    return true;
}

Image *ImageFactory::create(const char *path) {
    std::string spath(path);
    std::size_t dotPos = spath.find_last_of('.');
    std::string extension = spath.substr(dotPos, spath.length());
    printf("Extension: %s\n", extension.c_str());
    for (auto &c: extension) {
        c = tolower(c);
    }
    if (auto it = formats.find(extension); it != formats.end()) {
        return it->second(); // call the createFunc
    }

    return nullptr;
}