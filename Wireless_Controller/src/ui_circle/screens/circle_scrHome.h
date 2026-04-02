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

private:
    struct Corner {
        int wheelIdx = 0;
        int inValve = 0;
        int outValve = 0;
        lv_obj_t *arc = nullptr;
        lv_obj_t *pressLabel = nullptr;
        lv_obj_t *nameLabel = nullptr;
        lv_obj_t *btnUp = nullptr;
        lv_obj_t *btnDown = nullptr;
    };

    Corner corners_[4];
    lv_obj_t *tankLabel_ = nullptr;
    lv_obj_t *tankValueLabel_ = nullptr;

    void syncArcs();
};

extern ScrHome scrHome;

#endif /* SCREEN_MODE_CIRCLE */
