#ifndef ui_scrPresets_h
#define ui_scrPresets_h

#ifndef SCREEN_MODE_CIRCLE

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

    // LVGL button objects for preset selection (supports rotation/encoder).
    // Only the first presetCount entries are created; the rest stay NULL.
    lv_obj_t *btnPresets[MAX_PROFILE_COUNT];
    int presetCount = MAX_PROFILE_COUNT;

    // Container for preset buttons (for group navigation)
    lv_obj_t *presetButtonsContainer;

    // Save/Load buttons (LVGL buttons, replaces background image touch areas)
    lv_obj_t *btnSave;
    lv_obj_t *btnLoad;

    void init(lv_obj_t *parent = nullptr) override;
    void showPresetDialog();
    void loop() override;
    void setPreset(int num);
    void updateButtonStyles();
};

extern ScrPresets scrPresets;

#endif /* !SCREEN_MODE_CIRCLE */

#endif /* ui_scrPresets_h */