#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "boards/b1_28_v0_3.h"
#include "drivers/display_gc9a01.h"
#include "driver/gpio.h"
#include "drivers/config_nvs.h"

static const char *TAG = "board_1_28_v0_3";

static esp_pm_lock_handle_t usb_pm;

#define GPIO_CARD_DETECT GPIO_NUM_39
#define GPIO_VBUS_DETECT GPIO_NUM_9
#define GPIO_EXT_PWR GPIO_NUM_40
#define GPIO_KEY_UP GPIO_NUM_0
#define GPIO_KEY_DOWN GPIO_NUM_8
#define GPIO_KEY_ENTER GPIO_NUM_5

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

b1_28_v0_3::b1_28_v0_3() {
//  Power pin
  gpio_config_t io_conf = {};

  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_EXT_PWR);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_set_drive_capability(GPIO_EXT_PWR, GPIO_DRIVE_CAP_DEFAULT);
  gpio_set_level(GPIO_EXT_PWR, 0);
  gpio_hold_en(GPIO_EXT_PWR);

  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 40, .light_sleep_enable = true};
  esp_pm_configure(&pm_config);
  esp_sleep_enable_gpio_wakeup();
  _vbus = new VbusGpio(GPIO_VBUS_DETECT);
  }

Battery *b1_28_v0_3::getBattery() {
  return _battery;
}

Touch *b1_28_v0_3::getTouch() {
  return nullptr;
}

Keys *b1_28_v0_3::getKeys() {
  return _keys;
}

Display *b1_28_v0_3::getDisplay() {
  return _display;
}

Backlight *b1_28_v0_3::getBacklight() {
  return _backlight;
}

void b1_28_v0_3::powerOff() {
  LOGI(TAG, "Poweroff");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_hold_dis(GPIO_EXT_PWR);
  gpio_set_level(GPIO_EXT_PWR, 1);
  rtc_gpio_pullup_en(GPIO_KEY_ENTER);
  rtc_gpio_pulldown_dis(GPIO_KEY_ENTER);
  esp_sleep_enable_ext0_wakeup(GPIO_KEY_ENTER, 0);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  esp_deep_sleep_start();
}

BOARD_POWER b1_28_v0_3::powerState() {
  if (_vbus->VbusConnected()) {
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

bool b1_28_v0_3::storageReady() {
  if (!gpio_get_level(GPIO_CARD_DETECT)) {
    return true;
  }
  return false;
}

const char *b1_28_v0_3::name() {
  return "1.28\" 0.3";
}

void b1_28_v0_3::lateInit() {
  buffer = heap_caps_malloc(240 * 240 + 0x6100, MALLOC_CAP_INTERNAL);
  _i2c = new I2C(I2C_NUM_0, 6, 7, 100 * 1000, true);
  _battery = new battery_max17048(_i2c, GPIO_VBUS_DETECT);
  _battery->inserted(); //Battery not removable. So set this

  gpio_install_isr_service(0);
  _keys = new keys_gpio(GPIO_KEY_UP, GPIO_KEY_DOWN, GPIO_KEY_ENTER);

  _display = new display_gc9a01(18, 17, 16, 15, 21);
  _backlight = new backlight_ledc(GPIO_NUM_10, false, 0);

  gpio_pullup_en(GPIO_CARD_DETECT);
  if (!gpio_get_level(GPIO_CARD_DETECT)) {
    mount(GPIO_NUM_33, GPIO_NUM_36, GPIO_NUM_35, GPIO_NUM_34, GPIO_NUM_37, GPIO_NUM_38, GPIO_CARD_DETECT, 4, GPIO_NUM_0);
  }
  gpio_isr_handler_add(GPIO_CARD_DETECT, sdcard_removed, nullptr);
  gpio_set_intr_type(GPIO_CARD_DETECT, GPIO_INTR_ANYEDGE);

  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "USB", &usb_pm);

  gpio_config_t vbus_config = {};

  vbus_config.intr_type = GPIO_INTR_ANYEDGE;
  vbus_config.mode = GPIO_MODE_INPUT;
  vbus_config.pin_bit_mask = (1ULL << GPIO_VBUS_DETECT);
  vbus_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  vbus_config.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&vbus_config);

  gpio_isr_handler_add(GPIO_VBUS_DETECT, usb_connected, nullptr);
  usb_connected(nullptr); //Trigger usb detection
}
Board::WAKEUP_SOURCE b1_28_v0_3::bootReason() {
  if (esp_reset_reason() != ESP_RST_POWERON) {
    return Board::WAKEUP_SOURCE::KEY;
  }
  return Board::WAKEUP_SOURCE::NONE;
}
}
