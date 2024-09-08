#include "boards/b2_1_v0_5.h"

#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include "esp_io_expander.h"
#include "esp_io_expander_cat9532.h"
#include "drivers/keys_esp_io_expander.h"
#include "drivers/esp_io_expander_gpio.h"

#define USB_ENABLE

static const char *TAG = "board_2_1_v0_5";

static bool checkSdState(Gpio *gpio) {
  return !gpio->read();
}

static bool sdState;

//static void checkSDTimer(void *arg) {
//  auto io_expander = (esp_io_expander_handle_t) arg;
//  if (checkSdState(io_expander) != sdState) {
//    esp_restart();
//  }
//}

namespace Boards {

b2_1_v0_5::b2_1_v0_5() {
  buffer = heap_caps_malloc(480 * 480 + 0x6100, MALLOC_CAP_INTERNAL);
  _config = new Config_NVS();
  _i2c = new I2C(I2C_NUM_0, 47, 48, 100 * 1000, false);
  _battery = new battery_max17048(_i2c, GPIO_NUM_0);
  /*G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2 */
  std::array<int, 16> rgb = {11, 12, 13, 3, 4, 5, 6, 7, 14, 15, 16, 17, 18, 8, 9, 10};
  _pmic = new mfd_npm1300(_i2c, GPIO_NUM_21);
  _pmic->buck1_set(3300);



  _backlight = new backlight_ledc(GPIO_NUM_0, true, 100);
//  _backlight->setLevel(_config->getBacklight() * 10);
  _touch = new touch_ft5x06(_i2c);

  Gpio *_cs = _pmic->gpio_get(1);
  _cs->config(Gpio::gpio_direction::OUT, Gpio::gpio_pull_mode::NONE);
  _cs->write(true);

  esp_io_expander_handle_t _io_expander = nullptr;

  esp_io_expander_new_gpio(_cs, &_io_expander);

  spi_line_config_t line_config = {
      .cs_io_type = IO_TYPE_EXPANDER,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
      .cs_gpio_num = 1,
      .scl_io_type = IO_TYPE_GPIO,
      .scl_gpio_num = 3,
      .sda_io_type = IO_TYPE_GPIO,
      .sda_gpio_num = 4,
      .io_expander = _io_expander,                        // Set to NULL if not using IO expander
  };


  mfd_npm1300_gpio *up = _pmic->gpio_get(4);
  up->config(Gpio::gpio_direction::IN, Gpio::gpio_pull_mode::UP);

  mfd_npm1300_gpio *down = _pmic->gpio_get(3);
  down->config(Gpio::gpio_direction::IN, Gpio::gpio_pull_mode::UP);

  mfd_npm1300_shphld *enter = _pmic->shphld_get();

  _keys = new KeysGeneric(up, down, enter);

    _pmic->loadsw1_enable();
    _card_detect = _pmic->gpio_get(0);
    _card_detect->config(Gpio::gpio_direction::IN, Gpio::gpio_pull_mode::UP);

  if (checkSdState(_card_detect)) {
    mount(GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_44, GPIO_NUM_42, GPIO_NUM_NC, 4);
  }

  //TODO: Check if we can use DFS with the RGB LCD
  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 80, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);

  _display = new display_st7701s(line_config, 2, 1, 45, 46, rgb);



}

Battery *b2_1_v0_5::getBattery() {
  return _battery;
}

Touch *b2_1_v0_5::getTouch() {
  return _touch;
}

Keys *b2_1_v0_5::getKeys() {
  return _keys;
}

Display *b2_1_v0_5::getDisplay() {
  return _display;
}

Backlight *b2_1_v0_5::getBacklight() {
  return _backlight;
}

void b2_1_v0_5::powerOff() {
//  LOGI(TAG, "Poweroff");
//  vTaskDelay(100 / portTICK_PERIOD_MS);
//  esp_io_expander_set_level(_io_expander, IO_EXPANDER_PIN_NUM_3, 1);

}

BOARD_POWER b2_1_v0_5::powerState() {
//  if (powerConnected() != CHARGE_NONE) {
//    return BOARD_POWER_NORMAL;
//  }
//  if (_battery->getSoc() < 12) {
//    if (_battery->getSoc() < 10) {
//      return BOARD_POWER_CRITICAL;
//    }
//    return BOARD_POWER_LOW;
//
//  }
  return BOARD_POWER_NORMAL;
}

bool b2_1_v0_5::storageReady() {
  return checkSdState(_card_detect);
}

CHARGE_POWER b2_1_v0_5::powerConnected() {
//  if(gpio_get_level(GPIO_NUM_0)){
//    const std::lock_guard<std::mutex> lock(_i2c->i2c_lock);
//    uint32_t levels;
//    if(esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK){
//      if(levels  & (1 << 8)){
//        return CHARGE_LOW;
//      }
//      else {
//        return CHARGE_HIGH;
//      }
//    }
//    else {
//      return CHARGE_LOW;
//    }
//  }
  return CHARGE_NONE;
}
const char *b2_1_v0_5::name() {
  return "2.1\" 0.5";
}
}