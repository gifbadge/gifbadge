#include "ui/device_info.h"
#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"
#include "hw_init.h"

static void exit_callback(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

lv_obj_t *refresh_widget(lv_obj_t *parent, void (*f) (lv_event_t *e)){
  lv_obj_t *widget = lv_file_list_add(parent, nullptr);
  lv_obj_t *widget_label = lv_label_create(widget);
  lv_obj_add_style(widget_label, &menu_font_style, LV_PART_MAIN);
  lv_obj_add_event_cb(widget, f, LV_EVENT_REFRESH, nullptr);
  lv_obj_send_event(widget, LV_EVENT_REFRESH, nullptr);
  return widget;
}

lv_obj_t *device_info() {
  if (lvgl_lock(-1)) {
    new_group();
    lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
    lv_file_list_icon_style(cont_flex, &icon_style);

    lv_obj_t *board_name = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *board_name_label = lv_label_create(board_name);
    lv_obj_add_style(board_name, &menu_font_style, LV_PART_MAIN);
    char tmpStr[50];
    snprintf(tmpStr, sizeof(tmpStr), "Version: %s", get_board()->name());
    lv_label_set_text(board_name_label, tmpStr);

    lv_obj_t *serial_number = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *serial_number_label = lv_label_create(serial_number);
    lv_obj_add_style(serial_number_label, &menu_font_style, LV_PART_MAIN);
    snprintf(tmpStr, sizeof(tmpStr), "Serial: %s", get_board()->serialNumber());
    lv_label_set_text(serial_number_label, tmpStr);

    lv_obj_t *version = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *version_label = lv_label_create(version);
    lv_obj_add_style(version_label, &menu_font_style, LV_PART_MAIN);
    snprintf(tmpStr, sizeof(tmpStr), "SW Version: %s", get_board()->swVersion());
    lv_label_set_text(version_label, tmpStr);

    if(get_board()->getVbus() != nullptr){
      refresh_widget(cont_flex, [](lv_event_t *e){
        lv_obj_t *widget = lv_event_get_target_obj(e);
        lv_obj_t *label = lv_obj_get_child(widget, 0);
        char tmp_str[50];
        snprintf(tmp_str, sizeof(tmp_str), "USB Current Limit: %imA", get_board()->getVbus()->VbusMaxCurrentGet());
        lv_label_set_text(label, tmp_str);
      });
    }

    lv_obj_t *exit_btn = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(exit_label, "Exit");

    lv_obj_add_event_cb(exit_label, exit_callback, LV_EVENT_CLICKED, cont_flex);

    lv_obj_add_event_cb(cont_flex, [](lv_event_t *e){
      auto child_count = lv_obj_get_child_count(lv_event_get_target_obj(e));
      for(int i = 0; i < child_count; i++) {
        lv_obj_t * child = lv_obj_get_child(lv_event_get_target_obj(e), i);
        lv_obj_send_event(child, LV_EVENT_REFRESH, nullptr);
      }
    }, LV_EVENT_REFRESH, nullptr);

    lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
      auto *obj = (lv_obj_t *) timer->user_data;
      lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
    }, 5000, cont_flex);
    lv_obj_add_event_cb(cont_flex, [](lv_event_t *e) {
      auto *timer = (lv_timer_t *) e->user_data;
      lv_timer_delete(timer);
    }, LV_EVENT_DELETE, timer);

    lv_file_list_scroll_to_view(cont_flex, 0);

    lvgl_unlock();
    return cont_flex;
  }
  return nullptr;
}