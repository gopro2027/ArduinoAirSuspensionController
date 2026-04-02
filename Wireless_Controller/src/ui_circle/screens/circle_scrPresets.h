#pragma once

#include "ui/components/Scr.h"

#ifdef SCREEN_MODE_CIRCLE

class ScrPresets : public Scr {
    using Scr::Scr;

public:
    lv_obj_t *presetBtns[5] = {};
    lv_obj_t *btnSave = nullptr;
    lv_obj_t *btnLoad = nullptr;

    void init(lv_obj_t *parent = nullptr) override;
    void showPresetDialog();
    void loop() override;
    void setPreset(int num);
    void updateButtonStyles();
    void cleanup() override;
};

extern ScrPresets scrPresets;

#endif /* SCREEN_MODE_CIRCLE */
