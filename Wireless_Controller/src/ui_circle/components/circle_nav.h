#pragma once

#include <Arduino.h>
#include "lvgl.h"

/** Three-dot page indicator for circular UI — bottom center, inset from bezel. */
class CirclePageDots {
public:
    lv_obj_t *container = nullptr;
    lv_obj_t *dots[3] = {nullptr, nullptr, nullptr};

    void create(lv_obj_t *parent);
    void setActive(uint8_t index);
    void cleanup();

private:
    uint8_t active_ = 0;
};

extern CirclePageDots circlePageDots;
