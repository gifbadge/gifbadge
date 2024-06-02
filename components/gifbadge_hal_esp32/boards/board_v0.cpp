#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <wear_levelling.h>

#include "hal/board.h"

#include "boards/v0.h"
#include "drivers/backlight_ledc.h"
#include "hal_usb.h"
#include "tusb_msc_storage.h"

static const char *TAG = "board_v0";

namespace Boards {

v0::v0() {
  _i2c = new I2C(I2C_NUM_0, 21, 18);
  _battery = new battery_analog(ADC_CHANNEL_9);
  _keys = new keys_gpio(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_0);
  _display = new display_gc9a01(35, 36, 34, 37, 46);
  _backlight = new backlight_ledc(GPIO_NUM_45, 0);

  static wl_handle_t wl_handle = WL_INVALID_HANDLE;
  ESP_ERROR_CHECK(init_ext_flash(39, 41, 40, 42, &wl_handle));
  usb_init_wearlevel(&wl_handle);

  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 240, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);
}

Battery *v0::getBattery() {
  return _battery;
}

Touch *v0::getTouch() {
  return nullptr;
}

Keys *v0::getKeys() {
  return _keys;
}

Display *v0::getDisplay() {
  return _display;
}

Backlight *v0::getBacklight() {
  return _backlight;
}

void v0::powerOff() {
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(0));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(0));
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(0), 0);
  esp_deep_sleep_start();
}

BOARD_POWER v0::powerState() {
  return BOARD_POWER_NORMAL;
}

StorageInfo v0::storageInfo() {
  return StorageInfo();
}

const char *v0::name() {
  return "1.28\" 0.0";
}

bool v0::powerConnected() {
  // No provisions in design to monitor this
  return false;
}

bool v0::usbConnected() {
  return tinyusb_msc_storage_in_use_by_usb_host();
}
}