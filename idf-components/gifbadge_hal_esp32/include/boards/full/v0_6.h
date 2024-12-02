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
  char * SerialNumber() override;

  v0_6();
  ~v0_6() override = default;

  hal::battery::Battery *GetBattery() override;
  hal::touch::Touch *GetTouch() override;
  hal::keys::Keys *GetKeys() override;
  hal::display::Display *GetDisplay() override;
  hal::backlight::Backlight *GetBacklight() override;

  void PowerOff() override;
  BoardPower PowerState() override;
  bool StorageReady() override;
  void *TurboBuffer() override { return buffer; };
  const char *Name() override;
  void LateInit() override;
  WakeupSource BootReason() override;
  hal::vbus::Vbus *GetVbus() override;
  hal::charger::Charger *GetCharger() override;
  void DebugInfo() override;

 protected:
  I2C *_i2c;
  hal::keys::esp32s3::KeysGeneric *_keys;
  hal::display::esp32s3::display_st7701s *_display;
  hal::backlight::esp32s3::backlight_ledc *_backlight;
  hal::touch::esp32s3::touch_ft5x06 *_touch;
  bool _usbConnected = false;
  hal::pmic::esp32s3::PmicNpm1300 *_pmic;

  void *buffer;
  esp_io_expander_handle_t _io_expander = nullptr;
  hal::gpio::Gpio *_cs = nullptr;
  hal::gpio::Gpio *_card_detect = nullptr;
  hal::pmic::esp32s3::PmicNpm1300Led *_vbus_led = nullptr;
  hal::pmic::esp32s3::PmicNpm1300Led *_charge_led = nullptr;
  static void VbusCallback(bool state);
};
}