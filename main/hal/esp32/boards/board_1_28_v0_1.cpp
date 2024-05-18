#include <esp_pm.h>
#include "log.h"
#include <driver/sdmmc_defs.h>
#include <esp_vfs_fat.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include "hal/esp32/boards/board_1_28_v0_1.h"
#include "hal/esp32/hal_usb.h"
#include "hal/esp32/drivers/display_gc9a01.h"
#include "driver/gpio.h"
#include "hal/esp32/drivers/config_nvs.h"

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

board_1_28_v0_1::board_1_28_v0_1() {
  buffer = heap_caps_malloc(240*240+0x6100, MALLOC_CAP_INTERNAL);
  _config = new Config_NVS();
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
//    if (init_sdmmc_slot(GPIO_NUM_40,
//                        GPIO_NUM_39,
//                        GPIO_NUM_41,
//                        GPIO_NUM_42,
//                        GPIO_NUM_33,
//                        GPIO_NUM_47,
//                        GPIO_CARD_DETECT,
//                        &card,
//                        1) == ESP_OK) {
//      usb_init_mmc(GPIO_NUM_NC, &card);
//    }
            mount_sdmmc_slot(GPIO_NUM_40, GPIO_NUM_39, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_33, GPIO_NUM_47, GPIO_CARD_DETECT,
                             &card,
                             1);
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

  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "Board Lock", &pmLockHandle);
}

Battery * board_1_28_v0_1::getBattery() {
  return _battery;
}

Touch * board_1_28_v0_1::getTouch() {
  return _touch;
}

I2C * board_1_28_v0_1::getI2c() {
  return _i2c;
}

Keys * board_1_28_v0_1::getKeys() {
  return _keys;
}

Display * board_1_28_v0_1::getDisplay() {
  return _display;
}

Backlight * board_1_28_v0_1::getBacklight() {
  return _backlight;
}

void board_1_28_v0_1::powerOff() {
  LOGI(TAG, "Poweroff");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_SHUTDOWN, 1);
  gpio_hold_en(GPIO_SHUTDOWN);
}

BOARD_POWER board_1_28_v0_1::powerState() {
  //TODO Detect USB power status, implement critical level
  if (powerConnected()) {
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

bool board_1_28_v0_1::storageReady() {
  if (!gpio_get_level(GPIO_CARD_DETECT)) {
    return true;
  }
  return false;
}

StorageInfo board_1_28_v0_1::storageInfo() {
  StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
  double speed = card->real_freq_khz / 1000.00;
  uint64_t total_bytes;
  uint64_t free_bytes;
  esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
  return {card->cid.name, type, speed, total_bytes, free_bytes};
}

void board_1_28_v0_1::pmLock() {
  esp_pm_lock_acquire(pmLockHandle);
}

void board_1_28_v0_1::pmRelease() {
  esp_pm_lock_release(pmLockHandle);
}

int board_1_28_v0_1::StorageFormat() {
  LOGI(TAG, "Format Start");
  esp_err_t ret;
  esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = 30 * 1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
      .trigger_panic = false,
  };
#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0
  wdtConfig.idle_core_mask |= (1 << 0);
#endif
#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1
  wdtConfig.idle_core_mask |= (1 << 1);
#endif
  esp_task_wdt_reconfigure(&wdtConfig);
  ret = esp_vfs_fat_sdcard_format("/data", card);
  wdtConfig.timeout_ms = CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000;
  esp_task_wdt_reconfigure(&wdtConfig);
  LOGI(TAG, "Format Done");
  return ret;
}

const char * board_1_28_v0_1::name() {
  return "1.28\" 0.1-0.2";
}

bool board_1_28_v0_1::powerConnected() {
  return gpio_get_level(GPIO_VBUS_DETECT);
}
Config *board_1_28_v0_1::getConfig() {
  return _config;
}
void board_1_28_v0_1::debugInfo() {

}
