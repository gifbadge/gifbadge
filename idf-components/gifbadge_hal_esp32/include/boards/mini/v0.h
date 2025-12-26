#pragma once

#include "ff.h"
#include "hal/board.h"
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_gc9a01.h"
#include "drivers/config_nvs.h"
#include "boards/boards_esp32s3.h"

namespace Boards::esp32::s3::mini {
class v0 : public esp32s3 {
 public:
  v0();
  ~v0() override = default;

  hal::battery::Battery *GetBattery() override;
  hal::touch::Touch *GetTouch() override;
  hal::keys::Keys *GetKeys() override;
  hal::display::Display *GetDisplay() override;
  hal::backlight::Backlight *GetBacklight() override;

  void PowerOff() override;
  BoardPower PowerState() override;
  bool StorageReady() override { return true; };
  StorageInfo GetStorageInfo() override;
  int StorageFormat() override { return ESP_OK; };
  const char *Name() override;
  void *TurboBuffer() override { return nullptr; };
  bool UsbConnected() override;
  void LateInit() override;

 private:
  hal::battery::esp32s3::battery_analog *_battery;
  hal::keys::esp32s3::keys_gpio *_keys;
  hal::display::esp32s3::display_gc9a01 *_display;
  hal::backlight::Backlight *_backlight;
  int UsbCallBack(tusb_msc_callback_t callback);
};
}