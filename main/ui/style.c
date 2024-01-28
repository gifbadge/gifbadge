#include "ui/style.h"

#ifdef __cplusplus
extern "C"
{
#endif

lv_style_t battery_style_normal;
lv_style_t battery_style_empty;
lv_style_t container_style;
lv_style_t icon_style;
lv_style_t file_select_style;
lv_style_t menu_font_style;

void style_init(){
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_color_t btn_text_colour = lv_obj_get_style_text_color(btn, LV_PART_MAIN);
    lv_color_t btn_colour = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    lv_obj_del(btn);
    lv_style_init(&icon_style);
    lv_style_init(&menu_font_style);

    lv_style_set_text_color(&icon_style, btn_text_colour);
    if (lv_disp_get_hor_res(NULL) > 240) {
        lv_style_set_text_font(&icon_style, &material_icons_56);
        lv_style_set_text_font(&menu_font_style, &lv_font_montserrat_28);
    } else {
        lv_style_set_text_font(&icon_style, &material_icons);
        lv_style_set_text_font(&menu_font_style, &lv_font_montserrat_14);
    }

    lv_obj_t *drop = lv_dropdown_create(lv_scr_act());
    lv_style_init(&file_select_style);
    lv_style_set_bg_color(&file_select_style, lv_obj_get_style_bg_color(drop, LV_PART_MAIN));
    lv_style_set_bg_opa(&file_select_style, lv_obj_get_style_bg_opa(drop, LV_PART_MAIN));
    lv_style_set_border_color(&file_select_style, lv_obj_get_style_border_color(drop, LV_PART_MAIN));
    lv_style_set_border_side(&file_select_style, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&file_select_style, lv_obj_get_style_radius(drop, LV_PART_MAIN));
    lv_style_set_pad_all(&file_select_style, lv_obj_get_style_pad_bottom(drop, LV_PART_MAIN));
    lv_obj_del(drop);

    lv_style_init(&battery_style_normal);
    lv_style_set_text_color(&battery_style_normal, btn_text_colour);
    lv_style_set_text_font(&battery_style_normal, &lv_font_montserrat_28);
    lv_style_set_text_align(&battery_style_normal, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&battery_style_empty);
    lv_style_set_text_color(&battery_style_empty, lv_color_hex(0xFF0000));
    lv_style_set_text_font(&battery_style_empty, &lv_font_montserrat_28);
    lv_style_set_text_align(&battery_style_empty, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&container_style);
    lv_style_set_pad_all(&container_style, 0);
    lv_style_set_border_side(&container_style, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_opa(&container_style, 0);

//    lv_style_init(&icon_style);
//    lv_style_set_text_color(&icon_style, lv_color_black());
//    lv_style_set_text_font(&icon_style, &material_icons_56);
}

#ifdef __cplusplus
}
#endif