#include <array>
#include "image.h"
#include "jpeg.h"
#include "png.h"
#include "gif.h"
#include <sys/stat.h>

#include "bmp.h"

std::array<const char *, 5> extensionArray = {".gif", ".jpg", ".jpeg", ".png", ".bmp"};
std::span<const char *> extensions(extensionArray);
std::array<image::Image*(*)(const char *path), 5> handlers = {image::GIF::Create, image::JPEG::Create, image::JPEG::Create, image::PNGImage::Create, image::bmpImage::Create};

image::Image *ImageFactory(const char *path) {
  const char *ext = strrchr(path, '.');
  if(ext != nullptr) {
    for (int i = 0; i < extensionArray.size(); i++) {
      if (strcasecmp(extensionArray[i], ext) == 0) {
        return handlers[i](path);
      }
    }
  }
  return nullptr;
}