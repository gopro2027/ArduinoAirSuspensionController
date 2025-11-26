#ifndef ui_scrPresets_h
#define ui_scrPresets_h

#include "ui/components/Scr.h"
#include "Arduino/examples/LVGL_Arduino/Display_ST7789.h"

class ScrPresets : public Scr
{
    using Scr::Scr;

public:
    lv_obj_t *panel;
    lv_obj_t *car;
    lv_obj_t *ww1;
    lv_obj_t *ww2;
    lv_obj_t *wheels;
    lv_obj_t *btnPreset1;
    lv_obj_t *btnPreset2;
    lv_obj_t *btnPreset3;
    lv_obj_t *btnPreset4;
    lv_obj_t *btnPreset5;
    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void showPresetDialog();
    void loop();
    void setPreset(int num);
    void hideSelectors();
};

extern ScrPresets scrPresets;

#endif