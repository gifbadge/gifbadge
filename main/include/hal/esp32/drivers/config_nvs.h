#pragma once
#include <memory>
#include <nvs_handle.hpp>
#include "hal/config.h"


class Config_NVS: public Config {
 public:
  Config_NVS();
  ~Config_NVS() override = default;

  void setPath(const char *) override;
  void getPath(char *outPath) override;
  void setLocked(bool) override;
  bool getLocked() override;
  bool getSlideShow() override;
  void setSlideShow(bool) override;
  int getSlideShowTime() override;
  void setSlideShowTime(int) override;
  int getBacklight() override;
  void setBacklight(int) override;
  void reload() override;
  void save() override;


 private:
  std::unique_ptr<nvs::NVSHandle> handle;
  template<typename T>
  T get_item_or_default(const char *key, T value);
  void get_string_or_default(const char *item, const char* default_value, char *out, size_t out_len);
};