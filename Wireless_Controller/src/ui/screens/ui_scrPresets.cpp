#include "ui_scrPresets.h"

LV_IMG_DECLARE(navbar_presets);
ScrPresets scrPresets(navbar_presets);

void ScrPresets::init() 
{
    Scr::init();
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
}

void ScrPresets::loop() {
    Scr::loop();
}
