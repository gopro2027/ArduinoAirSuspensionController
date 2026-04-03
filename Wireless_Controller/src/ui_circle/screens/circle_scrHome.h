#pragma once

#include <user_defines.h>
#include "ui/components/Scr.h"
#include "utils/util.h"

#ifdef SCREEN_MODE_CIRCLE

class ScrHome : public Scr {
    using Scr::Scr;

public:
    void init(lv_obj_t *parent = nullptr) override;
    void loop() override;
    void cleanup() override;

    /** Preset slot 1–5 (merged from former presets page). */
    void setPreset(int num);
    void showPresetDialog();

private:

    struct Corner {
        int wheelIdx = 0;
        int inValve = 0;
        int outValve = 0;
        lv_obj_t *arc = nullptr;
        lv_obj_t *pressLabel = nullptr;
        lv_obj_t *nameLabel = nullptr;
        lv_obj_t *touchZone = nullptr;
        bool selected = false;
    };

    Corner corners_[4];
    lv_obj_t *tankLabel_ = nullptr;
    lv_obj_t *tankValueLabel_ = nullptr;

    /* Center preset: tap = dialog (OK / save height); long-press = load */
    lv_obj_t *presetSelectBtn_ = nullptr;
    lv_obj_t *presetNumberLabel_ = nullptr;

    int lastKnobCount_ = 0;
    unsigned long knobActiveUntil_ = 0;

    static const int ARC_SIZE_NORMAL = 200;
    static const int ARC_SIZE_SELECTED = 220;

    void syncArcs();
    void applySelectionStyle(int idx);
    void processKnob();
    void updatePresetVisuals();
    static void arc_click_cb(lv_event_t *e);
};

extern ScrHome scrHome;

#endif /* SCREEN_MODE_CIRCLE */
