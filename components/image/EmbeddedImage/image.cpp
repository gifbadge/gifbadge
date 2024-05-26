#include <array>
#include "image.h"
#include "jpeg.h"
#include "png.h"
#include "gif.h"

std::array<const char *, 4> extensionArray = {".gif", ".jpg", ".jpeg", ".png"};
std::span<const char *> extensions(extensionArray);
std::array<Image*(*)(), 4> handlers = {GIF::create, JPEG::create, JPEG::create, PNGImage::create};

Image *ImageFactory(const char *path) {
  const char *ext = strrchr(path, '.');
  if(ext != nullptr) {
    for (int i = 0; i < extensionArray.size(); i++) {
      if (strcasecmp(extensionArray[i], ext) == 0) {
        return handlers[i]();
      }
    }
  }
  return nullptr;
}