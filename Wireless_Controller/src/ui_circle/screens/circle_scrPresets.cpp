#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_scrPresets.h"
#include "ui/ui.h"
#include "utils/util.h"
#include <BTOas.h>

ScrPresets scrPresets(true);

/* ------------------------------------------------------------------ */
/*  Presets screen for 360x360 round display                          */
/*                                                                    */
/*  Single tap (after short delay): select/deselect. Second tap       */
/*  within window = double tap → PSI only (no toggle). Knob only      */
/*  when selected; encoder count synced while unselected.             */
/* ------------------------------------------------------------------ */

static constexpr int kPresetMin = 1;
static constexpr int kPresetMax = 5;
static constexpr uint32_t kPresetDoubleTapMs = 300;

static lv_timer_t *s_presetsSingleTapTimer = nullptr;

static void presetsSingleTapTimerCb(lv_timer_t *)
{
    s_presetsSingleTapTimer = nullptr;
    scrPresets.togglePresetCircleSelect();
}

static void loadSelectedPreset()
{
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

/** LVGL also sends SINGLE_CLICKED then CLICKED on each release; use CLICKED + timer so double-tap never toggles. */
static void centerPresetClickCb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    if (s_presetsSingleTapTimer) {
        lv_timer_delete(s_presetsSingleTapTimer);
        s_presetsSingleTapTimer = nullptr;
        if (currentPreset >= kPresetMin && currentPreset <= kPresetMax)
            scrPresets.showPresetDialog();
        return;
    }

    s_presetsSingleTapTimer = lv_timer_create(presetsSingleTapTimerCb, kPresetDoubleTapMs, nullptr);
    lv_timer_set_repeat_count(s_presetsSingleTapTimer, 1);
    lv_timer_set_auto_delete(s_presetsSingleTapTimer, true);
}

