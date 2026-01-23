#ifndef ui_scrHome_h
#define ui_scrHome_h

#include <user_defines.h>
#include "ui/components/Scr.h"
#include "utils/util.h"

class ScrHome : public Scr
{
    using Scr::Scr;

public:
    void init();
    void loop();
    lv_obj_t *icon_home_bg;

    // Pill container references (containers hold up/down buttons)
    lv_obj_t *pillFrontDriver;
    lv_obj_t *pillFrontAxle;
    lv_obj_t *pillFrontPassenger;
    lv_obj_t *pillRearDriver;
    lv_obj_t *pillRearAxle;
    lv_obj_t *pillRearPassenger;
};

extern ScrHome scrHome;

#endif