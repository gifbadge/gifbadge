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

namespace Boards {
class esp32s3 : public Boards::Board {
public:
 esp32s3();
 ~esp32s3() override = default;

 void pmLock() override;
 void pmRelease() override;

 Config *getConfig() override;
 void debugInfo() override;
 const char *swVersion() override;

protected:
 esp_pm_lock_handle_t pmLockHandle = nullptr;
 Config_NVS *_config;

};
}