void ScrPresets::init(lv_obj_t *parent)
{
    Scr::init(parent);

    presetSelectBtn = nullptr;
    presetNumberLabel = nullptr;
    hintLabel = nullptr;
    btnSave = btnLoad = nullptr;

    hintLabel = lv_label_create(scr);
    lv_label_set_text(hintLabel, "Tap: on/off  ·  Double: PSI  ·  Knob when on");
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(hintLabel, LV_ALIGN_TOP_MID, 0, 36);

    const int circleSz = 100;
    presetSelectBtn = lv_btn_create(scr);
    lv_obj_set_size(presetSelectBtn, circleSz, circleSz);
    lv_obj_set_style_radius(presetSelectBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(presetSelectBtn, lv_color_hex(GENERIC_GREY_DARK), 0);
    lv_obj_set_style_bg_opa(presetSelectBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(presetSelectBtn, 3, 0);
    lv_obj_set_style_border_color(presetSelectBtn, lv_color_hex(THEME_COLOR_DARK), 0);
    lv_obj_set_style_shadow_width(presetSelectBtn, 16, 0);
    lv_obj_set_style_shadow_color(presetSelectBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(presetSelectBtn, LV_OPA_50, 0);
    lv_obj_align(presetSelectBtn, LV_ALIGN_CENTER, 0, -18);
    lv_obj_add_event_cb(presetSelectBtn, centerPresetClickCb, LV_EVENT_CLICKED, nullptr);

    presetNumberLabel = lv_label_create(presetSelectBtn);
    lv_label_set_text(presetNumberLabel, "3");
    lv_obj_set_style_text_font(presetNumberLabel, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(presetNumberLabel, lv_color_white(), 0);
    lv_obj_remove_flag(presetNumberLabel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(presetNumberLabel);

    /* Save / Load — below the circle */
    const int actionBtnW = 100;
    const int actionBtnH = 40;
    const int actionY = 72;

    btnSave = lv_btn_create(scr);
    lv_obj_set_size(btnSave, actionBtnW, actionBtnH);
    lv_obj_set_style_radius(btnSave, 20, 0);
    lv_obj_set_style_bg_color(btnSave, lv_color_hex(GENERIC_GREY_DARK), 0);
    lv_obj_set_style_border_width(btnSave, 1, 0);
    lv_obj_set_style_border_color(btnSave, lv_color_hex(THEME_COLOR_MEDIUM), 0);
    lv_obj_set_style_shadow_width(btnSave, 0, 0);
    lv_obj_align(btnSave, LV_ALIGN_CENTER, -58, actionY);
    lv_obj_t *sl = lv_label_create(btnSave);
    lv_label_set_text(sl, LV_SYMBOL_SAVE " Save");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
    lv_obj_center(sl);
    lv_obj_add_event_cb(btnSave, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            static char buf[48];
            snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
            scrPresets.showMsgBox(buf, nullptr, "Confirm", "Cancel",
                                  []() {
                                      SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                                      sendRestPacket(&pkt);
                                      showDialog("Saved!", lv_color_hex(THEME_COLOR_LIGHT));
                                      requestPreset();
                                  },
                                  []() {}, false);
        }
    }, LV_EVENT_CLICKED, nullptr);

    btnLoad = lv_btn_create(scr);
    lv_obj_set_size(btnLoad, actionBtnW, actionBtnH);
    lv_obj_set_style_radius(btnLoad, 20, 0);
    lv_obj_set_style_bg_color(btnLoad, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_shadow_width(btnLoad, 0, 0);
    lv_obj_align(btnLoad, LV_ALIGN_CENTER, 58, actionY);
    lv_obj_t *ll = lv_label_create(btnLoad);
    lv_label_set_text(ll, LV_SYMBOL_UPLOAD " Load");
    lv_obj_set_style_text_color(ll, lv_color_white(), 0);
    lv_obj_set_style_text_font(ll, &lv_font_montserrat_14, 0);
    lv_obj_center(ll);
    lv_obj_add_event_cb(btnLoad, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            if (currentPreset == 1) {
                scrPresets.showMsgBox("Air out?", "Preset 1 is typically air out. Verify car is not moving.", "Confirm", "Cancel",
                                      []() { loadSelectedPreset(); }, []() {}, false);
            } else {
                loadSelectedPreset();
            }
        }
    }, LV_EVENT_CLICKED, nullptr);

#ifdef HAS_ROTARY_ENCODER
    if (g_knob_handle)
        lastKnobCount_ = iot_knob_get_count_value(g_knob_handle);
#endif

    presetCircleSelected_ = false;

    int savedPreset = currentPreset;
    currentPreset = -1;
    setPreset(savedPreset > 0 ? savedPreset : 3);
}

void ScrPresets::updateButtonStyles()
{
    if (!presetSelectBtn || !presetNumberLabel)
        return;

    const bool ok = (currentPreset >= kPresetMin && currentPreset <= kPresetMax);
    lv_label_set_text_fmt(presetNumberLabel, "%d", ok ? currentPreset : 0);

    if (presetCircleSelected_) {
        lv_obj_set_style_bg_color(presetSelectBtn, lv_color_hex(THEME_COLOR_LIGHT), 0);
        lv_obj_set_style_border_color(presetSelectBtn, lv_color_hex(THEME_COLOR_LIGHT), 0);
        lv_obj_set_style_shadow_color(presetSelectBtn, lv_color_hex(THEME_COLOR_MEDIUM), 0);
        lv_obj_set_style_shadow_opa(presetSelectBtn, LV_OPA_70, 0);
        lv_obj_set_style_shadow_width(presetSelectBtn, 22, 0);
        lv_obj_set_style_text_color(presetNumberLabel, lv_color_hex(0xFFFFFF), 0);
    } else {
        lv_obj_set_style_bg_color(presetSelectBtn, lv_color_hex(GENERIC_GREY_DARK), 0);
        lv_obj_set_style_border_color(presetSelectBtn, lv_color_hex(THEME_COLOR_DARK), 0);
        lv_obj_set_style_shadow_color(presetSelectBtn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(presetSelectBtn, LV_OPA_50, 0);
        lv_obj_set_style_shadow_width(presetSelectBtn, 16, 0);
        lv_obj_set_style_text_color(presetNumberLabel, lv_color_hex(0xAAAAAA), 0);
    }
}

void ScrPresets::togglePresetCircleSelect()
{
    presetCircleSelected_ = !presetCircleSelected_;
#ifdef HAS_ROTARY_ENCODER
    if (presetCircleSelected_ && g_knob_handle)
        lastKnobCount_ = iot_knob_get_count_value(g_knob_handle);
#endif
    updateButtonStyles();
}

void ScrPresets::setPreset(int num)
{
    currentPreset = num;
    updateButtonStyles();
    requestPreset();
}

void ScrPresets::showPresetDialog()
{
    static char text[100];
    static char titleBuf[10];
    snprintf(text, sizeof(text), "  fd: %i                        fp: %i\n  rd: %i                        rp: %i",
             profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER], profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER],
             profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER], profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER]);
    snprintf(titleBuf, sizeof(titleBuf), "Preset %i", currentPreset);
    showMsgBox(titleBuf, text, nullptr, "OK", []() {}, []() {}, false);
}

#ifdef HAS_ROTARY_ENCODER
void ScrPresets::processKnob()
{
    if (!g_knob_handle)
        return;

    int current = iot_knob_get_count_value(g_knob_handle);
    if (!presetCircleSelected_) {
        lastKnobCount_ = current;
        return;
    }

    int delta = current - lastKnobCount_;
    if (delta == 0)
        return;

    lastKnobCount_ = current;

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
}
#endif

void ScrPresets::loop()
{
    Scr::loop();
#ifdef HAS_ROTARY_ENCODER
    processKnob();
#endif
}

void ScrPresets::cleanup()
{
    if (s_presetsSingleTapTimer) {
        lv_timer_delete(s_presetsSingleTapTimer);
        s_presetsSingleTapTimer = nullptr;
    }
    presetCircleSelected_ = false;
    presetSelectBtn = nullptr;
    presetNumberLabel = nullptr;
    hintLabel = nullptr;
    btnSave = btnLoad = nullptr;
    Scr::cleanup();
}

#endif /* SCREEN_MODE_CIRCLE */
