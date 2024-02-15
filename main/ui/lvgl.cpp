#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include <sys/stat.h>
#include <memory>
#include <utility>
#include <map>
#include "esp_lcd_panel_io.h"
#include <lvgl.h>

#include "ui/lvgl.h"
#include "keys.h"
#include "config.h"
#include "hal/battery.h"
#include "ui/main_menu.h"
#include "ui/style.h"

static const char *TAG = "MENU";


//TODO: This is bad
std::shared_ptr<ImageConfig> image_config;

//Static Variables
static lv_disp_t *disp;
static TaskHandle_t lvgl_task;
static flushCbData cbData;
static esp_timer_handle_t lvgl_tick_timer;
static bool menu_state;
static std::shared_ptr<Board> _board;
static SemaphoreHandle_t lvgl_mux = nullptr;





//Exported Variables
extern "C" {
lv_indev_t *lvgl_encoder;
lv_indev_t *lvgl_touch;
}


static bool flush_ready([[maybe_unused]] esp_lcd_panel_io_handle_t panel_io,
                        [[maybe_unused]] esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
//    auto *disp_driver = (lv_disp_t *) user_ctx;
//    lv_display_flush_ready(disp_driver);
    return false;
}

static void flush_cb(lv_disp_t *drv, const lv_area_t *area, uint8_t *color_map) {
    lv_draw_sw_rgb565_swap(color_map, 480 * 480);
    cbData.display->write(
            area->x1,
            area->y1,
            area->x2 + 1,
            area->y2 + 1,
            color_map);
    if (!cbData.callbackEnabled) {
        lv_disp_flush_ready(drv); //Need to only do this on RGB displays
    }
}

static void tick([[maybe_unused]] void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}


bool lvgl_lock(int timeout_ms) {
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}


void lvgl_unlock() {
    xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_close() {
    ESP_LOGI(TAG, "Close");
    _board->getDisplay()->onColorTransDone(nullptr, nullptr);

    if (lvgl_lock(-1)) {
        lv_obj_clean(lv_scr_act());
//        lv_disp_remove(disp);
        lvgl_unlock();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); //Wait some time so the task can finish

    ESP_ERROR_CHECK(esp_timer_stop(lvgl_tick_timer));
    menu_state = false;
    ESP_LOGI(TAG, "Close Done");
}

void task(void *) {
    bool running = true;
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = 0;
    vTaskSuspend(nullptr); //Wait until we are actually needed

    while (running) {
        uint32_t option;
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, task_delay_ms / portTICK_PERIOD_MS);
        switch (option) {
            case LVGL_STOP:
                lvgl_close();
                vTaskSuspend(nullptr);
                break;
            case LVGL_EXIT:
                running = false;
                break;
            default:
                if (lvgl_lock(-1)) {
                    task_delay_ms = lv_timer_handler();
                    // Release the mutex
                    lvgl_unlock();
                }
                if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
                    task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
                } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
                    task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
                }
                break;
        }
    }
}

void keyboard_read(lv_indev_t *, lv_indev_data_t *data) {
//    ESP_LOGI(TAG, "keyboard_read");
    std::map<EVENT_CODE, EVENT_STATE> keys = input_read();
    if (keys[KEY_UP]) {
        data->enc_diff += -1;
    } else if (keys[KEY_DOWN]) {
        data->enc_diff += 1;
    } else if (keys[KEY_ENTER]) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

}

bool lvgl_menu_state() {
    return menu_state;
}

