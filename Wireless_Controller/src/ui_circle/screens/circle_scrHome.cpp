#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_scrHome.h"
#include "ui/ui.h"
#include <BTOas.h>

ScrHome scrHome(true);

static const unsigned long KNOB_HOLD_MS = 100;

static constexpr int kPresetMin = 1;
static constexpr int kPresetMax = 5;

/** After long-press load, suppress the following CLICKED so the PSI dialog does not open. */
static bool s_skipNextPresetClick = false;

static void loadSelectedPreset()
{
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

/** Deferred so preset dialog closes before opening confirm (avoids deleteMessageBoxNextFrame clobbering the new box). */
static void openSaveCurrentHeightConfirm()
{
    static char buf[48];
    snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
    scrHome.showMsgBox(buf, nullptr, "Confirm", "Cancel",
                       []() {
                           SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                           sendRestPacket(&pkt);
                           showDialog("Saved!", lv_color_hex(THEME_COLOR_LIGHT));
                           requestPreset();
                       },
                       []() {}, false);
}

static void centerPresetEventCb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_LONG_PRESSED) {
        if (currentPreset < kPresetMin || currentPreset > kPresetMax)
            return;
        s_skipNextPresetClick = true;
        if (currentPreset == 1) {
            scrHome.showMsgBox("Air out?", "Preset 1 is typically air out. Verify car is not moving.", "Confirm", "Cancel",
                               []() { loadSelectedPreset(); }, []() {}, true);
        } else {
            loadSelectedPreset();
        }
        return;
    }
    if (code != LV_EVENT_CLICKED)
        return;
    if (s_skipNextPresetClick) {
        s_skipNextPresetClick = false;
        return;
    }
    if (currentPreset >= kPresetMin && currentPreset <= kPresetMax)
        scrHome.showPresetDialog();
}

void ScrHome::arc_click_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    scrHome.corners_[idx].selected = !scrHome.corners_[idx].selected;
    scrHome.applySelectionStyle(idx);
}

static int s_circleArcMaxCached = -1;

void ScrHome::applySelectionStyle(int idx)
{
    Corner &c = corners_[idx];
    if (!c.arc)
        return;

    const int mainW = c.selected ? 20 : 16;
    const int indW  = c.selected ? 22 : 16;
    const int sz    = c.selected ? ARC_SIZE_SELECTED : ARC_SIZE_NORMAL;

    lv_obj_set_size(c.arc, sz, sz);
    lv_obj_align(c.arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_width(c.arc, mainW, LV_PART_MAIN);
    lv_obj_set_style_arc_width(c.arc, indW, LV_PART_INDICATOR);

    if (c.selected) {
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(GENERIC_GREY), LV_PART_MAIN);
        lv_obj_set_style_arc_color(c.arc, lv_color_white(), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(GENERIC_GREY_DARK), LV_PART_MAIN);
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
    }
}

