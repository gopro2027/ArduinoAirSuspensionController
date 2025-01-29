#ifndef alert_h
#define alert_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "Scr.h"

class Scr;

class Alert
{
public:
    lv_obj_t *rect;
    lv_obj_t *text;
    Alert(Scr *scr);
    void show(lv_color_t color, char *text);
    void hide();
};

#endif