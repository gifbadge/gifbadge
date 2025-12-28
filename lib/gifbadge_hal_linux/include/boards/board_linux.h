#pragma once

#pragma once
#include "hal/board.h"
#include "drivers/backlight_dummy.h"
#include "drivers/battery_dummy.h"
#include "drivers/display_sdl.h"
#include "drivers/config_mem.h"
#include "drivers/key_sdl.h"
#include <hal/backlight.h>
#include "hal/battery.h"
#include "hal/display.h"
#include "hal/keys.h"
#include "hal/touch.h"

class board_linux : public Boards::Board {
 public:
  hal::charger::Charger * GetCharger() override;
  void BootInfo() override;
  bool OtaCheck() override;
  Boards::OtaError OtaValidate() override;
  void OtaInstall() override;
  int OtaStatus() override;
  Boards::WakeupSource BootReason() override;

  void LateInit() override;
  void Reset() override;
  const char * SwVersion() override;
  char * SerialNumber() override;

  board_linux();
  ~board_linux() override = default;

  hal::battery::Battery * GetBattery() override;
  hal::touch::Touch * GetTouch() override;
  hal::keys::Keys * GetKeys() override;
  hal::display::Display * GetDisplay() override;
  hal::backlight::Backlight * GetBacklight() override;
  hal::vbus::Vbus *GetVbus() override;

  void PowerOff() override;
  void PmLock() override {};
  void PmRelease() override {};
  Boards::BoardPower PowerState() override;
  bool StorageReady() override;
  StorageInfo GetStorageInfo() override;
  int StorageFormat() override { return 0; };
  const char * Name() override;
  void * TurboBuffer() override {return _buffer;};
  hal::config::Config *GetConfig() override;
  void DebugInfo() override;
  bool UsbConnected() override;

 private:
  hal::backlight::oslinux::backlight_dummy *_backlight;
  hal::battery::oslinux::battery_dummy *_battery;
  hal::display::oslinux::display_sdl *_display;
  hal::config::oslinux::Config_Mem *_config;
  hal::keys::oslinux::keys_sdl *_keys;
  void *_buffer = nullptr;

};