#ifndef ui_scrPresets_h
#define ui_scrPresets_h

#include "ui/components/Scr.h"
#include "device_lib_exports.h"

class ScrPresets : public Scr
{
    using Scr::Scr;

public:
    lv_obj_t *panel;
    lv_obj_t *car;
    lv_obj_t *ww1;
    lv_obj_t *ww2;
    lv_obj_t *wheels;

    // LVGL button objects for preset selection (supports rotation/encoder)
    lv_obj_t *btnPreset1;
    lv_obj_t *btnPreset2;
    lv_obj_t *btnPreset3;
    lv_obj_t *btnPreset4;
    lv_obj_t *btnPreset5;

    // Container for preset buttons (for group navigation)
    lv_obj_t *presetButtonsContainer;

    // Save/Load buttons (LVGL buttons, replaces background image touch areas)
    lv_obj_t *btnSave;
    lv_obj_t *btnLoad;

    void init();
    void runTouchInput(SimplePoint pos, bool down);
    void showPresetDialog();
    void loop();
    void setPreset(int num);
    void updateButtonStyles();
};

extern ScrPresets scrPresets;

#endif