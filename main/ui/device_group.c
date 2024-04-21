//
// Created by gifbadge on 26/01/24.
//

#include "ui/device_group.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern lv_indev_t *lvgl_encoder;

lv_indev_t *get_encoder() {
//    lv_indev_t *dev;
//    while (true) {
//        LV_LOG_USER("get_encoder_loop");
//        dev = lv_indev_get_next(NULL);
//        if (lv_indev_get_type(dev) == LV_INDEV_TYPE_ENCODER) {
//            LV_LOG_USER("got encoder");
//            return dev;
//        } else if (dev == NULL) {
//            LV_LOG_USER("failed");
//            return NULL;
//        }
//    }
  return lvgl_encoder;
}

lv_group_t *new_group() {
  lv_group_t *g = lv_group_create();
  lv_group_set_default(g);
  lv_indev_set_group(get_encoder(), g);
  return g;
}

lv_group_t *restore_group(lv_obj_t *parent) {
  if(lv_obj_is_valid(parent)) {
    lv_group_t *old_group = lv_group_get_default();
    lv_group_t *g = lv_obj_get_group(parent);
    lv_indev_set_group(get_encoder(), g);
    lv_group_set_default(g);
    lv_group_delete(old_group);
    return g;
  }
  return NULL;
}

#ifdef __cplusplus
}
#endif
