#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <lvgl.h>
LV_FONT_DECLARE(material_icons)
LV_FONT_DECLARE(material_icons_56)

#define ICON_FOLDER "\ue2c7"
#define ICON_IMAGE "\ue3f4"
#define ICON_FOLDER_OPEN "\ue2c8"
#define ICON_BACK "\ue166"
#define ICON_UP "\ue5d8"


extern lv_style_t battery_style_normal;
extern lv_style_t battery_style_empty;
extern lv_style_t container_style;
extern lv_style_t icon_style;
extern lv_style_t file_select_style;
extern lv_style_t menu_font_style;


void style_init();

#ifdef __cplusplus
}
#endif