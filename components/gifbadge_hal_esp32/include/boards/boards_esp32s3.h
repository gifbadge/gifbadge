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

namespace Boards::esp32::s3 {
class esp32s3 : public Boards::Board {
public:
 esp32s3();
 ~esp32s3() override = default;

 void PmLock() override;
 void PmRelease() override;

 Config *GetConfig() override;
 void DebugInfo() override;
 const char *SwVersion() override;
  void Reset() override;
  char *SerialNumber() override;
  void BootInfo() override;
  bool OtaCheck() override;
  OtaError OtaValidate() override;
  void OtaInstall() override;
  int OtaStatus() override;

 protected:
 esp_pm_lock_handle_t pmLockHandle = nullptr;
 Config_NVS *_config;
 char serial[18];

 static void OtaInstallTask(void *arg);
  TaskHandle_t _ota_task_handle;
  int _ota_status;

};
}