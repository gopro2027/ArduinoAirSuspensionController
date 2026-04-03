#pragma once

#include "ui/components/Scr.h"

#ifdef SCREEN_MODE_CIRCLE

class ScrPresets : public Scr {
    using Scr::Scr;

    bool presetCircleSelected_ = false;

#ifdef HAS_ROTARY_ENCODER
    int lastKnobCount_ = 0;
    void processKnob();
#endif

public:
    lv_obj_t *presetSelectBtn = nullptr;
    lv_obj_t *presetNumberLabel = nullptr;
    lv_obj_t *hintLabel = nullptr;
    lv_obj_t *btnSave = nullptr;
    lv_obj_t *btnLoad = nullptr;

    void init(lv_obj_t *parent = nullptr) override;
    void showPresetDialog();
    void loop() override;
    void setPreset(int num);
    void updateButtonStyles();
    void togglePresetCircleSelect();
    void cleanup() override;
};

extern ScrPresets scrPresets;

#endif /* SCREEN_MODE_CIRCLE */
