#include "device_lib_exports.h"

#ifdef SCREEN_MODE_CIRCLE

#include "circle_scrPresets.h"
#include "ui/ui.h"
#include "utils/util.h"
#include <BTOas.h>

ScrPresets scrPresets(true);

static lv_obj_t *presetLabels[5] = {};

/* ------------------------------------------------------------------ */
/*  Presets screen for 360x360 round display                          */
/*                                                                    */
/*  Design: Five circular preset buttons arranged in a ring around    */
/*  the center. Active preset is highlighted with theme color.        */
/*  Save and Load buttons at center bottom area.                      */
/*  All content within inscribed ~300px circle.                       */
/* ------------------------------------------------------------------ */

static void loadSelectedPreset()
{
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

static void presetBtnEventCb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int presetNum = (int)(intptr_t)lv_event_get_user_data(e);
        scrPresets.setPreset(presetNum);
    }
}

void ScrPresets::init(lv_obj_t *parent)
{
    Scr::init(parent);

    for (int i = 0; i < 5; i++) {
        presetLabels[i] = nullptr;
        presetBtns[i] = nullptr;
    }

    /* Title */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "PRESETS");
    lv_obj_set_style_text_color(title, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -130);

    /* 5 preset buttons arranged in a semicircular arc across the top/middle.
     * Positions calculated to stay within the round viewport.
     * Layout: buttons at angles around a 110px radius from center. */
    struct BtnPos { int x; int y; };
    BtnPos positions[] = {
        {-100,  -50},   // P1 - far left
        { -52,  -95},   // P2 - upper left
        {   0, -112},   // P3 - top center
        {  52,  -95},   // P4 - upper right
        { 100,  -50},   // P5 - far right
    };

    const int btnSz = 50;
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, btnSz, btnSz);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(GENERIC_GREY_DARK), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(THEME_COLOR_DARK), 0);
        lv_obj_set_style_shadow_width(btn, 12, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_40, 0);
        lv_obj_align(btn, LV_ALIGN_CENTER, positions[i].x, positions[i].y);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "%d", i + 1);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl);
        presetLabels[i] = lbl;

        lv_obj_add_event_cb(btn, presetBtnEventCb, LV_EVENT_CLICKED, (void *)(intptr_t)(i + 1));
        presetBtns[i] = btn;
    }

    /* Active preset label — large center display */
    lv_obj_t *activeLabel = lv_label_create(scr);
    lv_label_set_text(activeLabel, "Select a preset");
    lv_obj_set_style_text_color(activeLabel, lv_color_hex(GENERIC_GREY_LIGHT), 0);
    lv_obj_set_style_text_font(activeLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(activeLabel, LV_ALIGN_CENTER, 0, -6);

    /* Save / Load buttons centered in lower area */
    const int actionBtnW = 100;
    const int actionBtnH = 40;
    const int actionY = 60;

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

    int savedPreset = currentPreset;
    currentPreset = -1;
    setPreset(savedPreset > 0 ? savedPreset : 3);
}

void ScrPresets::updateButtonStyles()
{
    for (int i = 0; i < 5; i++) {
        if (!presetBtns[i]) continue;
        bool active = (i + 1 == currentPreset);
        lv_obj_set_style_bg_color(presetBtns[i], lv_color_hex(active ? THEME_COLOR_LIGHT : GENERIC_GREY_DARK), 0);
        lv_obj_set_style_border_color(presetBtns[i], lv_color_hex(active ? THEME_COLOR_LIGHT : THEME_COLOR_DARK), 0);
        if (active) {
            lv_obj_set_style_shadow_color(presetBtns[i], lv_color_hex(THEME_COLOR_MEDIUM), 0);
            lv_obj_set_style_shadow_opa(presetBtns[i], LV_OPA_70, 0);
            lv_obj_set_style_shadow_width(presetBtns[i], 20, 0);
        } else {
            lv_obj_set_style_shadow_opa(presetBtns[i], LV_OPA_40, 0);
            lv_obj_set_style_shadow_width(presetBtns[i], 12, 0);
        }
        if (presetLabels[i])
            lv_obj_set_style_text_color(presetLabels[i], lv_color_hex(active ? 0xFFFFFF : 0x8888AA), 0);
    }
}

void ScrPresets::setPreset(int num)
{
    if (currentPreset == num)
        showPresetDialog();
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

void ScrPresets::loop()
{
    Scr::loop();
}

void ScrPresets::cleanup()
{
    for (int i = 0; i < 5; i++) {
        presetBtns[i] = nullptr;
        presetLabels[i] = nullptr;
    }
    presetButtonsContainer = btnSave = btnLoad = nullptr;
    Scr::cleanup();
}

#endif /* SCREEN_MODE_CIRCLE */
