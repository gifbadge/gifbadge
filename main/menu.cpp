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
#include "keys.h"
#include "touch.h"
#include "display.h"
#include "config.h"
#include "file_util.h"
#include "hal/battery.h"


static const char *TAG = "MENU";

static SemaphoreHandle_t lvgl_mux = nullptr;

//TODO: This is bad
std::shared_ptr<ImageConfig> image_config;
std::shared_ptr<Battery> battery_config;


bool Menu::flush_ready([[maybe_unused]] esp_lcd_panel_io_handle_t panel_io,
                       [[maybe_unused]] esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    auto *disp_driver = (lv_disp_drv_t *) user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

void Menu::flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    auto cbData = (flushCbData *) drv->user_data;
    esp_lcd_panel_draw_bitmap(cbData->panelHandle,
                              area->x1,
                              area->y1,
                              area->x2 + 1,
                              area->y2 + 1,
                              color_map);
    if(!cbData->callbackEnabled) {
        lv_disp_flush_ready(drv); //Need to only do this on RGB displays
    }
}

void Menu::tick([[maybe_unused]] void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}


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

static lv_style_t battery_style;
static lv_style_t container_style;
static lv_style_t icon_style;

LV_FONT_DECLARE(material_icons);
LV_FONT_DECLARE(material_icons_56)

