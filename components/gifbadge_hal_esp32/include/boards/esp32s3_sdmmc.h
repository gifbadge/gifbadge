#pragma once
#include "driver/sdmmc_host.h"
#include "boards_esp32s3.h"

namespace Boards {
class esp32s3_sdmmc : public Boards::esp32s3 {
 public:
  esp32s3_sdmmc() = default;
  ~esp32s3_sdmmc() override = default;

  bool usbConnected() override;
  StorageInfo storageInfo() override;
  int StorageFormat() override;

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
                  int width);
};
}
