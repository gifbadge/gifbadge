#pragma once

#pragma once
#include "hal/board.h"
#include "hal/linux/drivers/backlight_dummy.h"
#include "hal/linux/drivers/battery_dummy.h"
#include "hal/linux/drivers/display_sdl.h"
#include "hal/linux/drivers/config_mem.h"
#include "hal/linux/drivers/key_sdl.h"

class board_linux : public Board {
 public:
  board_linux();
  ~board_linux() override = default;

  Battery * getBattery() override;
  Touch * getTouch() override;
  Keys * getKeys() override;
  Display * getDisplay() override;
  Backlight * getBacklight() override;

  void powerOff() override;
  void pmLock() override {};
  void pmRelease() override {};
  BOARD_POWER powerState() override;
  bool storageReady() override;
  StorageInfo storageInfo() override;
  int StorageFormat() override { return 0; };
  const char * name() override;
  bool powerConnected() override;
  void * turboBuffer() override {return nullptr;};
  Config *getConfig() override;



 private:
  backlight_dummy *_backlight;
  battery_dummy *_battery;
  display_sdl *_display;
  Config_Mem *_config;
  keys_sdl *_keys;

};