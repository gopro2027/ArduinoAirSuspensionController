#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_scrHome.h"
#include "ui/ui.h"

ScrHome scrHome(true);

/* Knob-driven valve control: while the knob is being turned, keep valves
 * open for selected corners.  Once the user stops turning for KNOB_HOLD_MS
 * the valves close automatically. */
static const unsigned long KNOB_HOLD_MS = 100;

void ScrHome::arc_click_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    scrHome.corners_[idx].selected = !scrHome.corners_[idx].selected;
    scrHome.applySelectionStyle(idx);
}

void ScrHome::applySelectionStyle(int idx)
{
    Corner &c = corners_[idx];
    if (!c.arc)
        return;

    int sz = c.selected ? ARC_SIZE_SELECTED : ARC_SIZE_NORMAL;
    lv_obj_set_size(c.arc, sz, sz);

    if (c.selected) {
        lv_obj_set_style_arc_width(c.arc, 14, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(c.arc, LV_OPA_COVER, LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_arc_width(c.arc, 10, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(c.arc, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(c.arc, LV_OPA_COVER, LV_PART_INDICATOR);
    }
}

void ScrHome::init(lv_obj_t *parent)
{
    Scr::init(parent);

    corners_[0] = {WHEEL_FRONT_DRIVER, FRONT_DRIVER_IN, FRONT_DRIVER_OUT};
    corners_[1] = {WHEEL_FRONT_PASSENGER, FRONT_PASSENGER_IN, FRONT_PASSENGER_OUT};
    corners_[2] = {WHEEL_REAR_DRIVER, REAR_DRIVER_IN, REAR_DRIVER_OUT};
    corners_[3] = {WHEEL_REAR_PASSENGER, REAR_PASSENGER_IN, REAR_PASSENGER_OUT};

    const char *names[] = {"FD", "FP", "RD", "RP"};

    struct ArcLayout {
        int rotation;
        int startAngle;
        int endAngle;
        int labelX;
        int labelY;
        int nameX;
        int nameY;
    };

    const int arcWidth = 10;

    ArcLayout layouts[] = {
        /* FD: upper-left  (190->260, center 225) */
        {190, 0, 70,  -34, -34,  -42, -52},
        /* FP: upper-right (280->350, center 315) */
        {280, 0, 70,   34, -34,   42, -52},
        /* RD: lower-left  (100->170, center 135) */
        {100, 0, 70,  -34,  34,  -42,  52},
        /* RP: lower-right (10->80,   center  45) */
        { 10, 0, 70,   34,  34,   42,  52},
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
        lv_obj_align(c.pressLabel, LV_ALIGN_CENTER, L.labelX, L.labelY);

        c.nameLabel = lv_label_create(scr);
        lv_label_set_text(c.nameLabel, names[i]);
        lv_obj_set_style_text_color(c.nameLabel, lv_color_hex(GENERIC_GREY_LIGHT), 0);
        lv_obj_set_style_text_font(c.nameLabel, &lv_font_montserrat_10, 0);
        lv_obj_align(c.nameLabel, LV_ALIGN_CENTER, L.nameX, L.nameY);

        /* Invisible touch zone over each arc quadrant for tap-to-select */
        c.touchZone = lv_obj_create(scr);
        lv_obj_remove_style_all(c.touchZone);
        lv_obj_set_size(c.touchZone, 140, 140);
        int tzX = (L.labelX > 0) ? 50 : -50;
        int tzY = (L.labelY > 0) ? 50 : -50;
        lv_obj_align(c.touchZone, LV_ALIGN_CENTER, tzX, tzY);
        lv_obj_add_flag(c.touchZone, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(c.touchZone, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(c.touchZone, LV_OPA_TRANSP, 0);
        lv_obj_add_event_cb(c.touchZone, arc_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    tankLabel_ = lv_label_create(scr);
    lv_label_set_text(tankLabel_, "TANK");
    lv_obj_set_style_text_color(tankLabel_, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(tankLabel_, &lv_font_montserrat_10, 0);
    lv_obj_align(tankLabel_, LV_ALIGN_CENTER, 0, -14);

    tankValueLabel_ = lv_label_create(scr);
    lv_label_set_text(tankValueLabel_, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(tankValueLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_font(tankValueLabel_, &lv_font_montserrat_20, 0);
    lv_obj_align(tankValueLabel_, LV_ALIGN_CENTER, 0, 6);

#ifdef HAS_ROTARY_ENCODER
    if (g_knob_handle)
        lastKnobCount_ = iot_knob_get_count_value(g_knob_handle);
#endif
    knobActiveUntil_ = 0;
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

    lastKnobCount_ = current;
    knobActiveUntil_ = millis() + KNOB_HOLD_MS;

    bool anySelected = false;
    for (int i = 0; i < 4; i++) {
        Corner &c = corners_[i];
        if (!c.selected)
            continue;
        anySelected = true;
        if (delta > 0) {
            setValveBit(c.inValve);
            unsetValveBit(c.outValve);
        } else {
            setValveBit(c.outValve);
            unsetValveBit(c.inValve);
        }
    }

    if (!anySelected) {
        knobActiveUntil_ = 0;
    }
#endif
}

void ScrHome::syncArcs()
{
    bool hs = (*util_configValues._configFlagsBits() & (1 << ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE));

    for (int i = 0; i < 4; i++) {
        Corner &c = corners_[i];
        if (!c.arc)
            continue;
        int v = currentPressures[c.wheelIdx];
        int maxVal = hs ? 100 : 200;
        lv_arc_set_range(c.arc, 0, maxVal);
        if (v > maxVal) v = maxVal;
        lv_arc_set_value(c.arc, v);

        if (c.pressLabel) {
            if (hs) {
                lv_label_set_text_fmt(c.pressLabel, "%u%%", (unsigned)v);
            } else if (getunitsMode() == UNITS_MODE::PSI) {
                lv_label_set_text_fmt(c.pressLabel, "%u", (unsigned)v);
            } else {
                float bar = v / 14.5038f;
                lv_label_set_text_fmt(c.pressLabel, "%.1f", (double)bar);
            }
        }
    }

    if (tankValueLabel_) {
        int t = currentPressures[_TANK_INDEX];
        if (getunitsMode() == UNITS_MODE::PSI)
            lv_label_set_text_fmt(tankValueLabel_, "%u PSI", (unsigned)t);
        else {
            float bar = t / 14.5038f;
            lv_label_set_text_fmt(tankValueLabel_, "%.1f bar", (double)bar);
        }
    }
}

void ScrHome::loop()
{
    Scr::loop();
    processKnob();
    syncArcs();
}

void ScrHome::cleanup()
{
    /* Close any active valve bits from knob before leaving */
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

    tankLabel_ = nullptr;
    tankValueLabel_ = nullptr;
    for (int i = 0; i < 4; i++)
        corners_[i] = Corner{};
    Scr::cleanup();
}

#endif /* SCREEN_MODE_CIRCLE */
