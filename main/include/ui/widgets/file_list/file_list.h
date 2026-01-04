/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#ifndef LVGL_FILE_LIST_H
#define LVGL_FILE_LIST_H

#include "lvgl.h"
// #include "lvgl_private.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*Data of canvas*/
typedef struct {
  lv_obj_t obj;
  lv_obj_t *cont;
  lv_obj_t *head_area;
  lv_obj_t *browser_area;
  lv_obj_t *file_table;
  lv_obj_t *path_label;
  const char *sel_fn;
  void *user_data;
  lv_style_t *icon_style;
} lv_file_list_t;

extern const lv_obj_class_t lv_file_list_class;

lv_obj_t *lv_file_list_create(lv_obj_t *parent);

lv_obj_t *lv_file_list_add(lv_obj_t *obj, const char *icon);

void lv_file_list_icon_style(lv_obj_t *obj, lv_style_t *style);

void lv_file_list_scroll_to_view(lv_obj_t *obj, int i);

void lv_file_list_clear(lv_obj_t *obj);

void lv_file_list_set_user_data(lv_obj_t *obj, void *user_data);

void *lv_file_list_get_user_data(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif //LVGL_FILE_LIST_H
