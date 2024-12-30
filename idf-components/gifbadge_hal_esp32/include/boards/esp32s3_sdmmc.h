#pragma once
#include "driver/sdmmc_host.h"
#include "boards_esp32s3.h"

namespace Boards::esp32::s3 {
 class esp32s3_sdmmc : public Boards::esp32::s3::esp32s3 {
  public:
   void PowerOff() override;

   esp32s3_sdmmc() = default;
  ~esp32s3_sdmmc() override = default;

  bool UsbConnected() override;
  StorageInfo GetStorageInfo() override;
  int StorageFormat() override;
  void Reset() override;
  int UsbCallBack(tusb_msc_callback_t callback) override;

 protected:
  bool storageAvailable = false;
  sdmmc_card_t *card = nullptr;
  esp_err_t mount(gpio_num_t clk,
                  gpio_num_t cmd,
                  gpio_num_t d0,
                  gpio_num_t d1,
                  gpio_num_t d2,
                  gpio_num_t d3,
                  gpio_num_t cd,
                  int width,
                  gpio_num_t usb_sense);
};
}