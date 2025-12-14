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
  DISPLAY_FILE = 1,
  DISPLAY_NEXT = 2 | noResetBit,
  DISPLAY_PREVIOUS = 3 | noResetBit,
  DISPLAY_BATT = 4,
  DISPLAY_OTA = 5,
  DISPLAY_NO_STORAGE = 6,
  DISPLAY_SPECIAL_1 = 7,
  DISPLAY_SPECIAL_2 = 8,
  DISPLAY_NOTIFY_CHANGE = 9,
  DISPLAY_NOTIFY_USB = 10,
  DISPLAY_ADVANCE = 11,
};

void display_task(void *params);

extern float last_fps;