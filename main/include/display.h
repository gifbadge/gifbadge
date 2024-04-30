#pragma once

#include <queue.h>
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "config.h"
#include "hal/backlight.h"
#include "hal/display.h"

enum DISPLAY_OPTIONS {
  DISPLAY_NONE,
  DISPLAY_FILE,
  DISPLAY_NEXT,
  DISPLAY_PREVIOUS,
  DISPLAY_BATT,
  DISPLAY_OTA,
  DISPLAY_NO_STORAGE,
  DISPLAY_SPECIAL_1,
  DISPLAY_SPECIAL_2,
};

void display_task(void *params);

std::vector<std::string> list_directory(const std::string &path);