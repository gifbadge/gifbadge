/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

/**
 * @file lv_battery.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "ui/widgets/battery/lv_battery.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_battery_class

/** hor. pad and ver. pad cannot make the indicator smaller than this [px]*/
#define lv_battery_SIZE_MIN  4


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_battery_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_battery_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_battery_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_indic(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_battery_class = {
    .constructor_cb = lv_battery_constructor,
    .destructor_cb = lv_battery_destructor,
    .event_cb = lv_battery_event,
    .width_def = LV_DPI_DEF * 2,
    .height_def = LV_DPI_DEF / 10,
    .instance_size = sizeof(lv_battery_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_battery_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_battery_set_value(lv_obj_t *obj, int32_t value)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_battery_t * bar = (lv_battery_t *)obj;
    value = bar->max_value-value;
    if(bar->cur_value == value) return;

    value = LV_CLAMP(bar->min_value, value, bar->max_value);
    value = value < bar->start_value ? bar->start_value : value; /*Can't be smaller than the left value*/

    if(bar->cur_value == value) return;
    bar->cur_value = value;
    lv_obj_invalidate((lv_obj_t *)obj);
}

/*=====================
 * Getter functions
 *====================*/

int32_t lv_battery_get_value(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_battery_t * bar = (lv_battery_t *)obj;

    return bar->cur_value;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_battery_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_battery_t * bar = (lv_battery_t *)obj;
    bar->min_value = 0;
    bar->max_value = 100;
    bar->start_value = 0;
    bar->cur_value = 0;
    bar->indic_area.x1 = 0;
    bar->indic_area.x2 = 0;
    bar->indic_area.y1 = 0;
    bar->indic_area.y2 = 0;
    bar->mode = lv_battery_MODE_NORMAL;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_battery_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_battery_t * bar = (lv_battery_t *)obj;

    lv_anim_del(&bar->cur_value_anim, NULL);
    lv_anim_del(&bar->start_value_anim, NULL);
}

//static void draw_indic(lv_event_t * e)
//{
//    lv_obj_t * obj = lv_event_get_target(e);
//    lv_battery_t * bar = (lv_battery_t *)obj;
//
//  lv_layer_t * layer = lv_event_get_layer(e);
//
//    lv_area_t bar_coords;
//    lv_obj_get_coords(obj, &bar_coords);
//
//    lv_coord_t transf_w = lv_obj_get_style_transform_width(obj, LV_PART_MAIN);
//    lv_coord_t transf_h = lv_obj_get_style_transform_height(obj, LV_PART_MAIN);
//    bar_coords.x1 -= transf_w;
//    bar_coords.x2 += transf_w;
//    bar_coords.y1 -= transf_h;
//    bar_coords.y2 += transf_h;
//    lv_coord_t barw = lv_area_get_width(&bar_coords);
//    lv_coord_t barh = lv_area_get_height(&bar_coords);
//    int32_t range = bar->max_value - bar->min_value;
//    bool hor = barw >= barh ? true : false;
//
//    /*Calculate the indicator area*/
//    lv_coord_t bg_left = lv_obj_get_style_pad_left(obj,     LV_PART_MAIN);
//    lv_coord_t bg_right = lv_obj_get_style_pad_right(obj,   LV_PART_MAIN);
//    lv_coord_t bg_top = lv_obj_get_style_pad_top(obj,       LV_PART_MAIN)+(lv_area_get_height(&bar_coords)/10);
//    lv_coord_t bg_bottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN);
//    /*Respect padding and minimum width/height too*/
//    lv_area_copy(&bar->indic_area, &bar_coords);
//    bar->indic_area.x1 += bg_left;
//    bar->indic_area.x2 -= bg_right;
//    bar->indic_area.y1 += bg_top;
//    bar->indic_area.y2 -= bg_bottom;
//
//    if(hor && lv_area_get_height(&bar->indic_area) < lv_battery_SIZE_MIN) {
//        bar->indic_area.y1 = obj->coords.y1 + (barh / 2) - (lv_battery_SIZE_MIN / 2);
//        bar->indic_area.y2 = bar->indic_area.y1 + lv_battery_SIZE_MIN;
//    }
//    else if(!hor && lv_area_get_width(&bar->indic_area) < lv_battery_SIZE_MIN) {
//        bar->indic_area.x1 = obj->coords.x1 + (barw / 2) - (lv_battery_SIZE_MIN / 2);
//        bar->indic_area.x2 = bar->indic_area.x1 + lv_battery_SIZE_MIN;
//    }
//
//    lv_coord_t indicw = lv_area_get_width(&bar->indic_area);
//    lv_coord_t indich = lv_area_get_height(&bar->indic_area);
//
//    /*Calculate the indicator length*/
//    lv_coord_t anim_length = hor ? indicw : indich;
//
//    lv_coord_t anim_cur_value_x, anim_start_value_x;
//
//    lv_coord_t * axis1, * axis2;
//    lv_coord_t (*indic_length_calc)(const lv_area_t * area);
//
//    if(hor) {
//        axis1 = &bar->indic_area.x1;
//        axis2 = &bar->indic_area.x2;
//        indic_length_calc = lv_area_get_width;
//    }
//    else {
//        axis1 = &bar->indic_area.y1;
//        axis2 = &bar->indic_area.y2;
//        indic_length_calc = lv_area_get_height;
//    }
//
//    anim_start_value_x = (int32_t)((int32_t)anim_length * (bar->start_value - bar->min_value)) / range;
//
//    anim_cur_value_x = (int32_t)((int32_t)anim_length * (bar->cur_value - bar->min_value)) / range;
//
//    /*Invert*/
//    lv_coord_t * tmp;
//    tmp = axis1;
//    axis1 = axis2;
//    axis2 = tmp;
//    anim_cur_value_x = -anim_cur_value_x;
//    anim_start_value_x = -anim_start_value_x;
//
//    /*Set the indicator length*/
//    if(hor) {
//        *axis2 = *axis1 + anim_cur_value_x;
//        *axis1 += anim_start_value_x;
//    }
//    else {
//        *axis1 = *axis2 - anim_cur_value_x + 1;
//        *axis2 -= anim_start_value_x;
//    }
//
//  {
//    lv_area_t left_rectangle;
//    lv_area_copy(&left_rectangle, &bar_coords);
//    left_rectangle.x2 = left_rectangle.x1+(lv_area_get_width(&bar_coords)/4);
//    left_rectangle.y2 = left_rectangle.y1+(lv_area_get_height(&bar_coords)/10);
//
//    lv_draw_rect_dsc_t draw_rect_dsc;
//    lv_draw_rect_dsc_init(&draw_rect_dsc);
//    lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);
//
//    lv_draw_rect(layer, &draw_rect_dsc, &left_rectangle);
//
//  }
//
//  {
//    lv_area_t right_rectangle;
//    lv_area_copy(&right_rectangle, &bar_coords);
//    right_rectangle.x1 = right_rectangle.x2-(lv_area_get_width(&bar_coords)/4);
//    right_rectangle.y2 = right_rectangle.y1+(lv_area_get_height(&bar_coords)/10);
//
//    lv_draw_rect_dsc_t draw_rect_dsc;
//    lv_draw_rect_dsc_init(&draw_rect_dsc);
//    lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);
//
//    lv_draw_rect(layer, &draw_rect_dsc, &right_rectangle);
//
//  }
//
//    /*Do not draw a zero length indicator but at least call the draw part events*/
//    if(indic_length_calc(&bar->indic_area)-1 <= 1) {
//
//        lv_obj_draw_part_dsc_t part_draw_dsc;
//        lv_obj_draw_dsc_init(&part_draw_dsc, layer);
//        part_draw_dsc.part = LV_PART_INDICATOR;
//        part_draw_dsc.class_p = MY_CLASS;
//        part_draw_dsc.type = lv_battery_DRAW_PART_INDICATOR;
//        part_draw_dsc.draw_area = &bar->indic_area;
//
//        lv_event_send(obj, LV_EVENT_DRAW_PART_BEGIN, &part_draw_dsc);
//        lv_event_send(obj, LV_EVENT_DRAW_PART_END, &part_draw_dsc);
//        return;
//    }
//
//    lv_area_t indic_area;
//    lv_area_copy(&indic_area, &bar->indic_area);
//
//    lv_draw_rect_dsc_t draw_rect_dsc;
//    lv_draw_rect_dsc_init(&draw_rect_dsc);
//    lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);
//
//    lv_obj_draw_part_dsc_t part_draw_dsc;
//    lv_obj_draw_dsc_init(&part_draw_dsc, draw_ctx);
//    part_draw_dsc.part = LV_PART_INDICATOR;
//    part_draw_dsc.class_p = MY_CLASS;
//    part_draw_dsc.type = lv_battery_DRAW_PART_INDICATOR;
//    part_draw_dsc.rect_dsc = &draw_rect_dsc;
//    part_draw_dsc.draw_area = &bar->indic_area;
//
//    lv_event_send(obj, LV_EVENT_DRAW_PART_BEGIN, &part_draw_dsc);
//
//    lv_coord_t bg_radius = lv_obj_get_style_radius(obj, LV_PART_MAIN);
//
//#if LV_DRAW_COMPLEX
//    lv_draw_mask_radius_param_t mask_bg_param;
//    lv_area_t bg_mask_area;
//    bg_mask_area.x1 = obj->coords.x1 + bg_left;
//    bg_mask_area.x2 = obj->coords.x2 - bg_right;
//    bg_mask_area.y1 = obj->coords.y1 + bg_top;
//    bg_mask_area.y2 = obj->coords.y2 - bg_bottom;
//
//    lv_draw_mask_radius_init(&mask_bg_param, &bg_mask_area, bg_radius, false);
//    lv_coord_t mask_bg_id = lv_draw_mask_add(&mask_bg_param, NULL);
//#endif
//
//    /*Draw_only the background and background image*/
//    lv_opa_t shadow_opa = draw_rect_dsc.shadow_opa;
//    lv_opa_t border_opa = draw_rect_dsc.border_opa;
//    draw_rect_dsc.border_opa = LV_OPA_TRANSP;
//    draw_rect_dsc.shadow_opa = LV_OPA_TRANSP;
//
//    /*Get the max possible indicator area. The gradient should be applied on this*/
//    lv_area_t mask_indic_max_area;
//    lv_area_copy(&mask_indic_max_area, &bar_coords);
//    mask_indic_max_area.x1 += bg_left;
//    mask_indic_max_area.y1 += bg_top;
//    mask_indic_max_area.x2 -= bg_right;
//    mask_indic_max_area.y2 -= bg_bottom;
//    if(hor && lv_area_get_height(&mask_indic_max_area) < lv_battery_SIZE_MIN) {
//        mask_indic_max_area.y1 = obj->coords.y1 + (barh / 2) - (lv_battery_SIZE_MIN / 2);
//        mask_indic_max_area.y2 = mask_indic_max_area.y1 + lv_battery_SIZE_MIN;
//    }
//    else if(!hor && lv_area_get_width(&mask_indic_max_area) < lv_battery_SIZE_MIN) {
//        mask_indic_max_area.x1 = obj->coords.x1 + (barw / 2) - (lv_battery_SIZE_MIN / 2);
//        mask_indic_max_area.x2 = mask_indic_max_area.x1 + lv_battery_SIZE_MIN;
//    }
//
//#if LV_DRAW_COMPLEX
//    /*Create a mask to the current indicator area to see only this part from the whole gradient.*/
//    lv_draw_mask_radius_param_t mask_indic_param;
//    lv_draw_mask_radius_init(&mask_indic_param, &bar->indic_area, draw_rect_dsc.radius, false);
//    int16_t mask_indic_id = lv_draw_mask_add(&mask_indic_param, NULL);
//#endif
//
//    lv_draw_rect(layer, &draw_rect_dsc, &mask_indic_max_area);
//    draw_rect_dsc.border_opa = border_opa;
//    draw_rect_dsc.shadow_opa = shadow_opa;
//
//    /*Draw the border*/
//    draw_rect_dsc.bg_opa = LV_OPA_TRANSP;
//    draw_rect_dsc.bg_img_opa = LV_OPA_TRANSP;
//    draw_rect_dsc.shadow_opa = LV_OPA_TRANSP;
//    lv_draw_rect(layer, &draw_rect_dsc, &bar->indic_area);
//
//#if LV_DRAW_COMPLEX
//    lv_draw_mask_free_param(&mask_indic_param);
//    lv_draw_mask_free_param(&mask_bg_param);
//    lv_draw_mask_remove_id(mask_indic_id);
//    lv_draw_mask_remove_id(mask_bg_id);
//#endif
//
//    lv_event_send(obj, LV_EVENT_DRAW_PART_END, &part_draw_dsc);
//}


static void draw_indic(lv_event_t * e)
{
  lv_obj_t * obj = lv_event_get_current_target(e);
  lv_bar_t * bar = (lv_bar_t *)obj;

  lv_layer_t * layer = lv_event_get_layer(e);

  lv_area_t bar_coords;
  lv_obj_get_coords(obj, &bar_coords);

  int32_t transf_w = lv_obj_get_style_transform_width(obj, LV_PART_MAIN);
  int32_t transf_h = lv_obj_get_style_transform_height(obj, LV_PART_MAIN);
  lv_area_increase(&bar_coords, transf_w, transf_h);
  int32_t barw = lv_area_get_width(&bar_coords);
  int32_t barh = lv_area_get_height(&bar_coords);
  int32_t range = bar->max_value - bar->min_value;

  /*Prevent division by 0*/
  if(range == 0) {
    range = 1;
  }

  bool hor = barw >= barh;

  /*Calculate the indicator area*/
  int32_t bg_left = lv_obj_get_style_pad_left(obj,     LV_PART_MAIN);
  int32_t bg_right = lv_obj_get_style_pad_right(obj,   LV_PART_MAIN);
  int32_t bg_top = lv_obj_get_style_pad_top(obj,       LV_PART_MAIN);
  int32_t bg_bottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN);

  /*Respect padding and minimum width/height too*/
  lv_area_copy(&bar->indic_area, &bar_coords);
  bar->indic_area.x1 += bg_left;
  bar->indic_area.x2 -= bg_right;
  bar->indic_area.y1 += bg_top+(lv_area_get_height(&bar_coords)/10);
  bar->indic_area.y2 -= bg_bottom;

  if(hor && lv_area_get_height(&bar->indic_area) < lv_battery_SIZE_MIN) {
    bar->indic_area.y1 = obj->coords.y1 + (barh / 2) - (lv_battery_SIZE_MIN / 2);
    bar->indic_area.y2 = bar->indic_area.y1 + lv_battery_SIZE_MIN;
  }
  else if(!hor && lv_area_get_width(&bar->indic_area) < lv_battery_SIZE_MIN) {
    bar->indic_area.x1 = obj->coords.x1 + (barw / 2) - (lv_battery_SIZE_MIN / 2);
    bar->indic_area.x2 = bar->indic_area.x1 + lv_battery_SIZE_MIN;
  }

  int32_t indic_max_w = lv_area_get_width(&bar->indic_area);
  int32_t indic_max_h = lv_area_get_height(&bar->indic_area);

  /*Calculate the indicator length*/
  int32_t anim_length = hor ? indic_max_w : indic_max_h;

  int32_t anim_cur_value_x, anim_start_value_x;

  int32_t * axis1, * axis2;
  int32_t (*indic_length_calc)(const lv_area_t * area);

  if(hor) {
    axis1 = &bar->indic_area.x1;
    axis2 = &bar->indic_area.x2;
    indic_length_calc = lv_area_get_width;
  }
  else {
    axis1 = &bar->indic_area.y1;
    axis2 = &bar->indic_area.y2;
    indic_length_calc = lv_area_get_height;
  }

  anim_start_value_x = (int32_t)((int32_t)anim_length * (bar->start_value - bar->min_value)) / range;


  anim_cur_value_x = (int32_t)((int32_t)anim_length * (bar->cur_value - bar->min_value)) / range;


  /*Swap axes*/
  int32_t * tmp;
  tmp = axis1;
  axis1 = axis2;
  axis2 = tmp;
  anim_cur_value_x = -anim_cur_value_x;
  anim_start_value_x = -anim_start_value_x;


  /*Set the indicator length*/
  if(hor) {
    *axis2 = *axis1 + anim_cur_value_x;
    *axis1 += anim_start_value_x;
  }
  else {
    *axis1 = *axis2 - anim_cur_value_x + 1;
    *axis2 -= anim_start_value_x;
  }

  {
    lv_area_t left_rectangle;
    lv_area_copy(&left_rectangle, &bar_coords);
    left_rectangle.x2 = left_rectangle.x1+(lv_area_get_width(&bar_coords)/4);
    left_rectangle.y2 = left_rectangle.y1+(lv_area_get_height(&bar_coords)/10);

    lv_draw_rect_dsc_t draw_rect_dsc;
    lv_draw_rect_dsc_init(&draw_rect_dsc);
    lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);

    lv_draw_rect(layer, &draw_rect_dsc, &left_rectangle);

  }

  {
    lv_area_t right_rectangle;
    lv_area_copy(&right_rectangle, &bar_coords);
    right_rectangle.x1 = right_rectangle.x2-(lv_area_get_width(&bar_coords)/4);
    right_rectangle.y2 = right_rectangle.y1+(lv_area_get_height(&bar_coords)/10);

    lv_draw_rect_dsc_t draw_rect_dsc;
    lv_draw_rect_dsc_init(&draw_rect_dsc);
    lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);

    lv_draw_rect(layer, &draw_rect_dsc, &right_rectangle);

  }

  /*Do not draw a zero length indicator but at least call the draw task event*/
  if(indic_length_calc(&bar->indic_area)-1 <= 1) {
    lv_obj_send_event(obj, LV_EVENT_DRAW_TASK_ADDED, NULL);
    return;
  }

  lv_area_t indic_area;
  lv_area_copy(&indic_area, &bar->indic_area);

  lv_draw_rect_dsc_t draw_rect_dsc;
  lv_draw_rect_dsc_init(&draw_rect_dsc);
  lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_rect_dsc);

  int32_t bg_radius = lv_obj_get_style_radius(obj, LV_PART_MAIN);
  int32_t short_side = LV_MIN(barw, barh);
  if(bg_radius > short_side >> 1) bg_radius = short_side >> 1;

  int32_t indic_radius = draw_rect_dsc.radius;
  short_side = LV_MIN(lv_area_get_width(&bar->indic_area), lv_area_get_height(&bar->indic_area));
  if(indic_radius > short_side >> 1) indic_radius = short_side >> 1;

  /*Cases:
   * Simple:
   *   - indicator area is the same or smaller then the bg
   *   - indicator has the same or larger radius than the bg
   *   - what to do? just draw the indicator
   * Radius issue:
   *   - indicator area is the same or smaller then bg
   *   - indicator has smaller radius than the bg and the indicator overflows on the corners
   *   - what to do? draw the indicator on a layer and clip to bg radius
   * Larger indicator:
   *   - indicator area is the larger then the bg
   *   - radius doesn't matter
   *   - shadow doesn't matter
   *   - what to do? just draw the indicator
   * Shadow:
   *   - indicator area is the same or smaller then the bg
   *   - indicator has the same or larger radius than the bg (shadow needs to be drawn on strange clipped shape)
   *   - what to do? don't draw the shadow if the indicator is too small has strange shape
   * Gradient:
   *   - the indicator has a gradient
   *   - what to do? draw it on a bg sized layer clip the indicator are from the gradient
   *
   */

  bool mask_needed = false;
  if(hor && draw_rect_dsc.bg_grad.dir == LV_GRAD_DIR_HOR) mask_needed = true;
  else if(!hor && draw_rect_dsc.bg_grad.dir == LV_GRAD_DIR_VER) mask_needed = true;

  if(draw_rect_dsc.bg_image_src) mask_needed = true;

  bool radius_issue = true;
  /*The indicator is fully drawn if it's larger than the bg*/
  if((bg_left < 0 || bg_right < 0 || bg_top < 0 || bg_bottom < 0)) radius_issue = false;
  else if(indic_radius >= bg_radius) radius_issue = false;
  else if(_lv_area_is_in(&indic_area, &bar_coords, bg_radius)) radius_issue = false;

  if(radius_issue || mask_needed) {
    if(!radius_issue) {
      /*Draw only the shadow*/
      lv_draw_rect_dsc_t draw_tmp_dsc = draw_rect_dsc;
      draw_tmp_dsc.border_opa = 0;
      draw_tmp_dsc.outline_opa = 0;
      draw_tmp_dsc.bg_opa = 0;
      draw_tmp_dsc.bg_image_opa = 0;
      lv_draw_rect(layer, &draw_tmp_dsc, &indic_area);
    }
    else {
      draw_rect_dsc.border_opa = 0;
      draw_rect_dsc.outline_opa = 0;
    }
    draw_rect_dsc.shadow_opa = 0;

    /*If clipped for any reason cannot the border, outline, and shadow
     *as they would be clipped and looked ugly*/
    lv_draw_rect_dsc_t draw_tmp_dsc = draw_rect_dsc;
    draw_tmp_dsc.border_opa = 0;
    draw_tmp_dsc.outline_opa = 0;
    draw_tmp_dsc.shadow_opa = 0;
    lv_area_t indic_draw_area = indic_area;
    if(mask_needed) {
      if(hor) {
        indic_draw_area.x1 = bar_coords.x1 + bg_left;
        indic_draw_area.x2 = bar_coords.x2 - bg_right;
      }
      else {
        indic_draw_area.y1 = bar_coords.y1 + bg_top;
        indic_draw_area.y2 = bar_coords.y2 - bg_bottom;
      }
      draw_tmp_dsc.radius = 0;
    }

    lv_layer_t * layer_indic = lv_draw_layer_create(layer, LV_COLOR_FORMAT_ARGB8888, &indic_draw_area);

    lv_draw_rect(layer_indic, &draw_tmp_dsc, &indic_draw_area);

    lv_draw_mask_rect_dsc_t mask_dsc;
    lv_draw_mask_rect_dsc_init(&mask_dsc);
    if(radius_issue) {
      mask_dsc.area = bar_coords;
      mask_dsc.radius = bg_radius;
      lv_draw_mask_rect(layer_indic, &mask_dsc);
    }

    if(mask_needed) {
      mask_dsc.area = indic_area;
      mask_dsc.radius = indic_radius;
      lv_draw_mask_rect(layer_indic, &mask_dsc);
    }

    lv_draw_image_dsc_t layer_draw_dsc;
    lv_draw_image_dsc_init(&layer_draw_dsc);
    layer_draw_dsc.src = layer_indic;
    lv_draw_layer(layer, &layer_draw_dsc, &indic_draw_area);

    /*Add the border, outline, and shadow only to the indicator area.
     *They might have disabled if there is a radius_issue*/
    draw_tmp_dsc = draw_rect_dsc;
    draw_tmp_dsc.bg_opa = 0;
    draw_tmp_dsc.bg_image_opa = 0;
    lv_draw_rect(layer, &draw_tmp_dsc, &indic_area);
  }
  else {
    lv_draw_rect(layer, &draw_rect_dsc, &indic_area);
  }
}

static void lv_battery_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_coord_t indic_size;
        indic_size = lv_obj_calculate_ext_draw_size(obj, LV_PART_INDICATOR);

        /*Bg size is handled by lv_obj*/
        lv_coord_t * s = lv_event_get_param(e);
        *s = LV_MAX(*s, indic_size);

        /*Calculate the indicator area*/
        lv_coord_t bg_left = lv_obj_get_style_pad_left(obj, LV_PART_MAIN);
        lv_coord_t bg_right = lv_obj_get_style_pad_right(obj, LV_PART_MAIN);
        lv_coord_t bg_top = lv_obj_get_style_pad_top(obj, LV_PART_MAIN);
        lv_coord_t bg_bottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN);

        lv_coord_t pad = LV_MIN4(bg_left, bg_right, bg_top, bg_bottom);
        if(pad < 0) {
            *s = LV_MAX(*s, -pad);
        }
    }
    else if(code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        lv_battery_t * bar = (lv_battery_t *)obj;
        lv_obj_invalidate_area(obj, &bar->indic_area);
    }
    else if(code == LV_EVENT_DRAW_MAIN) {
        draw_indic(e);
    }
}