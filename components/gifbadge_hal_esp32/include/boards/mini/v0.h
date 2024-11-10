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
#include "i2c.h"
#include "boards/boards_esp32s3.h"

namespace Boards::esp32::s3::mini {
class v0 : public esp32s3 {
 public:
  v0();
  ~v0() override = default;

  Battery *GetBattery() override;
  Touch *GetTouch() override;
  Keys *GetKeys() override;
  Display *GetDisplay() override;
  Backlight *GetBacklight() override;

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
  battery_analog *_battery;
  I2C *_i2c;
  keys_gpio *_keys;
  display_gc9a01 *_display;
  Backlight *_backlight;
  int UsbCallBack(tusb_msc_callback_t callback);
};
}