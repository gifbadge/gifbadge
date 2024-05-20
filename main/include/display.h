#pragma once

#include <queue.h>
#include <string>
#include <vector>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/config.h"
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
  DISPLAY_NOTIFY_CHANGE,
  DISPLAY_NOTIFY_USB,
};

void display_task(void *params);