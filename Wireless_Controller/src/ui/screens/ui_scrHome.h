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
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
    lv_obj_t *icon_home_bg;

    // Pill references for press animation (unified up/down buttons)
    lv_obj_t *pillFrontDriver;
    lv_obj_t *pillFrontAxle;
    lv_obj_t *pillFrontPassenger;
    lv_obj_t *pillRearDriver;
    lv_obj_t *pillRearAxle;
    lv_obj_t *pillRearPassenger;

    // Currently pressed pill and which half (for animation)
    lv_obj_t *pressedPill;
    bool pressedIsUp;
};

extern ScrHome scrHome;

#endif