#pragma once
#include "hal/board.h"
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_st7701s.h"
#include "drivers/backlight_ledc.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/display_gc9a01.h"
#include "driver/sdmmc_host.h"
#include "esp_pm.h"
#include "drivers/config_nvs.h"
#include "soc/gpio_num.h"
#include "boards/esp32s3_sdmmc.h"
#include "drivers/vbus_gpio.h"

namespace Boards {
namespace esp32::mini {
class v0_3 : public Boards::esp32::s3::esp32s3_sdmmc {
 public:
  v0_3();
  ~v0_3() override = default;

  Battery *getBattery() override;
  Touch *getTouch() override;
  Keys *getKeys() override;
  Display *getDisplay() override;
  Backlight *getBacklight() override;

  void powerOff() override;

  BOARD_POWER powerState() override;
  bool storageReady() override;
  const char *name() override;
  void *turboBuffer() override { return buffer; }
  void lateInit() override;
  WAKEUP_SOURCE bootReason() override;;

 private:
  battery_max17048 *_battery;
  I2C *_i2c;
  keys_gpio *_keys;
  display_gc9a01 *_display;
  backlight_ledc *_backlight;
  void *buffer;
  VbusGpio *_vbus;
};
}
}

