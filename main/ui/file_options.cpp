#include "lvgl.h"

#include "ui/file_options.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/file_select.h"
#include "ui/device_group.h"
#include "ui/lvgl.h"
#include "ui/style.h"

extern "C" {

lv_event_code_t EVENT_SLIDESHOW_DISABLED = static_cast<lv_event_code_t>(lv_event_register_id());


static void FileOptionsExit(lv_event_t *e) {
    lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
//    for(int x = 0; x < lv_obj_get_child_cnt(lv_scr_act()); x++){
//        if(lv_obj_get_class(lv_obj_get_child(lv_scr_act(), x)) == &lv_file_list_class){
//            restore_group(lv_obj_get_child(lv_scr_act(), x));
//        }
//    }
}


static void FileOptionsFileSelect(lv_event_t *e) {
    char *current = lv_label_get_text(lv_event_get_target(e));
    lv_obj_t *file_window = file_select("/data/", current);
    lv_obj_add_event_cb(file_window, FileWindowClose, LV_EVENT_DELETE, lv_event_get_target(e));
}

static void FileOptionsRollerDefocus(lv_event_t *e) {
    LV_LOG_USER("Roller Defocused");
    restore_group(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static void FileOptionsMutuallyExclusive(lv_event_t *e){
    LV_LOG_USER("Triggered");
    lv_obj_t *lock_switch = lv_event_get_target(e);
    auto *slideshow_switch = static_cast<lv_obj_t *>(lv_event_get_user_data(e));

    bool locked_state = lv_obj_get_state(lock_switch)&LV_STATE_CHECKED;
    if(locked_state){
        lv_obj_add_state(slideshow_switch, LV_STATE_DISABLED);
        lv_event_send(slideshow_switch, EVENT_SLIDESHOW_DISABLED, nullptr);
    }
    else {
        lv_obj_clear_state(slideshow_switch, LV_STATE_DISABLED);
        lv_event_send(slideshow_switch, EVENT_SLIDESHOW_DISABLED, nullptr);

    }
}

static void FileOptionsDisableSlideshowTime(lv_event_t *e){
    LV_LOG_USER("Triggered");
    lv_obj_t *slideshow_switch = lv_event_get_target(e);
    lv_obj_t *slideshow_btn = lv_obj_get_parent(slideshow_switch);
    auto *slideshow_time_btn = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    bool state = (lv_obj_get_state(slideshow_switch)&LV_STATE_CHECKED && !(lv_obj_get_state(slideshow_btn)&LV_STATE_DISABLED));
    if(state){
        lv_obj_clear_state(slideshow_time_btn, LV_STATE_DISABLED);
    }
    else{
        lv_obj_add_state(slideshow_time_btn, LV_STATE_DISABLED);
    }
}



lv_obj_t * FileOptions() {
    if (lvgl_lock(-1)) {
        LV_LOG_USER("FileOptions");
        new_group();
        lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
        lv_file_list_icon_style(cont_flex, &icon_style);

        lv_obj_t *file_select = lv_file_list_add(cont_flex, "\ue3f4");


        lv_obj_t *btn = lv_label_create(file_select);
        lv_obj_add_style(btn, &menu_font_style, LV_PART_MAIN);
        lv_label_set_text(btn, "/data/");
        lv_label_set_long_mode(btn, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_add_style(btn, &file_select_style, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, FileOptionsFileSelect, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lock_button = lv_file_list_add(cont_flex, "\ue897");
        lv_obj_t *_switch = lv_switch_create(lock_button);
        lv_obj_set_size(_switch, (2 * lv_disp_get_hor_res(NULL)) / 10, (2 * lv_disp_get_hor_res(NULL)) / 17);
        lv_color_t bg = lv_obj_get_style_bg_color(_switch, LV_PART_MAIN);
        lv_obj_set_style_bg_color(_switch, bg, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_group_remove_obj(_switch);


        lv_obj_t *slideshow_button = lv_file_list_add(cont_flex, "\ue41b");
        lv_obj_t *slideshow_switch = lv_switch_create(slideshow_button);
        lv_obj_align(slideshow_switch, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_size(slideshow_switch, (2 * lv_disp_get_hor_res(NULL)) / 10, (2 * lv_disp_get_hor_res(NULL)) / 17);
        lv_obj_set_style_bg_color(slideshow_switch, bg, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_group_remove_obj(slideshow_switch);

        lv_obj_add_event_cb(_switch, FileOptionsMutuallyExclusive, LV_EVENT_VALUE_CHANGED, slideshow_button);

        lv_obj_t *slideshow_time_button = lv_file_list_add(cont_flex, "\ue41b\ue425");
        lv_obj_t *slideshow_time = lv_roller_create(slideshow_time_button);
        //Time must be in 15 second increments, otherwise the logic to convert to seconds needs to be updated
        lv_roller_set_options(slideshow_time, "0:15\n"
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
                                              "3:00", LV_ROLLER_MODE_INFINITE);

        lv_roller_set_visible_row_count(slideshow_time, 2);
        lv_group_t *rollergroup = lv_group_create();
        lv_group_add_obj(rollergroup, slideshow_time);
        lv_obj_add_event_cb(slideshow_time, FileOptionsRollerDefocus, LV_EVENT_VALUE_CHANGED, slideshow_time_button);

        lv_obj_add_event_cb(slideshow_button, FileOptionsDisableSlideshowTime, EVENT_SLIDESHOW_DISABLED, slideshow_time_button);
        lv_obj_add_event_cb(slideshow_switch, FileOptionsDisableSlideshowTime, LV_EVENT_VALUE_CHANGED, slideshow_time_button);
        lv_event_send(slideshow_button, EVENT_SLIDESHOW_DISABLED, nullptr);




        lv_obj_t *save_button = lv_file_list_add(cont_flex, "\ue161");

        lv_obj_t *exit_button = lv_file_list_add(cont_flex, "\ue5c9");
        lv_obj_t *exit = lv_obj_create(exit_button);
        lv_obj_add_flag(exit, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(exit, FileOptionsExit, LV_EVENT_CLICKED, cont_flex);
        lv_group_remove_obj(exit);


        lv_file_list_scroll_to_view(cont_flex, 0);
        lvgl_unlock();
        return cont_flex;
    }
    return nullptr;
}
}