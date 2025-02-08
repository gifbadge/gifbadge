#include <cstring>
#include <esp_vfs_fat.h>
#include <driver/sdmmc_defs.h>
#include <esp_task_wdt.h>
#include "boards/esp32s3_sdmmc.h"
#include "hal_usb.h"
#include "log.h"
#include "tusb_msc_storage.h"

static const char *TAG = "board_esp32s3_sdmmc";

namespace Boards {

StorageInfo esp32::s3::esp32s3_sdmmc::GetStorageInfo() {
  StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
  double speed = card->real_freq_khz / 1000.00;
  uint64_t total_bytes;
  uint64_t free_bytes;
  esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
  return {card->cid.name, type, speed, total_bytes, free_bytes};
}

void esp32::s3::esp32s3_sdmmc::Reset() {
  if(tinyusb_msc_storage_unmount() != ESP_OK){
    LOGI(TAG, "Failed to unmount");
  }
  esp32s3::Reset();
}

void esp32::s3::esp32s3_sdmmc::PowerOff() {
  if(tinyusb_msc_storage_unmount() != ESP_OK){
    LOGI(TAG, "Failed to unmount");
  }
}

int esp32::s3::esp32s3_sdmmc::StorageFormat() {
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
esp_err_t esp32::s3::esp32s3_sdmmc::mount(gpio_num_t clk,
                                          gpio_num_t cmd,
                                          gpio_num_t d0,
                                          gpio_num_t d1,
                                          gpio_num_t d2,
                                          gpio_num_t d3,
                                          gpio_num_t cd,
                                          int width,
                                          gpio_num_t usb_sense) {
#ifndef USB_DISABLED
  if (init_sdmmc_slot(clk,
                      cmd,
                      d0,
                      d1,
                      d2,
                      d3,
                      cd,
                      &card,
                      width) == ESP_OK) {
    usb_init_mmc(usb_sense, &card);
    storageAvailable = true;
    char str[12];
    /* Get volume label of the default drive */
    f_getlabel("", str, 0);
    LOGI(TAG, "Volume Name: %s", str);
    if (strlen(str) == 0) {
      f_setlabel("GifBadge");
    }
    return ESP_OK;
  } else {
    return ESP_FAIL;
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

bool esp32::s3::esp32s3_sdmmc::UsbConnected() {
  if(!storageAvailable){
    return false;
  }
#ifndef USB_DISABLED
  return tinyusb_msc_storage_in_use_by_usb_host();
#else
  return false;
#endif
}
int esp32::s3::esp32s3_sdmmc::UsbCallBack(tusb_msc_callback_t callback) {
  tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED, callback);
  return 0;
}
}