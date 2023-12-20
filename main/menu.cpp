#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include <sys/stat.h>
#include <memory>
#include <nvs_handle.hpp>
#include <utility>
#include <map>
#include "esp_lcd_panel_io.h"
#include "lvgl.h"

#include "menu.h"
#include "input.h"
#include "display.h"
#include "config.h"
#include "file_util.h"


static const char *TAG = "MENU";

//TODO: This is bad
std::shared_ptr<ImageConfig> image_config;
std::shared_ptr<BatteryConfig> battery_config;
extern int V_RES;
extern int H_RES;
int DISPLAY_TYPE = 0;

bool Menu::flush_ready([[maybe_unused]] esp_lcd_panel_io_handle_t panel_io,
                       [[maybe_unused]] esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    auto *disp_driver = (lv_disp_drv_t *) user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

void Menu::flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    auto panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    esp_lcd_panel_draw_bitmap(panel_handle,
                              area->x1,
                              area->y1,
                              area->x2 + 1,
                              area->y2 + 1,
                              color_map);
    if(DISPLAY_TYPE == 2) {
        lv_disp_flush_ready(drv);
    }
}

void Menu::tick([[maybe_unused]] void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static SemaphoreHandle_t lvgl_mux = nullptr;

bool Menu::lock(int timeout_ms) {
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}


void Menu::unlock() {
    xSemaphoreGiveRecursive(lvgl_mux);
}

void Menu::task(void *arg) {
    bool running = true;
    ESP_LOGI(TAG, "Starting LVGL task");
    Menu *menu = (Menu *) arg;
    uint32_t task_delay_ms = 0;
    vTaskSuspend(nullptr); //Wait until we are actually needed

    while (running) {
        uint32_t option;
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, task_delay_ms/portTICK_PERIOD_MS);
        switch (option) {
            case LVGL_STOP:
                menu->close();
                vTaskSuspend(nullptr);
                break;
            case LVGL_EXIT:
                running = false;
                break;
            default:
                if (menu->lock(-1)) {
                    task_delay_ms = lv_timer_handler();
                    // Release the mutex
                    menu->unlock();
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

Menu::Menu(esp_lcd_panel_handle_t panel_handle, std::shared_ptr<ImageConfig> _image_config, std::shared_ptr<BatteryConfig> _battery_config, int display_type) : _panel_handle{
        panel_handle} {
    image_config = std::move(_image_config);
    battery_config = std::move(_battery_config);
    DISPLAY_TYPE = display_type;

    lv_init();
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    buf1 = (lv_color_t *) heap_caps_malloc(H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    buf2 = (lv_color_t *) heap_caps_malloc(H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, H_RES * 20);


    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &tick, .name = "lvgl_tick"};
    lvgl_tick_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));

    xTaskCreate(task, "LVGL", 10000, this, LVGL_TASK_PRIORITY, &lvgl_task);

}

Menu::~Menu() {
    close();
    ESP_ERROR_CHECK(esp_timer_delete(lvgl_tick_timer));
    free(buf1);
    free(buf2);
}

static void on_window_delete(lv_event_t *e){
        ESP_LOGI(TAG, "DELETE CALLED ON WINDOW");
        lv_group_set_default((lv_group_t *) e->user_data);
        lv_indev_set_group(lv_indev_get_act(), (lv_group_t *) e->user_data);
}

struct FileSelect_Objects {
    lv_obj_t *folder;
    lv_obj_t *file;
    lv_obj_t *locked;
    lv_obj_t *win;
    lv_obj_t *slideshow;
    lv_obj_t *slideshow_time;
};

static void folder_update(lv_event_t *e) {
    auto *fields = (FileSelect_Objects *) e->user_data;
    lv_obj_remove_event_cb(fields->folder, nullptr);
    char selected_folder[128];
    lv_dropdown_get_selected_str(fields->folder, selected_folder, sizeof(selected_folder));
    if (strcmp(selected_folder, "UP") == 0) {
        const char *options = lv_dropdown_get_options(fields->folder);
        char *cp = strdup(options);
        strtok(cp, "\n");
        std::string path = strtok(nullptr, "\n");
        std::string dir = path.substr(0, path.find_last_of('/'));
        strcpy(selected_folder, dir.c_str());
        ESP_LOGI(TAG, "UP %s", dir.c_str());
    }
    lv_dropdown_clear_options(fields->folder);
    lv_dropdown_add_option(fields->folder, "UP", LV_DROPDOWN_POS_LAST);
    lv_dropdown_add_option(fields->folder, selected_folder, LV_DROPDOWN_POS_LAST);
    lv_dropdown_set_selected(fields->folder, 1);

    for (auto &folder: list_folders(selected_folder)) {
        lv_dropdown_add_option(fields->folder, folder.c_str(), LV_DROPDOWN_POS_LAST);
        ESP_LOGI(TAG, "%s", folder.c_str());
    }
    ESP_LOGI(TAG, "Folder %s", selected_folder);
    lv_dropdown_clear_options(fields->file);
    lv_dropdown_add_option(fields->file, "Entire Folder", LV_DROPDOWN_POS_LAST);
    lv_dropdown_set_selected(fields->file, 0);
    for (auto &f: list_directory(selected_folder)) {
        lv_dropdown_add_option(fields->file, f.c_str(), LV_DROPDOWN_POS_LAST);
    }
    lv_obj_add_event_cb(fields->folder, folder_update, LV_EVENT_VALUE_CHANGED, fields);
}

static void disable_slideshow_time(FileSelect_Objects *fields){
    ESP_LOGI(TAG, "Disable Slideshow Called %i", lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED);
    bool slideshow_state = lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED;

    if(slideshow_state) {
        lv_obj_clear_state(fields->slideshow_time, LV_STATE_DISABLED);
    }
    else {
        lv_obj_add_state(fields->slideshow_time, LV_STATE_DISABLED);
    }
}

static void disable_slideshow(FileSelect_Objects *fields){
    bool locked_state = lv_obj_get_state(fields->locked)&LV_STATE_CHECKED;
    if(locked_state){
        lv_obj_add_state(fields->slideshow, LV_STATE_DISABLED);
        lv_obj_add_state(fields->slideshow_time, LV_STATE_DISABLED);
    }
    else {
        lv_obj_clear_state(fields->slideshow, LV_STATE_DISABLED);
        disable_slideshow_time(fields);
    }
}

static void FileSelect() {
    auto *fields = new FileSelect_Objects;
    if (Menu::lock(-1)) {
        lv_obj_t *scr = lv_obj_create(lv_scr_act());
        lv_obj_set_size(scr, H_RES, V_RES);
        lv_obj_set_align(scr, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(scr, on_window_delete, LV_EVENT_DELETE, lv_group_get_default());
        fields->win = scr;
        lv_obj_t *cont_flex = lv_obj_create(scr);
        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(lv_indev_get_act(), g);
        lv_obj_set_size(cont_flex, LV_PCT(85), LV_PCT(85));
        lv_obj_align(cont_flex, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_flex_flow(cont_flex, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_cross_place(cont_flex, LV_FLEX_ALIGN_CENTER, 0);

        lv_obj_set_scroll_snap_y(cont_flex, LV_SCROLL_SNAP_START);

        lv_obj_t *folder_dropdown = lv_dropdown_create(cont_flex);
        fields->folder = folder_dropdown;
        lv_obj_t *file_dropdown = lv_dropdown_create(cont_flex);
        fields->file = file_dropdown;
        lv_dropdown_clear_options(file_dropdown);
        lv_dropdown_add_option(file_dropdown, "Entire Folder", LV_DROPDOWN_POS_LAST);
        lv_dropdown_clear_options(folder_dropdown);
        std::string path = image_config->getDirectory();
        lv_dropdown_add_option(folder_dropdown, "UP", LV_DROPDOWN_POS_LAST);
        lv_dropdown_add_option(folder_dropdown, path.c_str(), LV_DROPDOWN_POS_LAST);
        lv_dropdown_set_selected(folder_dropdown, 1);

        for (auto &folder: list_folders(path)) {
            lv_dropdown_add_option(folder_dropdown, folder.c_str(), LV_DROPDOWN_POS_LAST);
            ESP_LOGI(TAG, "%s", folder.c_str());
        }

        for (auto &f: list_directory(path)) {
            lv_dropdown_add_option(file_dropdown, f.c_str(), LV_DROPDOWN_POS_LAST);
        }
        int32_t selected_file = lv_dropdown_get_option_index(file_dropdown, image_config->getFile().c_str());

        if (selected_file > 0) {
            lv_dropdown_set_selected(file_dropdown, selected_file);
        }

        lv_obj_add_event_cb(folder_dropdown, folder_update, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *lock_button = lv_btn_create(cont_flex);
        fields->locked = lock_button;
        lv_obj_t *lock_button_label = lv_label_create(lock_button);
        if (image_config->getLocked()) {
            lv_obj_add_state(lock_button, LV_STATE_CHECKED);
        }
        lv_label_set_text(lock_button_label, "Lock");
        lv_obj_set_size(lock_button, LV_PCT(100), 40);
        lv_obj_set_align(lock_button_label, LV_ALIGN_CENTER);
        lv_obj_add_flag(lock_button, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(lock_button, [](lv_event_t *e){
            disable_slideshow((FileSelect_Objects *)e->user_data);
            }, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *slideshow_button = lv_btn_create(cont_flex);
        fields->slideshow = slideshow_button;
        lv_obj_t *slideshow_button_label = lv_label_create(slideshow_button);
        lv_obj_add_flag(slideshow_button, LV_OBJ_FLAG_CHECKABLE);
        lv_label_set_text(slideshow_button_label, "Slideshow");
        if (image_config->getSlideShow()) {
            lv_obj_add_state(slideshow_button, LV_STATE_CHECKED);
        }
        lv_obj_set_size(slideshow_button, LV_PCT(100), 40);
        lv_obj_set_align(slideshow_button_label, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(slideshow_button, [](lv_event_t *e){
            disable_slideshow_time((FileSelect_Objects *)e->user_data);
        }, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *slideshow_time_label = lv_label_create(cont_flex);
        lv_label_set_text(slideshow_time_label, "Slideshow time");
        lv_obj_t * slideshow_time = lv_roller_create(cont_flex);
        //Time must be in 15 second increments, otherwise the logic to convert to seconds needs to be updated
        lv_roller_set_options(slideshow_time,
                              "0:15\n"
                              "0:30\n"
                              "0:45\n"
                              "1:00\n"
                              "1:15\n"
                              "1:30\n"
                              "1:45\n"
                              "2:00\n"
                              "2:15\n"
                              "2:30\n"
                              "2:45\n"
                              "3:00",
                              LV_ROLLER_MODE_INFINITE);

        lv_roller_set_visible_row_count(slideshow_time, 3);
        lv_roller_set_selected(slideshow_time, (image_config->getSlideShowTime()/15)-1, LV_ANIM_OFF);

        fields->slideshow_time = slideshow_time;

        disable_slideshow(fields);
        disable_slideshow_time(fields);

        lv_obj_t *button_row = lv_obj_create(cont_flex);
        lv_obj_set_style_pad_all(button_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(button_row, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(button_row, LV_PCT(100), 45);

        lv_obj_t *save_button = lv_btn_create(button_row);
        lv_obj_t *save_button_label = lv_label_create(save_button);
        lv_label_set_text(save_button_label, "OK");
        lv_obj_set_align(save_button_label, LV_ALIGN_CENTER);
        lv_obj_set_size(save_button, LV_PCT(50), 40);
        lv_obj_add_event_cb(save_button, [](lv_event_t *e) {
            ESP_LOGI(TAG, "Save clicked");
            Menu::lock(-1);
            auto *fields = (FileSelect_Objects *) e->user_data;
            char tmp[128];
            lv_dropdown_get_selected_str(fields->folder, tmp, sizeof(tmp));
            ESP_LOGI(TAG, "%s", tmp);
            image_config->setDirectory(tmp);
            lv_dropdown_get_selected_str(fields->file, tmp, sizeof(tmp));
            if (strcmp(tmp, "Entire Folder") == 0) {
                strcpy(tmp, "");
            }
            ESP_LOGI(TAG, "%s", tmp);
            image_config->setFile(tmp);
            image_config->setLocked(lv_obj_get_state(fields->locked)&LV_STATE_CHECKED);
            image_config->setSlideShow(lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED);
            image_config->setSlideShowTime((lv_roller_get_selected(fields->slideshow_time)+1)*15);

            image_config->save();
            lv_obj_del(fields->win);
            free(fields);
            Menu::unlock();
        }, LV_EVENT_PRESSED, fields);

        lv_obj_t *exit_button = lv_btn_create(button_row);
        lv_obj_t *exit_button_label = lv_label_create(exit_button);
        lv_label_set_text(exit_button_label, "Cancel");
        lv_obj_set_align(exit_button_label, LV_ALIGN_CENTER);
        lv_obj_set_size(exit_button, LV_PCT(45), 40);
        lv_obj_add_event_cb(exit_button, [](lv_event_t *e) {
            ESP_LOGI(TAG, "Exit clicked");
            Menu::lock(-1);
            auto *fields = (FileSelect_Objects *) e->user_data;
            lv_obj_del(fields->win);
            free(fields);
            Menu::unlock();
        }, LV_EVENT_PRESSED, fields);

        Menu::unlock();
    }
}

static lv_style_t battery_style;

void battery_update(lv_obj_t *widget){
    ESP_LOGI(TAG, "Battery Update");
    char icon[4];
    char voltage[10];
    sprintf(voltage, " %1.3f", battery_config->getVoltage());
    if(battery_config->getVoltage() > 4){
        lv_style_set_text_color(&battery_style, lv_color_black());
        strcpy(icon, LV_SYMBOL_BATTERY_FULL);
    }
    else if(battery_config->getVoltage() > 3.7){
        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_3);
        strcpy(icon, LV_SYMBOL_BATTERY_3);

    }
    else if(battery_config->getVoltage() > 3.6){
        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_2);
        strcpy(icon, LV_SYMBOL_BATTERY_2);

    }
    else if(battery_config->getVoltage() > 3.5){
        lv_style_set_text_color(&battery_style, lv_color_black());
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_1);
        strcpy(icon, LV_SYMBOL_BATTERY_1);

    }
    else{
        lv_style_set_text_color(&battery_style, lv_color_hex(0xFF0000));
//        lv_label_set_text(widget, LV_SYMBOL_BATTERY_EMPTY);
        strcpy(icon, LV_SYMBOL_BATTERY_EMPTY);

    }
    char out[20] = "";
    strcat(out, icon);
    strcat(out, voltage);
    lv_label_set_text(widget, out);
}

static void MainMenu() {
    if (Menu::lock(-1)) {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_add_event_cb(scr, [](lv_event_t *e){
            auto *g = (lv_group_t *)lv_obj_get_group(e->target);
            lv_indev_set_group(lv_indev_get_act(), g);
            }, LV_EVENT_SCREEN_LOADED, nullptr);
        lv_obj_t *cont_flex = lv_obj_create(lv_scr_act());
        lv_obj_set_size(cont_flex,  LV_PCT(80), LV_PCT(80));
        lv_obj_align(cont_flex, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_flex_flow(cont_flex, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_cross_place(cont_flex, LV_FLEX_ALIGN_CENTER, 0);

        lv_style_init(&battery_style);
        lv_style_set_text_color(&battery_style, lv_color_black());
        lv_style_set_text_font(&battery_style, &lv_font_montserrat_28);

        lv_obj_t *battery = lv_label_create(cont_flex);
        lv_obj_set_size(battery, LV_PCT(20), LV_PCT(20));
        lv_obj_set_size(battery, LV_PCT(100), LV_PCT(20));
        lv_obj_add_style(battery, &battery_style, 0);
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


        lv_obj_t *file_button = lv_btn_create(cont_flex);
        lv_obj_t *file_button_label = lv_label_create(file_button);
        lv_label_set_text(file_button_label, "File Select");
        lv_obj_add_event_cb(file_button, [](lv_event_t *e) {
            FileSelect();
        }, LV_EVENT_PRESSED, nullptr);
        lv_obj_set_size(file_button, LV_PCT(100), 40);
        lv_obj_set_align(file_button_label, LV_ALIGN_CENTER);

        lv_obj_t *usb_button = lv_btn_create(cont_flex);
        lv_obj_add_flag(usb_button, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_t *usb_button_label = lv_label_create(usb_button);
        lv_label_set_text(usb_button_label, "Disable USB Storage");
        esp_err_t err;
        std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("usb", NVS_READWRITE, &err);
        bool usb_state;
        handle->get_item("usb_on", usb_state);
        if (usb_state) {
            lv_obj_add_state(usb_button, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(usb_button, [](lv_event_t *e) {
            esp_err_t err;
            std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("usb", NVS_READWRITE, &err);
            lv_obj_t *btn = lv_event_get_target(e);
            handle->set_item("usb_on", lv_obj_get_state(btn)&LV_STATE_CHECKED);
            handle->commit();
        }, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_set_size(usb_button, LV_PCT(100), 40);
        lv_obj_set_align(usb_button_label, LV_ALIGN_CENTER);

        lv_obj_t *exit_button = lv_btn_create(cont_flex);
        lv_obj_t *exit_button_label = lv_label_create(exit_button);
        lv_label_set_text(exit_button_label, "Exit");
        lv_obj_set_size(exit_button, LV_PCT(100), 40);
        lv_obj_set_align(exit_button_label, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(exit_button, [](lv_event_t *e) {
            TaskHandle_t handle = xTaskGetHandle("LVGL");
            xTaskNotifyIndexed(handle, 0, LVGL_STOP, eSetValueWithOverwrite);
            handle = xTaskGetHandle("display_task");
            xTaskNotifyIndexed(handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
        }, LV_EVENT_PRESSED, nullptr);


        Menu::unlock();
    }
}

void Menu::open(esp_lcd_panel_io_handle_t io_handle, QueueHandle_t input_queue, QueueHandle_t touch_queue) {
    _io_handle = io_handle;
    ESP_LOGI(TAG, "Open");
    state = true;

#ifdef CONFIG_GC9A01
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = flush_ready};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &conf, &disp_drv);
#endif

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = H_RES;
    disp_drv.ver_res = V_RES;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = _panel_handle;
    disp_drv.full_refresh = 1;
    disp = lv_disp_drv_register(&disp_drv);

    vTaskResume(lvgl_task);

    lv_indev_drv_init(&keyboard_drv);
    keyboard_drv.type = LV_INDEV_TYPE_ENCODER;
    keyboard_drv.read_cb = keyboard_read;
    keyboard_drv.user_data = input_queue;
    keyboard_drv.long_press_time = 400;
    keyboard_dev = lv_indev_drv_register(&keyboard_drv);
    lv_timer_set_period(keyboard_drv.read_timer, 200); //Slow down key reads


    lv_indev_drv_init(&touch_drv);
    keyboard_drv.type = LV_INDEV_TYPE_POINTER;
    keyboard_drv.read_cb = touch_read;
    keyboard_drv.user_data = touch_queue;
    keyboard_drv.long_press_time = 400;
    touch_dev = lv_indev_drv_register(&touch_drv);

    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    if (Menu::lock(-1)) {
        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(keyboard_dev, g);
        lv_obj_t *scr = lv_obj_create(nullptr);
        lv_scr_load(scr);
        Menu::unlock();
    }
    MainMenu();

    ESP_LOGI(TAG, "Open Done");
}

void Menu::close() {
    ESP_LOGI(TAG, "Close");
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = nullptr};
    esp_lcd_panel_io_register_event_callbacks(_io_handle, &conf, nullptr);

    if (lock(-1)) {
        lv_indev_delete(keyboard_dev);
        lv_indev_delete(touch_dev);
        lv_disp_remove(disp);
        unlock();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); //Wait some time so the task can finish

    ESP_ERROR_CHECK(esp_timer_stop(lvgl_tick_timer));
    state = false;
    ESP_LOGI(TAG, "Close Done");
}

void Menu::keyboard_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
//    ESP_LOGI(TAG, "keyboard_read");
    std::map<EVENT_CODE, EVENT_STATE> keys = input_read();
    if(keys[KEY_UP]){
        data->enc_diff += -1;
    }
    else if(keys[KEY_DOWN]){
        data->enc_diff += 1;
    }
    else if(keys[KEY_ENTER]){
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

}

bool Menu::is_open() {
    return state;
}

void Menu::touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto queue = (QueueHandle_t) drv->user_data;
    touch_event i = {};
    if (xQueueReceive(queue, (void *) &i, 0)) {
        data->point.x = (lv_coord_t)i.x;
        data->point.y = (lv_coord_t)i.y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
