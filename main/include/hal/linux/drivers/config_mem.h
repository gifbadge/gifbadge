#pragma once
#include <memory>
#include "hal/config.h"


class Config_Mem: public Config {
 public:
  Config_Mem();
  ~Config_Mem() override = default;

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
  bool _locked = false;
  bool _slideshow = false;
  int _slideshow_time = 30;
  int _backlight = 10;
  char _path[128] = "";

};