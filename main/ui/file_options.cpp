#include <lvgl.h>

#include "ui/file_options.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/file_select.h"
#include "ui/device_group.h"
#include "ui/menu.h"
#include "ui/style.h"
#include "display.h"

static const char *TAG = "file_options";

extern "C" {

lv_event_code_t EVENT_SLIDESHOW_DISABLED = static_cast<lv_event_code_t>(lv_event_register_id());

#define SLIDESHOW_TIME_INCREMENT 15

static void FileOptionsExit(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

struct FileOptionFields {
  lv_obj_t *container;
  lv_obj_t *file_select;
  lv_obj_t *lock_state;
  lv_obj_t *slideshow_state;
  lv_obj_t *slideshow_time;
};

static void FileOptionsSave(lv_event_t *e) {
  auto fields = static_cast<FileOptionFields *>(lv_event_get_user_data(e));
  auto config = ImageConfig();

  char *path = lv_label_get_text(fields->file_select);
  ESP_LOGI(TAG, "File: %s", path);
  config.setPath(path);

  bool locked = (lv_obj_get_state(fields->lock_state) & LV_STATE_CHECKED);
  ESP_LOGI(TAG, "Locked: %s", locked ? "True" : "False");
  config.setLocked(locked);

  bool slideshow = lv_obj_get_state(fields->slideshow_state) & LV_STATE_CHECKED;
  ESP_LOGI(TAG, "Slideshow: %s", slideshow ? "True" : "False");
  config.setSlideShow(slideshow);

  int slideshow_time = static_cast<int>((lv_slider_get_value(fields->slideshow_time)) * SLIDESHOW_TIME_INCREMENT);
  ESP_LOGI(TAG, "Slideshow Time: %ds", slideshow_time);
  config.setSlideShowTime(slideshow_time);

  config.save();

  TaskHandle_t handle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(handle, 0, DISPLAY_NOTIFY_CHANGE, eSetValueWithOverwrite);


  lv_obj_del(fields->container);
  free(fields);
}

static void FileOptionsFileSelect(lv_event_t *e) {
  char *current = lv_label_get_text(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  lv_obj_t *file_window = file_select("/data/", current);
  lv_obj_add_event_cb(file_window, FileWindowClose, LV_EVENT_DELETE, lv_event_get_target(e));
}

static void FileOptionsMutuallyExclusive(lv_event_t *e) {
  auto *lock_switch = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto *slideshow_btn = static_cast<lv_obj_t *>(lv_event_get_user_data(e));

  bool locked_state = lv_obj_get_state(lock_switch) & LV_STATE_CHECKED;

  if (locked_state) {
    lv_obj_add_state(slideshow_btn, LV_STATE_DISABLED);
    lv_obj_send_event(slideshow_btn, EVENT_SLIDESHOW_DISABLED, nullptr);
  } else {
    lv_obj_clear_state(slideshow_btn, LV_STATE_DISABLED);
    lv_obj_send_event(slideshow_btn, EVENT_SLIDESHOW_DISABLED, nullptr);
  }
}

static void FileOptionsDisableSlideshowTime(lv_event_t *e) {
  auto *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_obj_t *slideshow_btn;
  lv_obj_t *slideshow_switch;
  if (lv_event_get_code(e) == EVENT_SLIDESHOW_DISABLED) {
    slideshow_btn = target;
    slideshow_switch = lv_obj_get_child(slideshow_btn, 1);
  } else {
    slideshow_btn = lv_obj_get_parent(target);
    slideshow_switch = target;
  }
  auto *slideshow_time_btn = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
  bool state =
      (lv_obj_get_state(slideshow_switch) & LV_STATE_CHECKED && !(lv_obj_get_state(slideshow_btn) & LV_STATE_DISABLED));
  if (state) {
    lv_obj_clear_state(slideshow_time_btn, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(slideshow_time_btn, LV_STATE_DISABLED);
  }
}

static void SlideShowTimeSliderExitCallback(lv_event_t *e) {
  restore_group(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static void BacklightSliderChangedCallback(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target_obj(e);
  int level = lv_slider_get_value(obj) * SLIDESHOW_TIME_INCREMENT;
  lv_label_set_text_fmt(static_cast<lv_obj_t *>(lv_event_get_user_data(e)), "%d:%02d", level / 60, level % 60);

}

lv_obj_t *FileOptions() {
  if (lvgl_lock(-1)) {
    LV_LOG_USER("FileOptions");

    auto config = ImageConfig();

    new_group();
    lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
    lv_file_list_icon_style(cont_flex, &icon_style);

    //File Select
    lv_obj_t *file_select = lv_file_list_add(cont_flex, "\ue3f4");

    lv_obj_t *file_label = lv_label_create(file_select);
    lv_obj_add_style(file_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(file_label, config.getPath());
    lv_label_set_long_mode(file_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(file_label, 1);
    lv_obj_add_style(file_label, &file_select_style, LV_PART_MAIN);

    //Lock
    lv_obj_t *lock_button = lv_file_list_add(cont_flex, "\ue897");
    lv_obj_t *lock_switch = lv_switch_create(lock_button);
    if (config.getLocked()) {
      lv_obj_add_state(lock_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_size(lock_switch, (2 * lv_disp_get_hor_res(nullptr)) / 10, (2 * lv_disp_get_hor_res(nullptr)) / 17);
    lv_color_t bg = lv_obj_get_style_bg_color(lock_switch, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lock_switch, bg, static_cast<lv_style_selector_t>(LV_PART_INDICATOR) | LV_STATE_CHECKED);
    lv_group_remove_obj(lock_switch);

    //Slideshow
    lv_obj_t *slideshow_button = lv_file_list_add(cont_flex, "\ue41b");
    lv_obj_t *slideshow_switch = lv_switch_create(slideshow_button);
    if (config.getSlideShow()) {
      lv_obj_add_state(slideshow_switch, LV_STATE_CHECKED);
    }
    lv_obj_align(slideshow_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(slideshow_switch, (2 * lv_disp_get_hor_res(nullptr)) / 10, (2 * lv_disp_get_hor_res(nullptr)) / 17);
    lv_obj_set_style_bg_color(slideshow_switch,
                              bg,
                              static_cast<lv_style_selector_t>(LV_PART_INDICATOR) | LV_STATE_CHECKED);
    lv_group_remove_obj(slideshow_switch);

    //Slideshow Time
    lv_obj_t *slideshow_time_button = lv_file_list_add(cont_flex, "\ue41b\ue425");
    lv_obj_t *slideshow_time = lv_slider_create(slideshow_time_button);
    lv_obj_set_flex_grow(slideshow_time, 1);
//        lv_obj_set_style_margin_hor(slideshow_time, 10, LV_PART_MAIN);
//        lv_obj_set_style_pad_hor(slideshow_time, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(slideshow_time_button, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slideshow_time, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_bg_color(slideshow_time, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slideshow_time, lv_color_black(), LV_PART_MAIN);
    lv_slider_set_range(slideshow_time, 1, 12);
    lv_group_t *slider_group = lv_group_create();
    lv_group_add_obj(slider_group, slideshow_time);
    lv_slider_set_value(slideshow_time, config.getSlideShowTime() / 15, LV_ANIM_OFF);
    lv_obj_t *slideshow_time_label = lv_label_create(slideshow_time_button);
    lv_obj_add_flag(slideshow_time_label, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_label_set_text_fmt(slideshow_time_label,
                          "%d:%02d",
                          config.getSlideShowTime() / 60,
                          (config.getSlideShowTime() % 60));

    //Save
    lv_obj_t *save_button = lv_file_list_add(cont_flex, "\ue161");
    lv_obj_t *save = lv_obj_create(save_button);
    lv_obj_add_flag(save, LV_OBJ_FLAG_HIDDEN);
    lv_group_remove_obj(save);

    //Exit
    lv_obj_t *exit_button = lv_file_list_add(cont_flex, "\ue5c9");
    lv_obj_t *exit = lv_obj_create(exit_button);
    lv_obj_add_flag(exit, LV_OBJ_FLAG_HIDDEN);
    lv_group_remove_obj(exit);

    //Fields
    auto *fields = static_cast<FileOptionFields *>(malloc(sizeof(FileOptionFields)));
    fields->container = cont_flex;
    fields->file_select = file_label;
    fields->lock_state = lock_switch;
    fields->slideshow_state = slideshow_switch;
    fields->slideshow_time = slideshow_time;

    //Callbacks
    lv_obj_add_event_cb(file_label, FileOptionsFileSelect, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(lock_switch, FileOptionsMutuallyExclusive, LV_EVENT_VALUE_CHANGED, slideshow_button);
    lv_obj_add_event_cb(slideshow_button,
                        FileOptionsDisableSlideshowTime,
                        EVENT_SLIDESHOW_DISABLED,
                        slideshow_time_button);
    lv_obj_add_event_cb(slideshow_switch,
                        FileOptionsDisableSlideshowTime,
                        LV_EVENT_VALUE_CHANGED,
                        slideshow_time_button);
    lv_obj_add_event_cb(slideshow_time, BacklightSliderChangedCallback, LV_EVENT_VALUE_CHANGED, slideshow_time_label);
    lv_obj_add_event_cb(slideshow_time, SlideShowTimeSliderExitCallback, LV_EVENT_RELEASED, slideshow_time_button);
    lv_obj_add_event_cb(save, FileOptionsSave, LV_EVENT_CLICKED, fields);
    lv_obj_add_event_cb(exit, FileOptionsExit, LV_EVENT_CLICKED, cont_flex);


    //Set the initial state
    lv_obj_send_event(lock_switch, LV_EVENT_VALUE_CHANGED, slideshow_button);
    lv_obj_send_event(slideshow_button, EVENT_SLIDESHOW_DISABLED, nullptr);
    lv_file_list_scroll_to_view(cont_flex, 0);

    lvgl_unlock();
    return cont_flex;
  }
  return nullptr;
}
}