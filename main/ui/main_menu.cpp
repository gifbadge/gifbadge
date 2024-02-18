
#include <lvgl.h>

#include "ui/main_menu.h"
#include "ui/file_options.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/menu.h"
#include "display.h"
#include "ui/style.h"
#include "ui/device_group.h"
#include "ui/storage.h"
#include "hw_init.h"


static void exit_callback(lv_event_t *e){
    auto * obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    LV_LOG_USER("%s", lv_label_get_text(obj));
    TaskHandle_t handle = xTaskGetHandle("LVGL");
    xTaskNotifyIndexed(handle, 0, LVGL_STOP, eSetValueWithOverwrite);
}

static void file_options_close(lv_event_t *e){
    lv_obj_t *obj = lv_obj_get_parent(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
    restore_group(obj);
    lv_screen_load(lv_obj_get_screen(obj));
}

static void file_select_callback(lv_event_t *e){
    auto * obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    LV_LOG_USER("%s", lv_label_get_text(obj));
    lv_screen_load(lv_obj_create(nullptr));
    lv_obj_t *file_options_window = FileOptions();
    lv_obj_add_event_cb(file_options_window, file_options_close, LV_EVENT_DELETE, obj);
}

static void storage_callback(lv_event_t *e){
    auto * obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    LV_LOG_USER("%s", lv_label_get_text(obj));
    lv_screen_load(lv_obj_create(nullptr));
    lv_obj_t *window = storage_menu();
    lv_obj_add_event_cb(window, file_options_close, LV_EVENT_DELETE, obj);
}

static void BacklightSliderExitCallback(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target_obj(e);
    esp_err_t err;
    auto handle = nvs::open_nvs_handle("settings", NVS_READWRITE, &err);
    handle->set_item("backlight", lv_slider_get_value(obj));
    handle->commit();
    restore_group(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static void ShutdownCallback(lv_event_t *e) {
    get_board()->powerOff();
}

static void BacklightSliderChangedCallback(lv_event_t *e){
    lv_obj_t *obj = lv_event_get_target_obj(e);
    int level = lv_slider_get_value(obj)*10;
    get_board()->getBacklight()->setLevel(level);
}

void main_menu()
{
    if (lvgl_lock(-1)) {
        lv_obj_t *scr = lv_scr_act();
        new_group();
        lv_obj_t *main_menu = lv_file_list_create(scr);
        lv_file_list_icon_style(main_menu, &icon_style);


        lv_obj_t *backlight_button = lv_file_list_add(main_menu, "\ue3ab");
        lv_obj_t * slider = lv_slider_create(backlight_button);
        lv_obj_set_width(slider, lv_pct(80));
        lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
        lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(slider, lv_color_black(), LV_PART_MAIN);
        lv_slider_set_range(slider,1,10);
        lv_group_t *slider_group = lv_group_create();
        lv_group_add_obj(slider_group, slider);
        lv_obj_add_event_cb(slider, BacklightSliderChangedCallback, LV_EVENT_VALUE_CHANGED, backlight_button);
        lv_obj_add_event_cb(slider, BacklightSliderExitCallback, LV_EVENT_RELEASED, backlight_button);
        esp_err_t err;
        int backlight_level;
        auto handle = nvs::open_nvs_handle("settings", NVS_READWRITE, &err);
        err = handle->get_item("backlight", backlight_level);
        if(err == ESP_ERR_NVS_NOT_FOUND){
            backlight_level = 10;
        }
        lv_slider_set_value(slider, backlight_level, LV_ANIM_OFF);


        lv_obj_t *file_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *file_label = lv_label_create(file_btn);
        lv_obj_add_style(file_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(file_label, "File Select");

        lv_obj_t *storage_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *storage_label = lv_label_create(storage_btn);
        lv_obj_add_style(storage_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(storage_label, "Storage");

        lv_obj_t *shutdown_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *shutdown_label = lv_label_create(shutdown_btn);
        lv_obj_add_style(shutdown_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(shutdown_label, "Shutdown");

        lv_obj_t *exit_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *exit_label = lv_label_create(exit_btn);
        lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(exit_label, "Exit");

        lv_obj_add_event_cb(file_label, file_select_callback, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(storage_label, storage_callback, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(shutdown_label, ShutdownCallback, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(exit_label, exit_callback, LV_EVENT_CLICKED, nullptr);

        lv_file_list_scroll_to_view(main_menu, 0);
        lvgl_unlock();
    }
}

