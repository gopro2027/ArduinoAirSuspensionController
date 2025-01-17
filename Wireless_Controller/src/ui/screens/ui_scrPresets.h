#ifndef ui_scrPresets_h
#define ui_scrPresets_h

#include "ui/components/Scr.h"

class ScrPresets : public Scr {
    using Scr::Scr;
public:
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void loop();
};

extern ScrPresets scrPresets;

#endif