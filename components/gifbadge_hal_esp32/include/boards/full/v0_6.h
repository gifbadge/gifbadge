#pragma once
#include "hal/board.h"
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

#include "boards/esp32s3_sdmmc.h"
#include "drivers/PmicNpm1300.h"
#include "drivers/keys_generic.h"

namespace Boards::esp32::s3::full {
class v0_6 : public Boards::esp32::s3::esp32s3_sdmmc {
 public:
  v0_6();
  ~v0_6() override = default;

  Battery *GetBattery() override;
  Touch *GetTouch() override;
  Keys *GetKeys() override;
  Display *GetDisplay() override;
  Backlight *GetBacklight() override;

  void PowerOff() override;
  BoardPower PowerState() override;
  bool StorageReady() override;
  void *TurboBuffer() override { return buffer; };
  const char *Name() override;
  void LateInit() override;
  WAKEUP_SOURCE BootReason() override;
  Vbus *GetVbus() override;
  Charger *GetCharger() override;
  void DebugInfo() override;

 protected:
  I2C *_i2c;
  KeysGeneric *_keys;
  display_st7701s *_display;
  backlight_ledc *_backlight;
  touch_ft5x06 *_touch;
  bool _usbConnected = false;
  PmicNpm1300 *_pmic;

  void *buffer;
  esp_io_expander_handle_t _io_expander = nullptr;
  Gpio *_card_detect = nullptr;
  PmicNpm1300Led *_vbus_led = nullptr;
  PmicNpm1300Led *_charge_led = nullptr;
  static void VbusCallback(bool state);
};
}