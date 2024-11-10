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

namespace hal::vbus::esp32s3 {
class b2_1_v0_2v0_4_vbus: public hal::vbus::Vbus {
 public:
  b2_1_v0_2v0_4_vbus(gpio_num_t gpio, esp_io_expander_handle_t expander, uint8_t expander_pin);
  uint16_t VbusMaxCurrentGet() override;
  void VbusMaxCurrentSet(uint16_t mA) override;
  bool VbusConnected() override;

 private:
  esp_io_expander_handle_t _io_expander = nullptr;
  gpio_num_t _gpio;
  uint8_t _expander_gpio;
};
}

namespace Boards::esp32::s3::full {

class v0_2v0_4 : public Boards::esp32::s3::esp32s3_sdmmc {
 public:
  v0_2v0_4();
  ~v0_2v0_4() override = default;

  hal::battery::Battery *GetBattery() override;
  hal::touch::Touch *GetTouch() override;
  hal::keys::Keys *GetKeys() override;
  hal::display::Display *GetDisplay() override;
  hal::backlight::Backlight *GetBacklight() override;

  void PowerOff() override;
  BoardPower PowerState() override;
  bool StorageReady() override;
  void *TurboBuffer() override { return buffer; }
  void LateInit() override;
  hal::vbus::Vbus *GetVbus() override;;

 protected:
  I2C *_i2c;
  hal::keys::esp32s3::keys_esp_io_expander *_keys;
  hal::display::esp32s3::display_st7701s *_display;
  hal::backlight::esp32s3::backlight_ledc *_backlight;
  hal::touch::esp32s3::touch_ft5x06 *_touch;
  hal::vbus::esp32s3::b2_1_v0_2v0_4_vbus *_vbus;
  bool _usbConnected = false;

  void *buffer;
  hal::battery::esp32s3::battery_max17048 *_battery;
  esp_io_expander_handle_t _io_expander = nullptr;
};
}