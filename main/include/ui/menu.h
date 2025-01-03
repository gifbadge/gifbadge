#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <semphr.h>


#include "log.h"
#include <lvgl.h>
#include "hal/config.h"
#include "hal/board.h"
#include "display.h"

#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_PRIORITY     2

struct flushCbData {
  hal::display::Display *display;
  bool callbackEnabled;
};

extern "C" {
extern lv_indev_t *lvgl_encoder;
extern lv_indev_t *lvgl_touch;
}

enum LVGL_TASK_SIGNALS {
  LVGL_NONE,
  LVGL_STOP,
  LVGL_EXIT,
  LVGL_RESUME
};

bool lvgl_lock(int timeout_ms);
void lvgl_unlock();

void lvgl_init(Boards::Board *);
void lvgl_wake_up();
void lvgl_menu_open();
bool lvgl_menu_state();

lv_obj_t *create_screen();
void destroy_screens();

typedef lv_obj_t *(*MenuType)();

void lv_tick(TimerHandle_t);

