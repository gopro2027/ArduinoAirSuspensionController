#ifndef ui_scrHome_h
#define ui_scrHome_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

typedef struct
{
    lv_obj_t *ui_scrHome;
    lv_obj_t *ui_scrHome_icon_home_bg;
} ScrHome;
void ui_scrHome_screen_init(void);
void ui_scrHome_loop();

extern ScrHome scrHome;

#endif