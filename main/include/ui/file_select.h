//
// Created by gifbadge on 26/01/24.
//

#ifndef LVGL_FILE_SELECT_H
#define LVGL_FILE_SELECT_H
#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

void file_list(lv_obj_t *parent);
lv_obj_t *file_select(const char *top, const char *current);
void FileWindowClose(lv_event_t *e);

#ifdef __cplusplus
}
#endif
#endif //LVGL_FILE_SELECT_H
