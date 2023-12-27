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

//#ifdef CONFIG_LCD_1_28_GC9A01
//#define H_RES 240
//#define V_RES 240
//#elif CONFIG_LCD_2_1_ST7706
//#define H_RES 480
//#define V_RES 480
//#endif
#include "hal/backlight.h"
#include "hal/display.h"


enum DISPLAY_OPTIONS {
    DISPLAY_NONE,
    DISPLAY_FILE,
    DISPLAY_PATH,
    DISPLAY_USB,
    DISPLAY_MENU,
    DISPLAY_NEXT,
    DISPLAY_PREVIOUS,
    DISPLAY_BATT,
    DISPLAY_OTA,
};

enum DISPLAY_TYPES {
    DISPLAY_TYPE_NONE,
    DISPLAY_TYPE_GC9A01_1_28,
    DISPLAY_TYPE_ST7706_2_1
};

struct display_task_args {
    std::shared_ptr<Display> display;
    std::shared_ptr<ImageConfig> image_config;
    std::shared_ptr<Backlight> backlight;
};


void display_task(void *params);

std::vector<std::string> list_directory(const std::string &path);