
#include <lvgl.h>

#include "ui/main_menu.h"
#include "ui/file_options.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/lvgl.h"
#include "display.h"
#include "ui/style.h"
#include "ui/device_group.h"


static void exit_callback(lv_event_t *e){
    auto * obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    LV_LOG_USER("%s", lv_label_get_text(obj));
    TaskHandle_t handle = xTaskGetHandle("LVGL");
    xTaskNotifyIndexed(handle, 0, LVGL_STOP, eSetValueWithOverwrite);
    handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
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




void main_menu()
{
    if (lvgl_lock(-1)) {
        lv_obj_t *scr = lv_scr_act();
        new_group();
        lv_obj_t *main_menu = lv_file_list_create(scr);
        lv_obj_t *file_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *file_label = lv_label_create(file_btn);
        lv_obj_add_style(file_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(file_label, "File Select");
        lv_obj_t *exit_btn = lv_file_list_add(main_menu, nullptr);
        lv_obj_t *exit_label = lv_label_create(exit_btn);
        lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(exit_label, "Exit");
        lv_obj_add_event_cb(file_label, file_select_callback, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(exit_label, exit_callback, LV_EVENT_CLICKED, nullptr);
        lv_file_list_scroll_to_view(main_menu, 0);
        lvgl_unlock();
    }
}

