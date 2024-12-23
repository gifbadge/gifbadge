#pragma once

#include <queue.h>
#include <string>
#include <vector>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/config.h"
#include "hal/backlight.h"
#include "hal/display.h"

#define noResetBit (1<<5)

enum DISPLAY_OPTIONS {
  DISPLAY_NONE = 0,
  DISPLAY_FILE,
  DISPLAY_NEXT = 2 | noResetBit,
  DISPLAY_PREVIOUS = 3 | noResetBit,
  DISPLAY_BATT,
  DISPLAY_OTA,
  DISPLAY_NO_STORAGE,
  DISPLAY_SPECIAL_1,
  DISPLAY_SPECIAL_2,
  DISPLAY_NOTIFY_CHANGE,
  DISPLAY_NOTIFY_USB,
  DISPLAY_ADVANCE,
};

void display_task(void *params);