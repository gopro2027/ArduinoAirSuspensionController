#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_nav.h"
#include "utils/util.h"

CirclePageDots circlePageDots;

void CirclePageDots::create(lv_obj_t *parent)
{
    container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(container, 4, 0);
    lv_obj_set_style_pad_column(container, 8, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /* Position well inside the circular bezel — ~24px from edge for 360px display */
    lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_FLOATING);

    for (int i = 0; i < 3; i++) {
        dots[i] = lv_obj_create(container);
        lv_obj_remove_style_all(dots[i]);
        lv_obj_set_size(dots[i], 8, 8);
        lv_obj_set_style_radius(dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dots[i], lv_color_hex(GENERIC_GREY), 0);
        lv_obj_remove_flag(dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    active_ = 0;
    setActive(0);
}

void CirclePageDots::setActive(uint8_t index)
{
    if (index > 2)
        return;
    active_ = index;
    for (int i = 0; i < 3; i++) {
        if (!dots[i]) continue;
        if ((uint8_t)i == index) {
            lv_obj_set_style_bg_color(dots[i], lv_color_hex(THEME_COLOR_LIGHT), 0);
            lv_obj_set_size(dots[i], 10, 10);
        } else {
            lv_obj_set_style_bg_color(dots[i], lv_color_hex(GENERIC_GREY), 0);
            lv_obj_set_size(dots[i], 8, 8);
        }
    }
}

void CirclePageDots::cleanup()
{
    for (int i = 0; i < 3; i++)
        dots[i] = nullptr;
    container = nullptr;
}

#endif /* SCREEN_MODE_CIRCLE */
