#include <cstdio>
#include <string>
#include <array>
#include "image.h"
#include "jpeg.h"
#include "png.h"
#include "gif.h"

std::array<std::string_view, 4> extensions = {".gif", ".jpg", ".jpeg", ".png"};
std::array<Image*(*)(), 4> handlers = {GIF::create, JPEG::create, JPEG::create, PNGImage::create};

Image *ImageFactory(const char *path) {
  char extension[5] = {};
  strncpy(extension, strrchr(path, '.'), 4);
  printf("Extension: %s\n", extension);
  for (auto &c : extension) {
    c = tolower(c);
  }

  for (int i = 0; i < extensions.size(); i++) {
    if (extensions[i] == extension) {
      return handlers[i]();
    }
  }
  return nullptr;
}