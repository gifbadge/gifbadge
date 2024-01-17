#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include "esp_lcd_panel_io.h"
#include "lvgl.h"
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

class Menu {
public:
    explicit Menu(std::shared_ptr<Board>, std::shared_ptr<ImageConfig>);

    ~Menu();

    void open();

    void close();

    static bool flush_ready([[maybe_unused]] esp_lcd_panel_io_handle_t panel_io,
                            [[maybe_unused]] esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

    static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);


    static void tick([[maybe_unused]] void *);

    static bool lock(int timeout_ms);

    static void unlock();

    static void task([[maybe_unused]] void *arg);

    static void keyboard_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

    static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);


    bool is_open() const;


private:
    lv_color_t *buf1;
    lv_color_t *buf2;
    lv_disp_t *disp;
    TaskHandle_t lvgl_task;
    esp_timer_handle_t lvgl_tick_timer;
    esp_lcd_panel_io_handle_t _io_handle = nullptr;
    esp_lcd_panel_handle_t _panel_handle = nullptr;
    lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    lv_disp_drv_t disp_drv;      // contains callback functions
    lv_indev_t *keyboard_dev;
    lv_indev_t *touch_dev;
    lv_indev_drv_t keyboard_drv;
    lv_indev_drv_t touch_drv;
    std::shared_ptr<Board> _board;

    bool state = false;
    flushCbData cbData;

};

enum LVGL_TASK_SIGNALS{
    LVGL_NONE,
    LVGL_STOP,
    LVGL_EXIT
};