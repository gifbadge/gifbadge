#include <lvgl.h>

#include "ui/usb_connected.h"
#include "ui/menu.h"
#include "ui/style.h"

void lvgl_usb_connected() {
  if (lvgl_lock(-1)) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_t *label = lv_image_create(cont);
    lv_obj_set_style_text_font(label, &material_icons_special, LV_PART_MAIN);
//    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_image_set_src(label, ICON_USB);
    lv_obj_center(label);
    lvgl_unlock();
  }
}