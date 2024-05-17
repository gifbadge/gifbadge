#pragma once

#include <string>
#include <mutex>

#include <nvs_flash.h>
#include <nvs_handle.hpp>

class Config {
 public:
  Config() = default;
  virtual ~Config() = default;
  virtual void save() = 0;

  virtual void setPath(const char *) = 0;
  virtual void getPath(char *outPath) = 0;
  virtual void setLocked(bool) = 0;
  virtual bool getLocked() = 0;
  virtual bool getSlideShow() = 0;
  virtual void setSlideShow(bool) = 0;
  virtual int getSlideShowTime() = 0;
  virtual void setSlideShowTime(int) = 0;
  virtual int getBacklight() = 0;
  virtual void setBacklight(int) = 0;
  virtual void reload() = 0;

};