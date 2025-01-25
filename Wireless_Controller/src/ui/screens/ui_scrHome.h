#ifndef ui_scrHome_h
#define ui_scrHome_h

#include <user_defines.h>
#include "ui/components/Scr.h"

class ScrHome : public Scr {
    using Scr::Scr;
public:
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
    void updatePressureValues();
    lv_obj_t *icon_home_bg;
    lv_obj_t *ui_lblPressureFrontDriver;
    lv_obj_t *ui_lblPressureRearDriver;
    lv_obj_t *ui_lblPressureFrontPassenger;
    lv_obj_t *ui_lblPressureRearPassenger;
    lv_obj_t *ui_lblPressureTank;
};

extern ScrHome scrHome;

#endif