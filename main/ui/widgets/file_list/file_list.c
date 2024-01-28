//
// Created by gifbadge on 26/01/24.
//

#include <math.h>
#include "ui/widgets/file_list/file_list.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_file_list_class)

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_file_list_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void init_style(lv_obj_t *obj);

static void callback_dispatcher(lv_event_t *e);


/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_file_list_class = {.constructor_cb = lv_file_list_constructor, .width_def      = LV_SIZE_CONTENT, .height_def     = LV_SIZE_CONTENT, .instance_size  = sizeof(lv_file_list_t), .base_class     = &lv_obj_class,};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_file_list_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


/*=====================
 * Setter functions
 *====================*/

lv_obj_t *lv_file_list_add(lv_obj_t *obj, const char *icon) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    lv_obj_t *container = lv_btn_create(file_list->cont);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (icon != NULL) {
        lv_obj_t *img = lv_img_create(container);
        lv_img_set_src(img, icon);
        if (file_list->icon_style != NULL) {
            lv_obj_add_style(img, file_list->icon_style, LV_PART_MAIN);
        }
    }

    lv_obj_add_event_cb(container, callback_dispatcher, LV_EVENT_CLICKED, container);

    return container;
}

void lv_file_list_clear(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    lv_obj_clean(file_list->cont);
}

void lv_file_list_icon_style(lv_obj_t *obj, lv_style_t *style) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    file_list->icon_style = style;
}

void lv_file_list_scroll_to_view(lv_obj_t *obj, int i) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    lv_obj_scroll_to_view(lv_obj_get_child(file_list->cont, i), LV_ANIM_OFF);
}

void lv_file_list_set_user_data(lv_obj_t *obj, void *user_data) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    file_list->user_data = user_data;
}

/*=====================
 * Getter functions
 *====================*/

void *lv_file_list_get_user_data(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_file_list_t *file_list = (lv_file_list_t *) obj;
    return file_list->user_data;
}

/*=====================
 * Other functions
 *====================*/
/**********************
 *   STATIC FUNCTIONS
 **********************/

static void callback_dispatcher(lv_event_t *e) {
    LV_LOG_USER("Clicked");
    //Get the first child that isn't an image, as that's what we likely want to interact with
    lv_obj_t *obj = lv_obj_get_child(lv_event_get_current_target(e), 0);
    if (lv_obj_get_class(obj) == &lv_img_class && lv_obj_get_child_cnt(lv_event_get_current_target(e)) > 1) {
        obj = lv_obj_get_child(lv_event_get_current_target(e), 1);
    }
    if (lv_obj_get_state(obj) & LV_STATE_DISABLED) {
        lv_obj_scroll_to_view(lv_event_get_target(e), LV_ANIM_ON);
        return;
    }

    if (lv_indev_get_type(lv_event_get_indev(e)) != LV_INDEV_TYPE_ENCODER && lv_event_get_code(e) == LV_EVENT_PRESSED) {
        //Don't dispatch pressed events from anything but the encoder
        return;
    }


    if (obj == NULL) {
        return;
    }
    if (lv_obj_has_class(obj, &lv_switch_class)) {
        if (lv_obj_get_state(obj) & LV_STATE_CHECKED) {
            lv_obj_clear_state(obj, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
        }
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
    } else if (lv_obj_has_class(obj, &lv_roller_class)) {
        lv_indev_set_group(lv_indev_get_act(), lv_obj_get_group(obj));
        lv_group_focus_next(lv_obj_get_group(obj));
        lv_group_set_editing(lv_obj_get_group(obj), true);
        lv_event_send(obj, LV_EVENT_FOCUSED, NULL);
    } else {
        lv_event_send(obj, LV_EVENT_CLICKED, NULL);
    }
}

static void scroll_event_cb(lv_event_t *e) {
    lv_obj_t *cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = (lv_coord_t) (cont_a.y1 + lv_area_get_height(&cont_a) / 2);

    lv_coord_t r = (lv_coord_t) (lv_obj_get_height(cont) * 7 / 10);
    int32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for (i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = (lv_coord_t) (child_a.y1 + lv_area_get_height(&child_a) / 2);

        lv_coord_t diff_y = (lv_coord_t) (child_y_center - cont_y_center);
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if (diff_y >= r) {
            x = r;
        } else {
            /*Use Pythagoras theorem to get x from radius and y*/
//            uint32_t x_sqr = r * r - diff_y * diff_y;
//            lv_sqrt_res_t res;
//            lv_sqrt(x_sqr, &res, 0x8000);   /*Use lvgl's built in sqrt root function*/
//            x = (lv_coord_t) (r - res.i);
            uint32_t x_sqr = r * r - diff_y * diff_y;
            int res = sqrt(x_sqr);
//            lv_sqrt(x_sqr, &res, 0x8000);   /*Use lvgl's built in sqrt root function*/
            x = (lv_coord_t) (r - res);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        if (x == 0) {
            lv_obj_set_style_opa(child, LV_OPA_COVER, 0);
            for (int idx = 0; idx < lv_obj_get_child_cnt(child); idx++) {
                lv_obj_clear_state(lv_obj_get_child(child, idx), LV_STATE_DISABLED);
            }
        } else {
//            lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
//            lv_obj_set_style_opa(child, (LV_OPA_COVER - 100) - opa, 0);
            lv_obj_set_style_opa(child, 100, 0);
            for (int idx = 0; idx < lv_obj_get_child_cnt(child); idx++) {
                lv_obj_add_state(lv_obj_get_child(child, idx), LV_STATE_DISABLED);
            }
        }
    }
}

static void lv_file_list_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_file_list_t *explorer = (lv_file_list_t *) obj;


    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);

    explorer->cont = lv_obj_create(obj);
    lv_obj_set_flex_flow(explorer->cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(explorer->cont, LV_PCT(100));
    lv_obj_set_flex_grow(explorer->cont, 1);
    lv_obj_set_scroll_snap_y(explorer->cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_dir(explorer->cont, LV_DIR_VER);
    lv_obj_add_event_cb(explorer->cont, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    lv_event_send(explorer->cont, LV_EVENT_SCROLL, NULL);

    lv_obj_set_scrollbar_mode(explorer->cont, LV_SCROLLBAR_MODE_OFF);


    /*Initialize style*/
    init_style(obj);
    explorer->icon_style = NULL;

    LV_TRACE_OBJ_CREATE("finished");
}


static void init_style(lv_obj_t *obj) {
    lv_file_list_t *explorer = (lv_file_list_t *) obj;

    /*lv_file_explorer obj style*/
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf2f1f6), 0);

    /*main container style*/
    lv_obj_set_style_radius(explorer->cont, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(explorer->cont, true, 0);
    lv_obj_set_style_pad_bottom(explorer->cont, 50, LV_PART_MAIN);
    lv_obj_set_style_pad_top(explorer->cont, 50, LV_PART_MAIN);
    lv_obj_set_style_border_side(explorer->cont, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
}