void touch_read(lv_indev_t *drv, lv_indev_data_t *data) {

    auto touch = static_cast<Touch *>(lv_indev_get_driver_data(drv));
    auto i = touch->read();
    if (i.first > 0 && i.second > 0) {
        data->point.x = (int32_t) i.first;
        data->point.y = (int32_t) i.second;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_init(std::shared_ptr<Board> board, std::shared_ptr<ImageConfig> _image_config) {
    image_config = std::move(_image_config);
    _board = std::move(board);

    menu_state = false;

    lv_init();

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    const esp_timer_create_args_t lvgl_tick_timer_args = {
            .callback = &tick,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "lvgl_tick",
            .skip_unhandled_events = true
    };
    lvgl_tick_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));

    xTaskCreate(task, "LVGL", 10000, nullptr, LVGL_TASK_PRIORITY, &lvgl_task);


    disp = lv_display_create(_board->getDisplay()->getResolution().first, _board->getDisplay()->getResolution().second);
    lv_display_set_flush_cb(disp, flush_cb);
    size_t buffer_size = _board->getDisplay()->getResolution().first * _board->getDisplay()->getResolution().second * 2;
    ESP_LOGI(TAG, "Display Buffer Size %u", buffer_size);
    if(_board->getDisplay()->directRender()) {
        lv_display_set_buffers(disp, _board->getDisplay()->getBuffer(), _board->getDisplay()->getBuffer2(), buffer_size,
                               LV_DISPLAY_RENDER_MODE_DIRECT);
    }
    else {
        lv_display_set_buffers(disp, _board->getDisplay()->getBuffer(), nullptr, buffer_size,
                               LV_DISPLAY_RENDER_MODE_FULL);
    }
//    lv_display_set_flush_wait_cb(disp, flush_wait);

//    uint8_t *buf = static_cast<uint8_t *>(malloc(480 * 100 * 2));
//    lv_display_set_buffers(disp, buf, nullptr, 480*100*2,
//                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    style_init();
    lvgl_encoder = lv_indev_create();
    lv_indev_set_type(lvgl_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(lvgl_encoder, keyboard_read);
    lv_timer_set_period(lv_indev_get_read_timer(lvgl_encoder), 200);
//
//
//    if (_board->getTouch()) {
//        lvgl_touch = lv_indev_create();
//        lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
//        lv_indev_set_read_cb(lvgl_touch, touch_read);
//        lv_timer_set_period(lv_indev_get_read_timer(lvgl_touch), 150);
//    }

}

void battery_update(lv_obj_t *widget) {
    auto battery = static_cast<Battery *>(lv_obj_get_user_data(widget));
    if (battery->getSoc() > 90) {
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_FULL);
    } else if (battery->getSoc() > 70) {
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_3);
    } else if (battery->getSoc() > 45) {
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_2);

    } else if (battery->getSoc() > 10) {
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_1);
    } else {
        lv_obj_add_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_EMPTY);

    }
}

static void battery_widget(lv_obj_t *scr) {
    lv_obj_t *battery_bar = lv_obj_create(scr);
    lv_obj_set_size(battery_bar, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(battery_bar, lv_obj_get_style_bg_color(lv_scr_act(), LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_border_side(battery_bar, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(battery_bar, 0, LV_PART_MAIN);

    lv_obj_t *battery = lv_label_create(battery_bar);
    lv_obj_set_width(battery, LV_PCT(100));
    lv_obj_add_style(battery, &battery_style_normal, 0);
    lv_obj_add_style(battery, &battery_style_empty, LV_STATE_CHECKED);
    lv_obj_set_pos(battery, 0, 10);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_state(battery, LV_STATE_CHECKED);

    lv_obj_set_user_data(battery, _board->getBattery().get());


    //TODO: See why this causes a freeze in LVGL
    lv_obj_add_event_cb(battery, [](lv_event_t *e) {
        battery_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
    }, LV_EVENT_REFRESH, nullptr);
    battery_update(battery);

    lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
        auto *obj = (lv_obj_t *) timer->user_data;
        lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
    }, 30000, battery);
    lv_obj_add_event_cb(battery, [](lv_event_t *e) {
        auto *timer = (lv_timer_t *) e->user_data;
        lv_timer_del(timer);
    }, LV_EVENT_DELETE, timer);

}


void lvgl_menu_open() {
    ESP_LOGI(TAG, "Open");
    menu_state = true;

    cbData.display = _board->getDisplay();

    cbData.callbackEnabled = _board->getDisplay()->onColorTransDone(flush_ready, &disp);


    if(_board->getDisplay()->directRender()) {
        _board->getDisplay()->write(0, 0, _board->getDisplay()->getResolution().first,
                                    _board->getDisplay()->getResolution().second, _board->getDisplay()->getBuffer2());
    }
    vTaskResume(lvgl_task);


    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    if (lvgl_lock(-1)) {
        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(lvgl_encoder, g);
        lv_obj_t *scr = lv_obj_create(nullptr);
        lv_screen_load(scr);
        battery_widget(lv_layer_top());
        lvgl_unlock();
    }
    main_menu();

    ESP_LOGI(TAG, "Open Done");
}



