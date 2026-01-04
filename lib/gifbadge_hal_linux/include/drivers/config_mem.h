/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <memory>
#include "hal/config.h"

namespace hal::config::oslinux {
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
  char _path[128] = "/data";

};
}
