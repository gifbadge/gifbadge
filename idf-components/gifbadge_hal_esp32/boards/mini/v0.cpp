#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <wear_levelling.h>

#include "hal/board.h"

#include "boards/mini/v0.h"
#include "drivers/backlight_ledc.h"
#include "hal_usb.h"
#include "tusb_msc_storage.h"

static const char *TAG = "board_v0";

namespace Boards {

esp32::s3::mini::v0::v0() {
  _i2c = new I2C(I2C_NUM_0, 21, 18, 100 * 1000, false);
  _battery = new hal::battery::esp32s3::battery_analog(ADC_CHANNEL_9);
  gpio_install_isr_service(0);
  _keys = new hal::keys::esp32s3::keys_gpio(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_0);
  _display = new hal::display::esp32s3::display_gc9a01(35, 36, 34, 37, 46);
  _backlight = new hal::backlight::esp32s3::backlight_ledc(GPIO_NUM_45, false, 0);

  static wl_handle_t wl_handle = WL_INVALID_HANDLE;
  ESP_ERROR_CHECK(init_ext_flash(39, 41, 40, 42, &wl_handle));
  usb_init_wearlevel(&wl_handle);

  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 240, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);
}

hal::battery::Battery *esp32::s3::mini::v0::GetBattery() {
  return _battery;
}

hal::touch::Touch *esp32::s3::mini::v0::GetTouch() {
  return nullptr;
}

hal::keys::Keys *esp32::s3::mini::v0::GetKeys() {
  return _keys;
}

hal::display::Display *esp32::s3::mini::v0::GetDisplay() {
  return _display;
}

hal::backlight::Backlight *esp32::s3::mini::v0::GetBacklight() {
  return _backlight;
}

void esp32::s3::mini::v0::PowerOff() {
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(0));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(0));
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(0), 0);
  esp_deep_sleep_start();
}

BoardPower esp32::s3::mini::v0::PowerState() {
  return BOARD_POWER_NORMAL;
}

StorageInfo esp32::s3::mini::v0::GetStorageInfo() {
  return GetStorageInfo();
}

const char *esp32::s3::mini::v0::Name() {
  return "1.28\" 0.0";
}

bool esp32::s3::mini::v0::UsbConnected() {
  return tinyusb_msc_storage_in_use_by_usb_host();
}
void esp32::s3::mini::v0::LateInit() {

}

int esp32::s3::mini::v0::UsbCallBack(tusb_msc_callback_t callback) {
  tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED, callback);
  return 0;
}

}