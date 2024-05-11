#pragma once

#include <string>
#include <mutex>

#include <nvs_flash.h>
#include <nvs_handle.hpp>

class ImageConfig {
 public:
  ImageConfig();
  ~ImageConfig();
  void save();

  void setPath(const char *);
  const char * getPath();
  void setLocked(bool);
  bool getLocked();
  bool getSlideShow();
  void setSlideShow(bool);
  int getSlideShowTime();
  void setSlideShowTime(int);
  void reload();

 private:
  std::unique_ptr<nvs::NVSHandle> handle;
  template<typename T>
  T get_item_or_default(const char *key, T value);
  void get_string_or_default(const char *item, const char* default_value, char *out, size_t out_len);

  std::mutex mutex;
  char path[255];
  bool locked;
  bool slideshow;
  int slideshow_time;
};