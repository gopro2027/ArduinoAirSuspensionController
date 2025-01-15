#ifndef ui_scrHome_h
#define ui_scrHome_h

#ifdef __cplusplus
extern "C" {
#endif
#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

typedef struct
{
    lv_obj_t *ui_scrHome;
    lv_obj_t *ui_scrHome_icon_home_bg;
} ScrHome;
void ui_scrHome_screen_init(void);
void ui_scrHome_loop();

extern ScrHome scrHome;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif