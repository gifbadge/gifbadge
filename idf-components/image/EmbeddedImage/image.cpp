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

void CachedPath(const char *path, char *cachepath) {
  strcpy(cachepath, "/data/.cache");
  const char *relpath = strchr(path + 1, '/');
  strcat(cachepath, relpath);
}

image::Image::Image(const char *path) {
  strncpy(_path, path, 254);

}
image::Image *ImageFactory(const char *path) {
  if(const char *ext = strrchr(path, '.'); ext != nullptr) {
    // Check if the image is in our cache already
    struct stat buffer{};
    char cachepath[255];
    CachedPath(path, cachepath);
    if (strcasecmp(".gif", ext) != 0) {
      strcat(cachepath, ".bmp");
      if (stat(cachepath, &buffer) == 0) {
        return image::bmpImage::Create(cachepath);
      }
    }
    else {
      if (stat(cachepath, &buffer) == 0) {
        return image::GIF::Create(cachepath);
      }
    }

    // Otherwise load as normal
    for (int i = 0; i < extensionArray.size(); i++) {
      if (strcasecmp(extensionArray[i], ext) == 0) {
        return handlers[i](path);
      }
    }
  }
  return nullptr;
}