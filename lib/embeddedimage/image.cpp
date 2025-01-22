/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <array>
#include "image.h"
#include <fcntl.h>
#include "jpeg.h"
#include "png.h"
#include "gif.h"
#include "bmp.h"

std::array<const char *, 5> extensionArray = {".gif", ".jpg", ".jpeg", ".png", ".bmp"};
std::array<const char *, 5> magicArray = {"GIF87a", "GIF89a", "\xFF\xD8\xFF", "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", "BM"};
std::span<const char *> extensions(extensionArray);
std::array<image::Image*(*)(image::screenResolution res, const char *path), 5> handlers = {image::GIF::Create, image::GIF::Create, image::JPEG::Create, image::PNGImage::Create, image::bmpImage::Create};

image::Image::Image(screenResolution res, const char *path) :resolution(res) {
  strncpy(_path, path, 254);
}

void CachedPath(const char *path, char *cachepath) {
  strcpy(cachepath, "/data/.cache");
  const char *relpath = strchr(path + 1, '/');
  strcat(cachepath, relpath);
}

image::Image *ImageFactory(image::screenResolution res, const char *path) {
  int fd = open(path, O_RDONLY);
  char buffer[10];
  read(fd, buffer, 9);
  close(fd);
  buffer[9] = 0;
  for (int i = 0; i < magicArray.size(); i++) {
    if (strncmp(magicArray[i], buffer, strlen(magicArray[i])) == 0) {
      return handlers[i](res, path);
    }
  }
  return nullptr;
}