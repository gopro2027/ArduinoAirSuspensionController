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
    bool showPressures;
    lv_obj_t *scr;
    lv_obj_t *rect_bg;
    lv_obj_t *icon_navbar;
    Alert *alert;
    lv_obj_t *ui_lblPressureFrontDriver;
    lv_obj_t *ui_lblPressureRearDriver;
    lv_obj_t *ui_lblPressureFrontPassenger;
    lv_obj_t *ui_lblPressureRearPassenger;
    lv_obj_t *ui_lblPressureTank;
    int prevPressures[5];

    Scr(lv_image_dsc_t navbarImage, bool showPressures);
    virtual void runTouchInput(SimplePoint pos, bool down);
    virtual void init();
    virtual void loop();
    void updatePressureValues();
};

#endif