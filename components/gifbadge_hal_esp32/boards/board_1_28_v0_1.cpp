#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include "boards/b1_28_v0_1.h"
#include "drivers/display_gc9a01.h"
#include "driver/gpio.h"
#include "drivers/config_nvs.h"

static const char *TAG = "board_1_28_v0_1";

esp_pm_lock_handle_t usb_pm;

static void IRAM_ATTR sdcard_removed(void *) {
  esp_restart();
}

static void IRAM_ATTR usb_connected(void *) {
  if (gpio_get_level(GPIO_VBUS_DETECT)) {
    esp_pm_lock_acquire(usb_pm);
    esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_SRP_BVALID_IN_IDX, false);
  } else {
    esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_SRP_BVALID_IN_IDX, false);
    esp_pm_lock_release(usb_pm);
  }
}

namespace Boards {

b1_28_v0_1::b1_28_v0_1() {
  buffer = heap_caps_malloc(240 * 240 + 0x6100, MALLOC_CAP_INTERNAL);
  _i2c = new I2C(I2C_NUM_0, 17, 18);
  _battery = new battery_max17048(_i2c, GPIO_VBUS_DETECT);
  _battery->inserted(); //Battery not removable. So set this
  _keys = new keys_gpio(GPIO_NUM_0, GPIO_NUM_2, GPIO_NUM_1);
  _display = new display_gc9a01(35, 36, 34, 37, 38);
  _backlight = new backlight_ledc(GPIO_NUM_9, 0);
  _backlight->setLevel(100);

  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 40, .light_sleep_enable = true};
  esp_pm_configure(&pm_config);

  esp_sleep_enable_gpio_wakeup();

  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "USB", &usb_pm);

  gpio_pullup_en(GPIO_CARD_DETECT);
  if (!gpio_get_level(GPIO_CARD_DETECT)) {
    mount(GPIO_NUM_40, GPIO_NUM_39, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_33, GPIO_NUM_47, GPIO_CARD_DETECT, 1);
  }
  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_CARD_DETECT, sdcard_removed, nullptr);
  gpio_set_intr_type(GPIO_CARD_DETECT, GPIO_INTR_ANYEDGE);

  gpio_config_t vbus_config = {};

  vbus_config.intr_type = GPIO_INTR_ANYEDGE;
  vbus_config.mode = GPIO_MODE_INPUT;
  vbus_config.pin_bit_mask = (1ULL << GPIO_VBUS_DETECT);
  vbus_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  vbus_config.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&vbus_config);

  gpio_isr_handler_add(GPIO_VBUS_DETECT, usb_connected, nullptr);
  usb_connected(nullptr); //Trigger usb detection

  //Shutdown pin
  gpio_config_t io_conf = {};

  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_SHUTDOWN);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_set_drive_capability(GPIO_SHUTDOWN, GPIO_DRIVE_CAP_MAX);
}

Battery *b1_28_v0_1::getBattery() {
  return _battery;
}

Touch *b1_28_v0_1::getTouch() {
  return nullptr;
}

Keys *b1_28_v0_1::getKeys() {
  return _keys;
}

Display *b1_28_v0_1::getDisplay() {
  return _display;
}

Backlight *b1_28_v0_1::getBacklight() {
  return _backlight;
}

void b1_28_v0_1::powerOff() {
  LOGI(TAG, "Poweroff");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_SHUTDOWN, 1);
  gpio_hold_en(GPIO_SHUTDOWN);
}

BOARD_POWER b1_28_v0_1::powerState() {
  //TODO Detect USB power status, implement critical level
  if (powerConnected() != CHARGE_NONE) {
    return BOARD_POWER_NORMAL;
  }
  if (_battery->getSoc() < 12) {
    if (_battery->getSoc() < 10) {
      return BOARD_POWER_CRITICAL;
    }
    return BOARD_POWER_LOW;

  }
  return BOARD_POWER_NORMAL;
}

bool b1_28_v0_1::storageReady() {
  if (!gpio_get_level(GPIO_CARD_DETECT)) {
    return true;
  }
  return false;
}

const char *b1_28_v0_1::name() {
  return "1.28\" 0.1-0.2";
}

CHARGE_POWER b1_28_v0_1::powerConnected() {
  if(gpio_get_level(GPIO_VBUS_DETECT)){
    return CHARGE_LOW;
  }
  return CHARGE_NONE;
}
}
