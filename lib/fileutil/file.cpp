/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cstring>
#include <span>
#include <sys/stat.h>

#include "file.h"

#ifdef ESP_PLATFORM
#include <ff.h>
#endif

bool is_file(const char *path) {
  struct stat buffer{};
  stat(path, &buffer);
  mode_t temp = (buffer.st_mode & S_IFREG);
  return temp > 0;
}

bool is_not_hidden(const char *path) {
  if (basename(path)[0] == '.' || basename(path)[0] == '~') {
    return false;
  }
#ifdef ESP_PLATFORM
  // Check if it's hidden file on a FAT FS
    FILINFO info;
    if (f_stat(&path[5], &info) == 0) {
      if (info.fattrib & AM_HID) {
        return false;
      }
    }
#endif
  return true;
}

bool is_valid_extension(const char *path, std::span<const char *> extensions) {
  const char *ext = strrchr(basename(path), '.');
  if (ext != nullptr) {
    for (auto &extension : extensions) {
      if (strcasecmp(extension, ext) == 0) {
        return true;
      }
    }
  }
  return false;
}

bool valid_image_file(const char *path, std::span<const char *> extensions) {
  return is_file(path) && is_not_hidden(path) && is_valid_extension(path, extensions);
}

