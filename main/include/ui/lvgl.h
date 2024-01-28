#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include "esp_lcd_panel_io.h"
#include <lvgl.h>
#include "config.h"
#include "hal/board.h"

#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2

struct flushCbData {
    esp_lcd_panel_handle_t panelHandle;
    bool callbackEnabled;
};

extern "C"{
    extern lv_indev_t *lvgl_encoder;
    extern lv_indev_t *lvgl_touch;
}

enum LVGL_TASK_SIGNALS{
    LVGL_NONE,
    LVGL_STOP,
    LVGL_EXIT
};

bool lvgl_lock(int timeout_ms);
void lvgl_unlock();

void lvgl_init(std::shared_ptr<Board>, std::shared_ptr<ImageConfig>);
void lvgl_menu_open();
bool lvgl_menu_state();
