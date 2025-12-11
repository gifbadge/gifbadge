#pragma once

#pragma once
#include <hal/board.h>
#include <esp_pm.h>
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_st7701s.h"
#include "drivers/backlight_ledc.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/keys_esp_io_expander.h"
#include "driver/sdmmc_host.h"
#include "drivers/config_nvs.h"
#include "tinyusb_msc.h"


namespace Boards::esp32::s3 {
class esp32s3 : public Board {
 public:
  size_t MemorySize() override;

  esp32s3();
 ~esp32s3() override = default;

 void PmLock() override;
 void PmRelease() override;

 hal::config::Config *GetConfig() override;
 void DebugInfo() override;
 const char *SwVersion() override;
  void Reset() override;
  char *SerialNumber() override;
  void BootInfo() override;
  bool OtaCheck() override;
 static OtaError OtaHeaderValidate(uint8_t const *data);
 OtaError OtaValidate() override;
  void OtaInstall() override;
  int OtaStatus() override;

 protected:
 esp_pm_lock_handle_t pmLockHandle = nullptr;
 hal::config::esp32s3::Config_NVS *_config;
 char serial[18];
  tinyusb_msc_storage_handle_t storage_handle = nullptr;
  char storage_base_path[6] = "/data";

 static void OtaInstallTask(void *arg);
  TaskHandle_t _ota_task_handle = nullptr;
  int _ota_status = -1;

};
}

char* lltoa(long long val, int base);