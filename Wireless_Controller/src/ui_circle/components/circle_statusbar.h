#pragma once

#include <Arduino.h>
#include "lvgl.h"

/** Compact top overlay: battery + optional alert dot (circular UI).
 *  Positioned inside the circular bezel near top center. */
class CircleStatusbarMini {
public:
    lv_obj_t *container = nullptr;
    lv_obj_t *batteryIcon = nullptr;
    lv_obj_t *batteryLabel = nullptr;
    lv_obj_t *alertDot = nullptr;

    void create(lv_obj_t *parent);
    void update();
    void cleanup();

private:
    void refreshBattery();
};

extern CircleStatusbarMini circleStatusbarMini;
