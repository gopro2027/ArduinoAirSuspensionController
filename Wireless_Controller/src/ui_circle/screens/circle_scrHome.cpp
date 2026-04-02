#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_scrHome.h"
#include "ui/ui.h"

ScrHome scrHome(true);

static void valve_pressed(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_PRESSED)
        setValveBit((int)(intptr_t)lv_event_get_user_data(e));
}

static void valve_released(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED)
        unsetValveBit((int)(intptr_t)lv_event_get_user_data(e));
}

/* ------------------------------------------------------------------ */
/*  Home screen layout for 360x360 round display                      */
/*                                                                    */
/*  Design: Four 120° arc gauges arranged in quadrants around center. */
/*  Each arc has a pressure label inside and corner abbreviation.     */
/*  Up/Down valve buttons sit at each arc's outer edge.               */
/*  Tank pressure displayed as a large centered label.                */
/*                                                                    */
/*  The usable circle is ~320px dia to stay clear of round bezel.     */
/* ------------------------------------------------------------------ */

void ScrHome::init(lv_obj_t *parent)
{
    Scr::init(parent);

    corners_[0] = {WHEEL_FRONT_DRIVER, FRONT_DRIVER_IN, FRONT_DRIVER_OUT};
    corners_[1] = {WHEEL_FRONT_PASSENGER, FRONT_PASSENGER_IN, FRONT_PASSENGER_OUT};
    corners_[2] = {WHEEL_REAR_DRIVER, REAR_DRIVER_IN, REAR_DRIVER_OUT};
    corners_[3] = {WHEEL_REAR_PASSENGER, REAR_PASSENGER_IN, REAR_PASSENGER_OUT};

    const char *names[] = {"FD", "FP", "RD", "RP"};

    /* Arc layout: four arcs, each spanning ~70° with 20° gaps.
     * FL top-left, FR top-right, RL bottom-left, RR bottom-right.
     * Start angles measured clockwise from 12 o'clock (LVGL: 0°=3 o'clock).
     * Rotation offsets: TL=225°, TR=315°, BL=135°, BR=45° (center of each quadrant) */
    struct ArcLayout {
        int rotation;      // lv_arc_set_rotation
        int startAngle;    // lv_arc_set_bg_angles start
        int endAngle;      // lv_arc_set_bg_angles end
        int labelX;        // pressure label offset from center
        int labelY;
        int nameX;         // abbreviation label offset
        int nameY;
        int btnUpX;        // up button position
        int btnUpY;
        int btnDownX;      // down button position
        int btnDownY;
    };

    const int arcSize = 140;
    const int arcWidth = 10;

    ArcLayout layouts[] = {
        /* FD: top-left quadrant */
        {200, 0, 80, -58, -58, -90, -24, -125, -66, -68, -125},
        /* FP: top-right quadrant */
        {280, 0, 80,  58, -58,  90, -24,  125, -66,  68, -125},
        /* RD: bottom-left quadrant */
        {110, 0, 80, -58,  58, -90,  24, -68,  125, -125,  66},
        /* RP: bottom-right quadrant */
        {  10, 0, 80,  58,  58,  90,  24,  68,  125,  125,  66},
    };

    for (int i = 0; i < 4; i++) {
        Corner &c = corners_[i];
        ArcLayout &L = layouts[i];

        c.arc = lv_arc_create(scr);
        lv_obj_set_size(c.arc, arcSize, arcSize);
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

        const int btnSz = 36;
        c.btnUp = lv_btn_create(scr);
        lv_obj_set_size(c.btnUp, btnSz, btnSz);
        lv_obj_set_style_radius(c.btnUp, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(c.btnUp, lv_color_hex(GENERIC_GREY_DARK), 0);
        lv_obj_set_style_bg_opa(c.btnUp, LV_OPA_80, 0);
        lv_obj_set_style_border_width(c.btnUp, 1, 0);
        lv_obj_set_style_border_color(c.btnUp, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_shadow_width(c.btnUp, 0, 0);
        lv_obj_align(c.btnUp, LV_ALIGN_CENTER, L.btnUpX, L.btnUpY);
        lv_obj_t *lu = lv_label_create(c.btnUp);
        lv_label_set_text(lu, LV_SYMBOL_PLUS);
        lv_obj_set_style_text_color(lu, lv_color_hex(0x4ADE80), 0);
        lv_obj_center(lu);
        lv_obj_add_event_cb(c.btnUp, valve_pressed, LV_EVENT_PRESSED, (void *)(intptr_t)c.inValve);
        lv_obj_add_event_cb(c.btnUp, valve_released, LV_EVENT_RELEASED, (void *)(intptr_t)c.inValve);

        c.btnDown = lv_btn_create(scr);
        lv_obj_set_size(c.btnDown, btnSz, btnSz);
        lv_obj_set_style_radius(c.btnDown, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(c.btnDown, lv_color_hex(GENERIC_GREY_DARK), 0);
        lv_obj_set_style_bg_opa(c.btnDown, LV_OPA_80, 0);
        lv_obj_set_style_border_width(c.btnDown, 1, 0);
        lv_obj_set_style_border_color(c.btnDown, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_shadow_width(c.btnDown, 0, 0);
        lv_obj_align(c.btnDown, LV_ALIGN_CENTER, L.btnDownX, L.btnDownY);
        lv_obj_t *ld = lv_label_create(c.btnDown);
        lv_label_set_text(ld, LV_SYMBOL_MINUS);
        lv_obj_set_style_text_color(ld, lv_color_hex(0xF87171), 0);
        lv_obj_center(ld);
        lv_obj_add_event_cb(c.btnDown, valve_pressed, LV_EVENT_PRESSED, (void *)(intptr_t)c.outValve);
        lv_obj_add_event_cb(c.btnDown, valve_released, LV_EVENT_RELEASED, (void *)(intptr_t)c.outValve);
    }

    /* Tank pressure — large centered text */
    tankLabel_ = lv_label_create(scr);
    lv_label_set_text(tankLabel_, "TANK");
    lv_obj_set_style_text_color(tankLabel_, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(tankLabel_, &lv_font_montserrat_10, 0);
    lv_obj_align(tankLabel_, LV_ALIGN_CENTER, 0, -14);

    tankValueLabel_ = lv_label_create(scr);
    lv_label_set_text(tankValueLabel_, "—");
    lv_obj_set_style_text_color(tankValueLabel_, lv_color_white(), 0);
    lv_obj_set_style_text_font(tankValueLabel_, &lv_font_montserrat_20, 0);
    lv_obj_align(tankValueLabel_, LV_ALIGN_CENTER, 0, 6);
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
    syncArcs();
}

void ScrHome::cleanup()
{
    tankLabel_ = nullptr;
    tankValueLabel_ = nullptr;
    for (int i = 0; i < 4; i++)
        corners_[i] = Corner{};
    Scr::cleanup();
}

#endif /* SCREEN_MODE_CIRCLE */
