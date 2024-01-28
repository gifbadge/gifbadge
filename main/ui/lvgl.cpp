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
#include "lvgl.h"

#include "ui/lvgl.h"
#include "keys.h"
#include "config.h"
#include "hal/battery.h"
#include "ui/main_menu.h"
#include "ui/style.h"

static const char *TAG = "MENU";


//TODO: This is bad
std::shared_ptr<ImageConfig> image_config;
std::shared_ptr<Battery> battery_config;


//Static Variables
static lv_disp_drv_t disp_drv;
static TaskHandle_t lvgl_task;
static flushCbData cbData;
static esp_timer_handle_t lvgl_tick_timer;
static lv_color_t *buf1;
static lv_color_t *buf2;
static lv_disp_draw_buf_t disp_buf;
static bool menu_state;
static std::shared_ptr<Board> _board;
static SemaphoreHandle_t lvgl_mux = nullptr;
static lv_indev_drv_t keyboard_drv;
static lv_indev_drv_t touch_drv;






//Exported Variables
extern "C"{
lv_indev_t *lvgl_encoder;
lv_indev_t *lvgl_touch;
}



static bool flush_ready([[maybe_unused]] esp_lcd_panel_io_handle_t panel_io,
                        [[maybe_unused]] esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    auto *disp_driver = (lv_disp_drv_t *) user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_draw_bitmap(cbData.panelHandle,
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

void keyboard_read(lv_indev_drv_t *, lv_indev_data_t *data) {
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

bool lvgl_menu_state(){
    return menu_state;
}

void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto touch = static_cast<Touch *>(drv->user_data);
    auto i = touch->read();
    if (i.first > 0 && i.second > 0) {
        data->point.x = (lv_coord_t) i.first;
        data->point.y = (lv_coord_t) i.second;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_init(std::shared_ptr<Board> board, std::shared_ptr<ImageConfig> _image_config) {
    image_config = std::move(_image_config);
    _board = std::move(board);

    battery_config = _board->getBattery();
    menu_state = false;

    lv_init();
    int h_res = _board->getDisplay()->getResolution().first;
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    buf1 = (lv_color_t *) heap_caps_malloc(h_res * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    buf2 = (lv_color_t *) heap_caps_malloc(h_res * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, h_res * 60);


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


    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = static_cast<lv_coord_t>(_board->getDisplay()->getResolution().first);
    disp_drv.ver_res = static_cast<lv_coord_t>(_board->getDisplay()->getResolution().second);
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &disp_buf;
//    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    style_init();
    lv_indev_drv_init(&keyboard_drv);
    keyboard_drv.type = LV_INDEV_TYPE_ENCODER;
    keyboard_drv.read_cb = keyboard_read;
    keyboard_drv.long_press_time = 400;
    lvgl_encoder = lv_indev_drv_register(&keyboard_drv);
    lv_timer_set_period(keyboard_drv.read_timer, 200); //Slow down key reads


    if (_board->getTouch()) {
        lv_indev_drv_init(&touch_drv);
        touch_drv.type = LV_INDEV_TYPE_POINTER;
        touch_drv.read_cb = touch_read;
        touch_drv.user_data = _board->getTouch().get();
        touch_drv.long_press_time = 400;
        lvgl_touch = lv_indev_drv_register(&touch_drv);
        lv_timer_set_period(touch_drv.read_timer, 50);
    }

}

void battery_update(lv_obj_t *widget){
    ESP_LOGI(TAG, "Battery Update");
    if(battery_config->getSoc() > 90){
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_FULL);
    }
    else if(battery_config->getSoc() > 70){
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_3);
    }
    else if(battery_config->getSoc() > 45){
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_2);

    }
    else if(battery_config->getSoc() > 10){
        lv_obj_clear_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_1);
    }
    else{
        lv_obj_add_state(widget, LV_STATE_CHECKED);
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_EMPTY);

    }
}

static void battery_widget(lv_obj_t *scr){
    lv_obj_t *battery_bar = lv_obj_create(scr);
    lv_obj_set_size(battery_bar, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(battery_bar, lv_obj_get_style_bg_color(lv_scr_act(), LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_border_side(battery_bar, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(battery_bar, 0, LV_PART_MAIN);

    lv_obj_t *battery = lv_label_create(battery_bar);
    lv_obj_set_width(battery, LV_PCT(100));
    lv_obj_add_style(battery, &battery_style_normal, 0);
    lv_obj_add_style(battery, &battery_style_empty, LV_STATE_CHECKED);
    lv_obj_set_pos(battery,0, 10);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_state(battery, LV_STATE_CHECKED);


    //TODO: See why this causes a freeze in LVGL
        lv_obj_add_event_cb(battery, [](lv_event_t *e){
            battery_update(lv_event_get_target(e));
        }, LV_EVENT_REFRESH, nullptr);
        battery_update(battery);

        lv_timer_t * timer = lv_timer_create([](lv_timer_t * timer){
            auto *obj = (lv_obj_t *) timer->user_data;
            lv_event_send(obj, LV_EVENT_REFRESH, nullptr);

            }, 1000,  battery);
        lv_obj_add_event_cb(battery, [](lv_event_t *e){
            auto * timer = (lv_timer_t *)e->user_data;
            lv_timer_del(timer);
        }, LV_EVENT_DELETE, timer);

}


void lvgl_menu_open() {
    ESP_LOGI(TAG, "Open");
    menu_state = true;

    cbData.panelHandle = _board->getDisplay()->getPanelHandle();

    cbData.callbackEnabled = _board->getDisplay()->onColorTransDone(flush_ready, &disp_drv);



    vTaskResume(lvgl_task);




    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    if (lvgl_lock(-1)) {
        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(lvgl_encoder, g);
        lv_obj_t *scr = lv_obj_create(nullptr);
        lv_scr_load(scr);
        battery_widget(lv_layer_top());
        lvgl_unlock();
    }
    main_menu();

    ESP_LOGI(TAG, "Open Done");
}



