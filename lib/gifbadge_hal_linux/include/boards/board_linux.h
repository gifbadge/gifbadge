#pragma once

#pragma once
#include "hal/board.h"
#include "drivers/backlight_dummy.h"
#include "drivers/battery_dummy.h"
#include "drivers/display_sdl.h"
#include "drivers/config_mem.h"
#include "drivers/key_sdl.h"
#include "backlight.h"
#include "battery.h"
#include "display.h"
#include "keys.h"
#include "touch.h"
#include "backlight_dummy.h"

class board_linux : public Board {
 public:
  board_linux();
  ~board_linux() override = default;

  hal::battery::Battery * getBattery() override;
  hal::touch::Touch * getTouch() override;
  hal::keys::Keys * getKeys() override;
  hal::display::Display * getDisplay() override;
  hal::backlight::Backlight * getBacklight() override;

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
  void debugInfo() override;
  bool usbConnected() override;

 private:
  hal::backlight::_linux::backlight_dummy *_backlight;
  battery_dummy *_battery;
  display_sdl *_display;
  Config_Mem *_config;
  keys_sdl *_keys;

};