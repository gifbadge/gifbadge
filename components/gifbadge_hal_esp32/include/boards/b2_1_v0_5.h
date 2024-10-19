#pragma once
#include <hal/board.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_st7701s.h"
#include "drivers/backlight_ledc.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/keys_esp_io_expander.h"
#include "driver/sdmmc_host.h"
#include "drivers/config_nvs.h"

#include "esp32s3_sdmmc.h"
#include "drivers/PmicNpm1300.h"
#include "drivers/keys_generic.h"

namespace Boards {
class b2_1_v0_5 : public Boards::esp32s3_sdmmc {
 public:
  b2_1_v0_5();
  ~b2_1_v0_5() override = default;

  Battery *getBattery() override;
  Touch *getTouch() override;
  Keys *getKeys() override;
  Display *getDisplay() override;
  Backlight *getBacklight() override;

  void powerOff() override;
  BOARD_POWER powerState() override;
  bool storageReady() override;
  void *turboBuffer() override { return buffer; };
  const char * name() override;
  void lateInit() override;
  WAKEUP_SOURCE bootReason() override;
  Vbus *getVbus() override;
  Charger *getCharger() override;

 protected:
  I2C *_i2c;
  KeysGeneric *_keys;
  display_st7701s *_display;
  backlight_ledc *_backlight;
  touch_ft5x06 *_touch;
  bool _usbConnected = false;
  PmicNpm1300 *_pmic;

  void *buffer;
  battery_max17048 *_battery;
  esp_io_expander_handle_t _io_expander = nullptr;
  Gpio *_card_detect = nullptr;
  PmicNpm1300Led *_vbus_led = nullptr;
  PmicNpm1300Led *_charge_led = nullptr;
  static void VbusCallback(bool state);
};
}