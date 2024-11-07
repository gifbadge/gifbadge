#ifndef LVGL_DEVICE_GROUP_H
#define LVGL_DEVICE_GROUP_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <lvgl.h>

lv_indev_t *get_encoder();

lv_group_t *new_group();

lv_group_t *restore_group(lv_obj_t *);

#ifdef __cplusplus
}
#endif
#endif //LVGL_DEVICE_GROUP_H
