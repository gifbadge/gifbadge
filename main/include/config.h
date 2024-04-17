#pragma once

#include <string>
#include <mutex>

#include <nvs_flash.h>
#include <nvs_handle.hpp>
#include <filesystem>

class ImageConfig {
 public:
  ImageConfig();
  ~ImageConfig();
  void save();

  void setPath(const std::filesystem::path &);
  std::filesystem::path getPath();
  void setDirectory(const std::filesystem::path &);
  std::filesystem::path getDirectory();
  void setFile(const std::filesystem::path &);
  std::filesystem::path getFile();
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
  std::string get_string_or_default(const char *item, std::string value);

  std::mutex mutex;
  std::filesystem::path directory;
  std::filesystem::path image_file;
  std::filesystem::path path;
  bool locked;
  bool slideshow;
  int slideshow_time;
};