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
};

extern ScrHome scrHome;

#endif