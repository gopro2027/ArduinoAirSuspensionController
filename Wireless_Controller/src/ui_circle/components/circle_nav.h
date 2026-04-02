#pragma once

#include <Arduino.h>
#include "lvgl.h"

#ifdef SCREEN_MODE_CIRCLE

class CircleMenu {
public:
    lv_obj_t *handle = nullptr;
    lv_obj_t *statusHandle = nullptr;
    lv_obj_t *overlay = nullptr;
    lv_obj_t *menuBtns[4] = {};
    lv_obj_t *menuLabels[4] = {};
    lv_obj_t *statusOverlay = nullptr;

    bool isOpen = false;

    void create(lv_obj_t *parent);
    void open();
    void close();
    void showStatus();
    void hideStatus();
    void setActive(uint8_t index);
    void cleanup();

private:
    uint8_t active_ = 0;
    lv_obj_t *parent_ = nullptr;
};

extern CircleMenu circleMenu;

#endif /* SCREEN_MODE_CIRCLE */
