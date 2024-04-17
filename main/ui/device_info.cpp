
#include <esp_app_desc.h>
#include "ui/device_info.h"
#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"
#include "hw_init.h"

static void exit_callback(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

lv_obj_t *device_info() {
  if (lvgl_lock(-1)) {
    new_group();
    lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
    lv_file_list_icon_style(cont_flex, &icon_style);

    lv_obj_t *board_name = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *board_name_label = lv_label_create(board_name);
    lv_obj_add_style(board_name, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(board_name_label, ("Version: " + get_board()->name()).c_str());

    lv_obj_t *serial_number = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *serial_number_label = lv_label_create(serial_number);
    lv_obj_add_style(serial_number_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(serial_number_label, ("Serial: " + std::string("")).c_str());

    const esp_app_desc_t *description = esp_app_get_description();
    lv_obj_t *version = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *version_label = lv_label_create(version);
    lv_obj_add_style(version_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(version_label, (std::string("SW Version: ") + description->version).c_str());

    lv_obj_t *exit_btn = lv_file_list_add(cont_flex, nullptr);
    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(exit_label, "Exit");

    lv_obj_add_event_cb(exit_label, exit_callback, LV_EVENT_CLICKED, cont_flex);

    lv_file_list_scroll_to_view(cont_flex, 0);

    lvgl_unlock();
    return cont_flex;
  }
  return nullptr;
}