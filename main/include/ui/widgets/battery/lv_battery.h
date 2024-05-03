/**
 * @file lv_battery.h
 *
 */

#ifndef lv_battery_H
#define lv_battery_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

enum {
    lv_battery_MODE_NORMAL,
    lv_battery_MODE_SYMMETRICAL,
    lv_battery_MODE_RANGE
};
typedef uint8_t lv_battery_mode_t;

typedef struct {
    lv_obj_t * bar;
    int32_t anim_start;
    int32_t anim_end;
    int32_t anim_state;
} _lv_battery_anim_t;

typedef struct {
    lv_obj_t obj;
    int32_t cur_value;          /**< Current value of the bar*/
    int32_t min_value;          /**< Minimum value of the bar*/
    int32_t max_value;          /**< Maximum value of the bar*/
    int32_t start_value;        /**< Start value of the bar*/
    lv_area_t indic_area;       /**< Save the indicator area. Might be used by derived types*/
    _lv_battery_anim_t cur_value_anim;
    _lv_battery_anim_t start_value_anim;
    lv_battery_mode_t mode : 2;     /**< Type of bar*/
} lv_battery_t;

extern const lv_obj_class_t lv_battery_class;

/**
 * `type` field in `lv_obj_draw_part_dsc_t` if `class_p = lv_battery_class`
 * Used in `LV_EVENT_DRAW_PART_BEGIN` and `LV_EVENT_DRAW_PART_END`
 */
typedef enum {
    lv_battery_DRAW_PART_INDICATOR,    /**< The indicator*/
} lv_battery_draw_part_type_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a bar object
 * @param parent    pointer to an object, it will be the parent of the new bar
 * @return          pointer to the created bar
 */
lv_obj_t * lv_battery_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set a new value on the bar
 * @param bar       pointer to a bar object
 * @param value     new value
 * @param anim      LV_ANIM_ON: set the value with an animation; LV_ANIM_OFF: change the value immediately
 */
void lv_battery_set_value(lv_obj_t *obj, int32_t value);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the value of a bar
 * @param obj       pointer to a bar object
 * @return          the value of the bar
 */
int32_t lv_battery_get_value(const lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*lv_battery_H*/
