#ifndef ui_scr_h
#define ui_scr_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "alert.h"

class Alert;
struct SimplePoint;

class Scr
{
public:
    lv_image_dsc_t navbarImage;
    lv_obj_t *scr;
    lv_obj_t *rect_bg;
    lv_obj_t *icon_navbar;
    Alert *alert;
    Scr(lv_image_dsc_t navbarImage);
    virtual void runTouchInput(SimplePoint pos, bool down);
    virtual void init();
    virtual void loop();
};

#endif