Menu::Menu(std::shared_ptr<Board> board, std::shared_ptr<ImageConfig> _image_config): _board(std::move(board)) {
    image_config = std::move(_image_config);
    battery_config = _board->getBattery();

    lv_init();
    int h_res = _board->getDisplay()->getResolution().first;
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    buf1 = (lv_color_t *) heap_caps_malloc(h_res * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    buf2 = (lv_color_t *) heap_caps_malloc(h_res * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, h_res * 20);


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

    xTaskCreate(task, "LVGL", 10000, this, LVGL_TASK_PRIORITY, &lvgl_task);

    lv_style_init(&battery_style);
    lv_style_set_text_color(&battery_style, lv_color_black());
    lv_style_set_text_font(&battery_style, &lv_font_montserrat_28);
    lv_style_set_text_align(&battery_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&container_style);
    lv_style_set_pad_all(&container_style, 0);
    lv_style_set_border_side(&container_style, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_opa(&container_style, 0);

    lv_style_init(&icon_style);
    lv_style_set_text_color(&icon_style, lv_color_black());
    lv_style_set_text_font(&icon_style, &material_icons_56);

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
    lv_obj_t *slideshow_time_container;
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
        lv_dropdown_add_option(fields->file, std::filesystem::path(f).filename().c_str(), LV_DROPDOWN_POS_LAST);
    }
    lv_obj_add_event_cb(fields->folder, folder_update, LV_EVENT_VALUE_CHANGED, fields);
}

static void disable_slideshow_time(FileSelect_Objects *fields){
    ESP_LOGI(TAG, "Disable Slideshow Called %i", lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED);
    bool slideshow_state = lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED;

    if(slideshow_state) {
        lv_obj_clear_state(fields->slideshow_time, LV_STATE_DISABLED);
        lv_obj_clear_flag(fields->slideshow_time_container, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_state(fields->slideshow_time, LV_STATE_DISABLED);
        lv_obj_add_flag(fields->slideshow_time_container, LV_OBJ_FLAG_HIDDEN);
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


void battery_update(lv_obj_t *widget){
    ESP_LOGI(TAG, "Battery Update");
    if(battery_config->getSoc() > 90){
        lv_style_set_text_color(&battery_style, lv_color_black());
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_FULL);
    }
    else if(battery_config->getSoc() > 70){
        lv_style_set_text_color(&battery_style, lv_color_black());
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_3);
    }
    else if(battery_config->getSoc() > 45){
        lv_style_set_text_color(&battery_style, lv_color_black());
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_2);

    }
    else if(battery_config->getSoc() > 10){
        lv_style_set_text_color(&battery_style, lv_color_black());
        lv_label_set_text(widget, LV_SYMBOL_BATTERY_1);

    }
    else{
        lv_style_set_text_color(&battery_style, lv_color_hex(0xFF0000));
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
    lv_obj_add_style(battery, &battery_style, 0);
    lv_obj_set_pos(battery,0, 10);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);

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

static lv_obj_t* lvgl_switch(lv_obj_t *parent, const char *text){
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_add_style(container, &container_style, LV_PART_MAIN);
    lv_obj_set_size(container, lv_pct(95), LV_SIZE_CONTENT);
    lv_obj_t *label = lv_label_create(container);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(label, &icon_style, LV_PART_MAIN);
    lv_label_set_text(label, text);
    lv_obj_t *_switch = lv_switch_create(container);
    lv_obj_align(_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(_switch, 102, 60);
    return _switch;
}

static void FileSelect() {
    auto *fields = new FileSelect_Objects;
    if (Menu::lock(-1)) {
        lv_obj_t *parent = lv_obj_create(lv_scr_act());
        lv_obj_set_size(parent, LV_PCT(100), LV_PCT(100));
        lv_obj_set_align(parent, LV_ALIGN_CENTER);
        lv_obj_set_style_radius(parent, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_clip_corner(parent, true, 0);
        lv_obj_set_style_pad_bottom(parent, 50, LV_PART_MAIN);
        lv_obj_set_style_pad_top(parent, 50, LV_PART_MAIN);
        lv_obj_add_event_cb(parent, on_window_delete, LV_EVENT_DELETE, lv_group_get_default());
        fields->win = parent;
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_cross_place(parent, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_style_flex_main_place(parent, LV_FLEX_ALIGN_CENTER, 0);

        lv_obj_t *cont_flex = lv_obj_create(parent);
        lv_obj_set_style_border_side(cont_flex, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
        lv_obj_set_flex_grow(cont_flex, 1);
        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(lv_indev_get_act(), g);
        lv_obj_set_width(cont_flex, lv_pct(100));

        lv_obj_center(cont_flex);
        lv_obj_set_flex_flow(cont_flex, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont_flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


        lv_obj_set_scroll_snap_y(cont_flex, LV_SCROLL_SNAP_CENTER);

        lv_obj_t *folder_container = lv_obj_create(cont_flex);
        lv_obj_add_style(folder_container, &container_style, LV_PART_MAIN);
        lv_obj_set_size(folder_container, lv_pct(95), LV_SIZE_CONTENT);

        lv_obj_t *folder_label = lv_label_create(folder_container);
        lv_obj_align(folder_label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_style(folder_label, &icon_style, LV_PART_MAIN);
        lv_label_set_text(folder_label, "\ue2c8");

        lv_obj_t *folder_dropdown = lv_dropdown_create(folder_container);
        lv_obj_t *list = lv_dropdown_get_list(folder_dropdown);
        lv_obj_set_style_text_font(folder_dropdown, &lv_font_montserrat_28, LV_PART_MAIN);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_28, LV_PART_MAIN);
        lv_obj_set_width(folder_dropdown, LV_PCT(80));
        lv_obj_align(folder_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
        fields->folder = folder_dropdown;

        lv_obj_t *file_container = lv_obj_create(cont_flex);
        lv_obj_add_style(file_container, &container_style, LV_PART_MAIN);
        lv_obj_set_size(file_container, lv_pct(95), LV_SIZE_CONTENT);

        lv_obj_t *file_label = lv_label_create(file_container);
        lv_obj_align(file_label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_style(file_label, &icon_style, LV_PART_MAIN);
        lv_label_set_text(file_label, "\ue3f4");

        lv_obj_t *file_dropdown = lv_dropdown_create(file_container);
        list = lv_dropdown_get_list(file_dropdown);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_28, LV_PART_MAIN);
        lv_obj_set_style_text_font(file_dropdown, &lv_font_montserrat_28, LV_PART_MAIN);
        lv_obj_set_width(file_dropdown, LV_PCT(80));
        lv_obj_align(file_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
        fields->file = file_dropdown;
        lv_dropdown_clear_options(file_dropdown);
        lv_dropdown_add_option(file_dropdown, "Entire Folder", LV_DROPDOWN_POS_LAST);
        lv_dropdown_clear_options(folder_dropdown);
        std::filesystem::path path = image_config->getDirectory();
        lv_dropdown_add_option(folder_dropdown, "UP", LV_DROPDOWN_POS_LAST);
        lv_dropdown_add_option(folder_dropdown, path.c_str(), LV_DROPDOWN_POS_LAST);
        lv_dropdown_set_selected(folder_dropdown, 1);

        for (auto &folder: list_folders(path)) {
            lv_dropdown_add_option(folder_dropdown, folder.c_str(), LV_DROPDOWN_POS_LAST);
            ESP_LOGI(TAG, "%s", folder.c_str());
        }

        for (auto &f: list_directory(path)) {
            lv_dropdown_add_option(file_dropdown, std::filesystem::path(f).filename().c_str(), LV_DROPDOWN_POS_LAST);
        }
        int32_t selected_file = lv_dropdown_get_option_index(file_dropdown, image_config->getFile().filename().c_str());

        if (selected_file > 0) {
            lv_dropdown_set_selected(file_dropdown, selected_file);
        }

        lv_obj_add_event_cb(folder_dropdown, folder_update, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *lock_button = lvgl_switch(cont_flex, "\ue897");
        fields->locked = lock_button;
        if (image_config->getLocked()) {
            lv_obj_add_state(lock_button, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(lock_button, [](lv_event_t *e){
            disable_slideshow((FileSelect_Objects *)e->user_data);
            }, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *slideshow_button = lvgl_switch(cont_flex, "\ue41b");
        fields->slideshow = slideshow_button;
        if (image_config->getSlideShow()) {
            lv_obj_add_state(slideshow_button, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(slideshow_button, [](lv_event_t *e){
            disable_slideshow_time((FileSelect_Objects *)e->user_data);
        }, LV_EVENT_VALUE_CHANGED, fields);

        lv_obj_t *time_container = lv_obj_create(cont_flex);
        fields->slideshow_time_container = time_container;
        lv_obj_add_style(time_container, &container_style, LV_PART_MAIN);
        lv_obj_set_size(time_container, lv_pct(95), LV_SIZE_CONTENT);
        lv_obj_t *slideshow_time_label = lv_label_create(time_container);
        lv_obj_align(slideshow_time_label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_style(slideshow_time_label, &icon_style, LV_PART_MAIN);
        lv_label_set_text(slideshow_time_label, "\ue41b\ue425");
        lv_obj_t * slideshow_time = lv_roller_create(time_container);
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
        lv_obj_align(slideshow_time, LV_ALIGN_RIGHT_MID, 0, 0);


        fields->slideshow_time = slideshow_time;

        disable_slideshow(fields);
        disable_slideshow_time(fields);

        lv_obj_t *button_row = lv_obj_create(parent);
        lv_obj_set_flex_grow(button_row, 0);
        lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_add_style(button_row, &container_style, LV_PART_MAIN);
        lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(button_row, LV_PCT(100), 45);

//        lv_obj_add_flag(button_row, LV_OBJ_FLAG_FLOATING);
//        lv_obj_align(button_row, LV_ALIGN_BOTTOM_LEFT, 0, 0);//-lv_obj_get_style_pad_right(cont_flex, LV_PART_MAIN));

        lv_obj_t *save_button = lv_btn_create(button_row);
        lv_obj_clear_flag(save_button, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_t *save_button_label = lv_label_create(save_button);
        lv_obj_set_style_text_font(save_button_label, &material_icons, LV_PART_MAIN);
        lv_label_set_text(save_button_label, "\ue161");
        lv_obj_set_align(save_button_label, LV_ALIGN_CENTER);
        lv_obj_set_size(save_button, LV_PCT(50), 40);
        lv_obj_add_event_cb(save_button, [](lv_event_t *e) {
            ESP_LOGI(TAG, "Save clicked");
            Menu::lock(-1);
            auto *fields = (FileSelect_Objects *) e->user_data;
            char tmp[128];
            lv_dropdown_get_selected_str(fields->folder, tmp, sizeof(tmp));
            std::filesystem::path dir = tmp;
            ESP_LOGI(TAG, "%s", dir.c_str());
            image_config->setDirectory(dir.c_str());
            lv_dropdown_get_selected_str(fields->file, tmp, sizeof(tmp));
            if (strcmp(tmp, "Entire Folder") == 0) {
                strcpy(tmp, "");
            }
            else{
                ESP_LOGI(TAG, "%s", (dir/tmp).c_str());
                image_config->setFile(dir/tmp);
            }
            image_config->setLocked(lv_obj_get_state(fields->locked)&LV_STATE_CHECKED);
            image_config->setSlideShow(lv_obj_get_state(fields->slideshow)&LV_STATE_CHECKED);
            image_config->setSlideShowTime((lv_roller_get_selected(fields->slideshow_time)+1)*15);

            image_config->save();
            lv_obj_del(fields->win);
            free(fields);
            Menu::unlock();
        }, LV_EVENT_PRESSED, fields);

        lv_obj_t *exit_button = lv_btn_create(button_row);
        lv_obj_clear_flag(exit_button, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_t *exit_button_label = lv_label_create(exit_button);
        lv_obj_set_style_text_font(exit_button_label, &material_icons, LV_PART_MAIN);
        lv_label_set_text(exit_button_label, "\ue5c9");
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



static void MainMenu() {
    if (Menu::lock(-1)) {
        lv_obj_t *scr = lv_scr_act();
        battery_widget(lv_layer_top());
        lv_obj_add_event_cb(scr, [](lv_event_t *e){
            auto *g = (lv_group_t *)lv_obj_get_group(e->target);
            lv_indev_set_group(lv_indev_get_act(), g);
            }, LV_EVENT_SCREEN_LOADED, nullptr);
        lv_obj_t *cont_flex = lv_obj_create(lv_scr_act());
        lv_obj_set_style_radius(cont_flex, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_clip_corner(cont_flex, true, 0);
        lv_obj_set_size(cont_flex,  LV_PCT(100), LV_PCT(100));
        lv_obj_align(cont_flex, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_flex_flow(cont_flex, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont_flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

//        lv_obj_set_style_flex_cross_place(cont_flex, LV_FLEX_ALIGN_CENTER, 0);

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

void Menu::open() {
    ESP_LOGI(TAG, "Open");
    state = true;

    cbData.panelHandle = _board->getDisplay()->getPanelHandle();

    cbData.callbackEnabled = _board->getDisplay()->onColorTransDone(flush_ready, &disp_drv);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = static_cast<lv_coord_t>(_board->getDisplay()->getResolution().first);
    disp_drv.ver_res = static_cast<lv_coord_t>(_board->getDisplay()->getResolution().second);
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = &cbData;
//    disp_drv.full_refresh = 1;
    disp = lv_disp_drv_register(&disp_drv);

    vTaskResume(lvgl_task);

    lv_indev_drv_init(&keyboard_drv);
    keyboard_drv.type = LV_INDEV_TYPE_ENCODER;
    keyboard_drv.read_cb = keyboard_read;
    keyboard_drv.long_press_time = 400;
    keyboard_dev = lv_indev_drv_register(&keyboard_drv);
    lv_timer_set_period(keyboard_drv.read_timer, 200); //Slow down key reads


    if(_board->getTouch()){
        lv_indev_drv_init(&touch_drv);
        touch_drv.type = LV_INDEV_TYPE_POINTER;
        touch_drv.read_cb = touch_read;
        touch_drv.user_data = _board->getTouch().get();
        touch_drv.long_press_time = 400;
        touch_dev = lv_indev_drv_register(&touch_drv);
        lv_timer_set_period(touch_drv.read_timer, 50);
    }


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
    _board->getDisplay()->onColorTransDone(nullptr, nullptr);

    if (lock(-1)) {
        lv_indev_delete(keyboard_dev);
        if(_board->getTouch()) {
            lv_indev_delete(touch_dev);
        }
        lv_disp_remove(disp);
        unlock();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); //Wait some time so the task can finish

    ESP_ERROR_CHECK(esp_timer_stop(lvgl_tick_timer));
    state = false;
    ESP_LOGI(TAG, "Close Done");
}

void Menu::keyboard_read(lv_indev_drv_t *, lv_indev_data_t *data) {
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

bool Menu::is_open() const {
    return state;
}

void Menu::touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto touch = static_cast<Touch *>(drv->user_data);
    auto i = touch->read();
    if (i.first > 0 && i.second > 0) {
        data->point.x = (lv_coord_t)i.first;
        data->point.y = (lv_coord_t)i.second;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
