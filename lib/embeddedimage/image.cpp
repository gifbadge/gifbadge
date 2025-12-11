#include <array>
#include "image.h"
#include <fcntl.h>
#include "jpeg.h"
#include "png.h"
#include "gif.h"

std::array<const char *, 4> extensionArray = {".gif", ".jpg", ".jpeg", ".png"};
std::array<const char *, 4> magicArray = {"GIF87a", "GIF89a", "\xFF\xD8\xFF", "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"};
std::span<const char *> extensions(extensionArray);
std::array<image::Image*(*)(image::screenResolution res), 4> handlers = {image::GIF::Create, image::GIF::Create, image::JPEG::Create, image::PNGImage::Create};


image::Image *ImageFactory(image::screenResolution res, const char *path) {
  int fd = open(path, O_RDONLY);
  char buffer[10];
  read(fd, buffer, 9);
  close(fd);
  buffer[9] = 0;
  for (int i = 0; i < magicArray.size(); i++) {
    if (strncmp(magicArray[i], buffer, strlen(magicArray[i])) == 0) {
      return handlers[i](res);
    }
  }
  return nullptr;
}