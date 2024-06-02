#include <esp_vfs_fat.h>
#include <driver/sdmmc_defs.h>
#include <esp_task_wdt.h>
#include "boards/board_esp32s3_sdmmc.h"
#include "hal_usb.h"
#include "log.h"

static const char *TAG = "board_esp32s3_sdmmc";

StorageInfo board_esp32s3_sdmmc::storageInfo() {
  StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
  double speed = card->real_freq_khz / 1000.00;
  uint64_t total_bytes;
  uint64_t free_bytes;
  esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
  return {card->cid.name, type, speed, total_bytes, free_bytes};
}

int board_esp32s3_sdmmc::StorageFormat() {
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
esp_err_t board_esp32s3_sdmmc::mount(gpio_num_t clk,
                                     gpio_num_t cmd,
                                     gpio_num_t d0,
                                     gpio_num_t d1,
                                     gpio_num_t d2,
                                     gpio_num_t d3,
                                     gpio_num_t cd,
                                     int width) {
#ifdef USB_ENABLE
  if (checkSdState(_io_expander)) {
    if (init_sdmmc_slot(clk,
                   cmd,
                   d0,
                   d1,
                   d2,
                   d3,
                   cd,
                   &card,
                   width) == ESP_OK) {
      usb_init_mmc(0, &card);
    } else {
      return ESP_FAIL;
    }
  }
#else
  return mount_sdmmc_slot(clk,
                          cmd,
                          d0,
                          d1,
                          d2,
                          d3,
                          cd,
                          &card,
                          width);
#endif
}

bool board_esp32s3_sdmmc::usbConnected() {
#ifdef USB_ENABLE
  return tinyusb_msc_storage_in_use_by_usb_host();
#else
  return false;
#endif
}
