#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <lvgl.h>
LV_FONT_DECLARE(material_icons)
LV_FONT_DECLARE(material_icons_56)
LV_FONT_DECLARE(material_icons_special)
LV_FONT_DECLARE(battery_symbols_14)


#define ICON_FOLDER "\ue2c7"
#define ICON_IMAGE "\ue3f4"
#define ICON_FOLDER_OPEN "\ue2c8"
#define ICON_BACK "\ue166"
#define ICON_UP "\ue5d8"
#define ICON_CHARGING "\uea0b"
#define ICON_CONNECTED "\uefe6"

#define ICON_BATTERY_ALERT "\ue19c"
#define ICON_BATTERY_REMOVED "\uf7ea"
#define ICON_BATTERY_CHARGING_0 "\ue1a3"
#define ICON_BATTERY_CHARGING_30 "\uf0a3"
#define ICON_BATTERY_CHARGING_50 "\uf0a4"
#define ICON_BATTERY_CHARGING_80 "\uf0a6"

#define ICON_BATTERY_0 "\uebdc"
#define ICON_BATTERY_30 "\uebe0"
#define ICON_BATTERY_50 "\uebe2"
#define ICON_BATTERY_80 "\uebd2"
#define ICON_BATTERY_100 "\ue1a4"

#define ICON_USB "\ue1e0"

extern lv_style_t battery_style_normal;
extern lv_style_t battery_style_empty;
extern lv_style_t container_style;
extern lv_style_t icon_style;
extern lv_style_t file_select_style;
extern lv_style_t menu_font_style;
extern lv_style_t style_battery_indicator;
extern lv_style_t style_battery_main;
extern lv_style_t style_battery_icon;

void style_init();

#ifdef __cplusplus
}
#endif