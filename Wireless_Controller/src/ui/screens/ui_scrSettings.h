#ifndef ui_scrSettings_h
#define ui_scrSettings_h

#include "ui/components/Scr.h"

class ScrSettings : public Scr {
    using Scr::Scr;
public:
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
};

extern ScrSettings scrSettings;

#endif