void ScrHome::init(lv_obj_t *parent)
{
    Scr::init(parent);

    corners_[0] = {WHEEL_FRONT_DRIVER, FRONT_DRIVER_IN, FRONT_DRIVER_OUT};
    corners_[1] = {WHEEL_FRONT_PASSENGER, FRONT_PASSENGER_IN, FRONT_PASSENGER_OUT};
    corners_[2] = {WHEEL_REAR_DRIVER, REAR_DRIVER_IN, REAR_DRIVER_OUT};
    corners_[3] = {WHEEL_REAR_PASSENGER, REAR_PASSENGER_IN, REAR_PASSENGER_OUT};

    const char *names[] = {"FL", "FR", "RL", "RR"};

    struct ArcLayout {
        int rotation;
        int startAngle;
        int endAngle;
        int pressX;
        int pressY;
        int nameX;
        int nameY;
    };

    const int arcWidth = 16;
    const int pressureRad = 87;
    const int nameRad = 100;

    ArcLayout layouts[] = {
        {190, 0, 70,  -pressureRad, -pressureRad,  -nameRad, -nameRad},
        {280, 0, 70,   pressureRad, -pressureRad,   nameRad, -nameRad},
        {100, 0, 70,  -pressureRad,  pressureRad,  -nameRad,  nameRad},
        { 10, 0, 70,   pressureRad,  pressureRad,   nameRad,  nameRad},
    };

    for (int i = 0; i < 4; i++) {
        Corner &c = corners_[i];
        ArcLayout &L = layouts[i];

        c.arc = lv_arc_create(scr);
        lv_obj_set_size(c.arc, ARC_SIZE_NORMAL, ARC_SIZE_NORMAL);
        lv_arc_set_rotation(c.arc, L.rotation);
        lv_arc_set_bg_angles(c.arc, L.startAngle, L.endAngle);
        lv_arc_set_range(c.arc, 0, 200);
        lv_arc_set_value(c.arc, 0);
        if (i == 1 || i == 3)
            lv_arc_set_mode(c.arc, LV_ARC_MODE_REVERSE);
        lv_obj_remove_flag(c.arc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_width(c.arc, arcWidth, LV_PART_MAIN);
        lv_obj_set_style_arc_width(c.arc, arcWidth, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(GENERIC_GREY_DARK), LV_PART_MAIN);
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(c.arc, true, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(c.arc, true, LV_PART_INDICATOR);
        lv_obj_remove_style(c.arc, nullptr, LV_PART_KNOB);
        lv_obj_align(c.arc, LV_ALIGN_CENTER, 0, 0);

        c.pressLabel = lv_label_create(scr);
        lv_label_set_text(c.pressLabel, "0");
        lv_obj_set_style_text_color(c.pressLabel, lv_color_white(), 0);
        lv_obj_set_style_text_font(c.pressLabel, &lv_font_montserrat_16, 0);
        lv_obj_align(c.pressLabel, LV_ALIGN_CENTER, L.pressX, L.pressY);

        c.nameLabel = lv_label_create(scr);
        lv_label_set_text(c.nameLabel, names[i]);
        lv_obj_set_style_text_color(c.nameLabel, lv_color_hex(GENERIC_GREY_LIGHT), 0);
        lv_obj_set_style_text_font(c.nameLabel, &lv_font_montserrat_10, 0);
        lv_obj_align(c.nameLabel, LV_ALIGN_CENTER, L.nameX, L.nameY);

        c.touchZone = lv_obj_create(scr);
        lv_obj_remove_style_all(c.touchZone);
        lv_obj_set_size(c.touchZone, 140, 140);
        int tzX = (L.pressX > 0) ? 50 : -50;
        int tzY = (L.pressY > 0) ? 50 : -50;
        lv_obj_align(c.touchZone, LV_ALIGN_CENTER, tzX, tzY);
        lv_obj_add_flag(c.touchZone, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(c.touchZone, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(c.touchZone, LV_OPA_TRANSP, 0);
        lv_obj_add_event_cb(c.touchZone, arc_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    for (int i = 0; i < 4; i++) {
        if (corners_[i].arc)
            lv_obj_move_foreground(corners_[i].arc);
        if (corners_[i].pressLabel)
            lv_obj_move_foreground(corners_[i].pressLabel);
        if (corners_[i].nameLabel)
            lv_obj_move_foreground(corners_[i].nameLabel);
        if (corners_[i].touchZone)
            lv_obj_move_foreground(corners_[i].touchZone);
    }

    const int tankLabelY = 40;
    const int tankValueY = 50;

    tankLabel_ = lv_label_create(scr);
    lv_label_set_text(tankLabel_, "TANK");
    lv_obj_set_style_text_color(tankLabel_, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(tankLabel_, &lv_font_montserrat_10, 0);
    lv_obj_align(tankLabel_, LV_ALIGN_TOP_MID, 0, tankLabelY);

    tankValueLabel_ = lv_label_create(scr);
    lv_label_set_text(tankValueLabel_, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(tankValueLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_font(tankValueLabel_, &lv_font_montserrat_20, 0);
    lv_obj_align(tankValueLabel_, LV_ALIGN_TOP_MID, 0, tankValueY);

    lv_obj_move_foreground(tankLabel_);
    lv_obj_move_foreground(tankValueLabel_);

    /* Center preset: tap = PSI dialog (OK / save height), long-press = load. */
    const int presetCircleSz = 100;
    presetSelectBtn_ = lv_btn_create(scr);
    lv_obj_set_size(presetSelectBtn_, presetCircleSz, presetCircleSz);
    lv_obj_set_style_radius(presetSelectBtn_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(presetSelectBtn_, lv_color_hex(GENERIC_GREY_DARK), 0);
    lv_obj_set_style_bg_opa(presetSelectBtn_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(presetSelectBtn_, 3, 0);
    lv_obj_set_style_border_color(presetSelectBtn_, lv_color_hex(THEME_COLOR_DARK), 0);
    lv_obj_set_style_shadow_width(presetSelectBtn_, 16, 0);
    lv_obj_set_style_shadow_color(presetSelectBtn_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(presetSelectBtn_, LV_OPA_50, 0);
    lv_obj_align(presetSelectBtn_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(presetSelectBtn_, centerPresetEventCb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(presetSelectBtn_, centerPresetEventCb, LV_EVENT_LONG_PRESSED, nullptr);

    presetNumberLabel_ = lv_label_create(presetSelectBtn_);
    lv_label_set_text(presetNumberLabel_, "3");
    lv_obj_set_style_text_font(presetNumberLabel_, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(presetNumberLabel_, lv_color_white(), 0);
    lv_obj_remove_flag(presetNumberLabel_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(presetNumberLabel_);

    lv_obj_move_foreground(presetSelectBtn_);

#ifdef HAS_ROTARY_ENCODER
    if (g_knob_handle)
        lastKnobCount_ = iot_knob_get_count_value(g_knob_handle);
#endif
    knobActiveUntil_ = 0;

    int savedPreset = currentPreset;
    currentPreset = -1;
    setPreset(savedPreset > 0 ? savedPreset : 3);
}

void ScrHome::updatePresetVisuals()
{
    if (!presetSelectBtn_ || !presetNumberLabel_)
        return;

    const bool ok = (currentPreset >= kPresetMin && currentPreset <= kPresetMax);
    lv_label_set_text_fmt(presetNumberLabel_, "%d", ok ? currentPreset : 0);
}

void ScrHome::setPreset(int num)
{
    currentPreset = num;
    updatePresetVisuals();
    requestPreset();
}

void ScrHome::showPresetDialog()
{
    static char text[100];
    static char titleBuf[10];
    snprintf(text, sizeof(text), "  fd: %i                        fp: %i\n  rd: %i                        rp: %i",
             profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER], profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER],
             profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER], profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER]);
    snprintf(titleBuf, sizeof(titleBuf), "Preset %i", currentPreset);
    showMsgBox(titleBuf, text, "OK", "Save height to preset",
               []() {},
               []() { runNextFrame([]() { openSaveCurrentHeightConfirm(); }); },
               false);
}

void ScrHome::processKnob()
{
#ifdef HAS_ROTARY_ENCODER
    if (!g_knob_handle)
        return;

    int current = iot_knob_get_count_value(g_knob_handle);
    int delta = current - lastKnobCount_;

    if (delta == 0) {
        if (knobActiveUntil_ && millis() > knobActiveUntil_) {
            for (int i = 0; i < 4; i++) {
                Corner &c = corners_[i];
                if (c.selected) {
                    unsetValveBit(c.inValve);
                    unsetValveBit(c.outValve);
                }
            }
            knobActiveUntil_ = 0;
        }
        return;
    }

    if (isMsgBoxDisplayed()) {
        lastKnobCount_ = current;
        return;
    }

    lastKnobCount_ = current;

    bool anyCornerSelected = false;
    for (int i = 0; i < 4; i++) {
        if (corners_[i].selected) {
            anyCornerSelected = true;
            break;
        }
    }

    if (anyCornerSelected) {
        knobActiveUntil_ = millis() + KNOB_HOLD_MS;
        for (int i = 0; i < 4; i++) {
            Corner &c = corners_[i];
            if (!c.selected)
                continue;
            if (delta > 0) {
                setValveBit(c.inValve);
                unsetValveBit(c.outValve);
            } else {
                setValveBit(c.outValve);
                unsetValveBit(c.inValve);
            }
        }
        return;
    }

    knobActiveUntil_ = 0;

    int p = currentPreset;
    if (p < kPresetMin || p > kPresetMax)
        p = 3;
    p += delta;
    if (p < kPresetMin)
        p = kPresetMin;
    if (p > kPresetMax)
        p = kPresetMax;
    if (p != currentPreset)
        setPreset(p);
#endif
}

void ScrHome::syncArcs()
{
    const bool hs =
        (*util_configValues._configFlagsBits() & (1 << ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE));
    const int maxVal = hs ? 100 : 200;
    const int uMode = (int)getunitsMode();
    const bool unitsChanged = (prevUnitsMode != uMode);
    const bool maxChanged = (s_circleArcMaxCached != maxVal);

    if (maxChanged) {
        s_circleArcMaxCached = maxVal;
        for (int i = 0; i < 4; i++) {
            if (corners_[i].arc)
                lv_arc_set_range(corners_[i].arc, 0, maxVal);
        }
    }

    for (int i = 0; i < 4; i++) {
        Corner &c = corners_[i];
        if (!c.arc)
            continue;
        int v = currentPressures[c.wheelIdx];
        if (v > maxVal)
            v = maxVal;

        const bool vChanged = (prevPressures[c.wheelIdx] != v);
        const bool needArc = vChanged || maxChanged;
        const bool needLabel = vChanged || unitsChanged || maxChanged;

        if (needArc)
            lv_arc_set_value(c.arc, v);

        if (needLabel && c.pressLabel) {
            if (hs) {
                lv_label_set_text_fmt(c.pressLabel, "%u%%", (unsigned)v);
            } else if (getunitsMode() == UNITS_MODE::PSI) {
                lv_label_set_text_fmt(c.pressLabel, "%u", (unsigned)v);
            } else {
                float bar = v / 14.5038f;
                lv_label_set_text_fmt(c.pressLabel, "%.1f", (double)bar);
            }
        }

        if (needArc || needLabel)
            prevPressures[c.wheelIdx] = v;
    }

    if (tankValueLabel_) {
        int t = currentPressures[_TANK_INDEX];
        const bool tChanged = (prevPressures[_TANK_INDEX] != t);
        const bool needTank = tChanged || unitsChanged;
        if (needTank) {
            if (getunitsMode() == UNITS_MODE::PSI)
                lv_label_set_text_fmt(tankValueLabel_, "%u PSI", (unsigned)t);
            else {
                float bar = t / 14.5038f;
                lv_label_set_text_fmt(tankValueLabel_, "%.1f bar", (double)bar);
            }
            prevPressures[_TANK_INDEX] = t;
        }
    }

    if (unitsChanged)
        prevUnitsMode = uMode;
}

void ScrHome::loop()
{
    Scr::loop();
    processKnob();
    syncArcs();
}

void ScrHome::cleanup()
{
#ifdef HAS_ROTARY_ENCODER
    if (knobActiveUntil_) {
        for (int i = 0; i < 4; i++) {
            Corner &c = corners_[i];
            if (c.selected) {
                unsetValveBit(c.inValve);
                unsetValveBit(c.outValve);
            }
        }
        knobActiveUntil_ = 0;
    }
#endif

    s_skipNextPresetClick = false;
    presetSelectBtn_ = nullptr;
    presetNumberLabel_ = nullptr;
    tankLabel_ = nullptr;
    tankValueLabel_ = nullptr;
    for (int i = 0; i < 4; i++)
        corners_[i] = Corner{};
    s_circleArcMaxCached = -1;
    Scr::cleanup();
}

#endif /* SCREEN_MODE_CIRCLE */
