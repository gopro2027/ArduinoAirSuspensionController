#pragma once
#include "lvgl.h"

// Which tab is active
enum NavId { NAV_HOME = 0, NAV_PRESETS = 1, NAV_SETTINGS = 2 };

// Create a 49px-tall bottom navbar (active is a Label, others are Buttons).
// Returns the nav container (store in Scr::icon_navbar if you like).
lv_obj_t* navbar_create(lv_obj_t* parent, NavId active);
