// ui_scrPresets.cpp — resolution-aware version (240x320 author space → any panel)
// Requires util.h to provide scaleCR/scaleSR, and your authored rects (ctr_preset_*, etc.)

#include "ui_scrPresets.h"
#include "ui/ui.h"     // for showMsgBox() etc.

LV_IMG_DECLARE(navbar_presets);
ScrPresets scrPresets(navbar_presets, true);

LV_IMG_DECLARE(presets_bg);
LV_IMG_DECLARE(img_car);
LV_IMG_DECLARE(img_wheels);
LV_IMG_DECLARE(selected_1);
LV_IMG_DECLARE(selected_2);
LV_IMG_DECLARE(selected_3);
LV_IMG_DECLARE(selected_4);
LV_IMG_DECLARE(selected_5);

// -------------------- small helpers --------------------
// Convert authored (240x320) scalar to display pixels via scaleSR()
static inline int sx(int x_ref){ SimpleRect r{(double)x_ref, 0, 0, 0}; return (int)scaleSR(r).x; }
static inline int sy(int y_ref){ SimpleRect r{0, (double)y_ref, 0, 0}; return (int)scaleSR(r).y; }

// -------------------- authored constants → scaled placement --------------------
// Center X stays true display-center because bitmaps are not scaled
static const int car_x    = DISPLAY_WIDTH  / 2 - img_car.header.w     / 2;
static const int wheels_x = DISPLAY_WIDTH  / 2 - img_wheels.header.w  / 2;

// Authored Y values (in 240x320) scaled to display height
static const int wheels_y = sy(88);
static const int car_y_1  = sy(88 - 21);
static const int car_y_2  = sy(88 - 21 - 4);
static const int car_y_3  = sy(88 - 21 - 8);
static const int car_y_4  = sy(88 - 21 - 12);
static const int car_y_5  = sy(88 - 21 - 16);

// Wheel-well cutout boxes are in *bitmap pixel space* (car image doesn’t scale)
static SimpleRect fender1Offset = {40, 37, 72 - 40, 63 - 37};
static SimpleRect fender2Offset = {166, 35, 199 - 166, 60 - 35};

// -------------------- animation callback --------------------
static void car_anim_func(lv_obj_t *obj, int32_t y)
{
    lv_obj_set_y(obj, y);
    // Keep wheel-well masks aligned to the car image (bitmap-space offsets)
    lv_obj_set_y(scrPresets.ww1, y + (lv_coord_t)fender1Offset.y);
    lv_obj_set_y(scrPresets.ww2, y + (lv_coord_t)fender2Offset.y);
}

static void animCarPreset(ScrPresets *scr, lv_coord_t end)
{
    const int duration_ms = 1000;
    const lv_coord_t start = lv_obj_get_y(scr->car);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, scr->car);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)car_anim_func);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_values(&a, start, end);
    lv_anim_start(&a);
}

// -------------------- lifecycle --------------------
void ScrPresets::init()
{
    Scr::init();

    // Remove the generic gray bg; this screen uses an image bg
    lv_obj_add_flag(this->rect_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_delete(this->rect_bg);

    // Background image, pinned to top center
    this->rect_bg = lv_image_create(this->scr);
    lv_image_set_src(this->rect_bg, &presets_bg);
    lv_obj_set_align(this->rect_bg, LV_ALIGN_TOP_MID);

    // Wheel well 1 (mask rectangle in absolute bitmap pixels)
    this->ww1 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww1);
    lv_obj_set_size(this->ww1, (lv_coord_t)fender1Offset.w, (lv_coord_t)fender1Offset.h);
    lv_obj_set_style_bg_color(this->ww1, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(this->ww1, LV_OPA_COVER, 0);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_x(this->ww1, car_x + (lv_coord_t)fender1Offset.x);
    lv_obj_set_y(this->ww1, car_y_1 + (lv_coord_t)fender1Offset.y);

    // Wheel well 2
    this->ww2 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww2);
    lv_obj_set_size(this->ww2, (lv_coord_t)fender2Offset.w, (lv_coord_t)fender2Offset.h);
    lv_obj_set_style_bg_color(this->ww2, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(this->ww2, LV_OPA_COVER, 0);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_x(this->ww2, car_x + (lv_coord_t)fender2Offset.x);
    lv_obj_set_y(this->ww2, car_y_1 + (lv_coord_t)fender2Offset.y);

    // Wheels image
    this->wheels = lv_image_create(this->scr);
    lv_image_set_src(this->wheels, &img_wheels);
    lv_obj_set_x(this->wheels, wheels_x);
    lv_obj_set_y(this->wheels, wheels_y);

    // Car image
    this->car = lv_image_create(this->scr);
    lv_image_set_src(this->car, &img_car);
    lv_obj_set_x(this->car, car_x);
    lv_obj_set_y(this->car, car_y_1);

    // Preset selector overlays (positions authored in 240x320 → scaled via scaleCR)
    this->btnPreset1 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset1, &selected_1);
    { CenterRect p = scaleCR(ctr_preset_1);
      lv_obj_set_x(this->btnPreset1, (lv_coord_t)(p.cx - selected_1.header.w / 2));
      lv_obj_set_y(this->btnPreset1, (lv_coord_t)(p.cy - selected_1.header.h / 2)); }

    this->btnPreset2 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset2, &selected_2);
    { CenterRect p = scaleCR(ctr_preset_2);
      lv_obj_set_x(this->btnPreset2, (lv_coord_t)(p.cx - selected_2.header.w / 2));
      lv_obj_set_y(this->btnPreset2, (lv_coord_t)(p.cy - selected_2.header.h / 2)); }

    this->btnPreset3 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset3, &selected_3);
    { CenterRect p = scaleCR(ctr_preset_3);
      lv_obj_set_x(this->btnPreset3, (lv_coord_t)(p.cx - selected_3.header.w / 2));
      lv_obj_set_y(this->btnPreset3, (lv_coord_t)(p.cy - selected_3.header.h / 2)); }

    this->btnPreset4 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset4, &selected_4);
    { CenterRect p = scaleCR(ctr_preset_4);
      lv_obj_set_x(this->btnPreset4, (lv_coord_t)(p.cx - selected_4.header.w / 2));
      lv_obj_set_y(this->btnPreset4, (lv_coord_t)(p.cy - selected_4.header.h / 2)); }

    this->btnPreset5 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset5, &selected_5);
    { CenterRect p = scaleCR(ctr_preset_5);
      lv_obj_set_x(this->btnPreset5, (lv_coord_t)(p.cx - selected_5.header.w / 2));
      lv_obj_set_y(this->btnPreset5, (lv_coord_t)(p.cy - selected_5.header.h / 2)); }

    // Bring HUD bits forward
    lv_obj_move_foreground(this->icon_navbar);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);

    this->hideSelectors();
    this->setPreset(3);
}

void ScrPresets::hideSelectors()
{
    lv_obj_add_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN);
}

void ScrPresets::setPreset(int num)
{
    if (currentPreset == num) {
        this->showPresetDialog();
    }
    currentPreset = num;
    hideSelectors();

    switch (num)
    {
        case 1: animCarPreset(this, (lv_coord_t)car_y_1); lv_obj_clear_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN); break;
        case 2: animCarPreset(this, (lv_coord_t)car_y_2); lv_obj_clear_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN); break;
        case 3: animCarPreset(this, (lv_coord_t)car_y_3); lv_obj_clear_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN); break;
        case 4: animCarPreset(this, (lv_coord_t)car_y_4); lv_obj_clear_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN); break;
        case 5: animCarPreset(this, (lv_coord_t)car_y_5); lv_obj_clear_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN); break;
    }

    requestPreset();
}

void ScrPresets::showPresetDialog()
{
    static char text[100];
    static char title[16];
    snprintf(text, sizeof(text),
             "  fd: %i                        fp: %i\n"
             "  rd: %i                        rp: %i",
             profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER],
             profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER],
             profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER],
             profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER]);

    snprintf(title, sizeof(title), "Preset %i", currentPreset);
    this->showMsgBox(title, text, NULL, "OK", []() -> void {}, []() -> void {}, false);
}

static void loadSelectedPreset()
{
    Serial.println("load preset");
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);

    if (!down) // just released
    {
        if (!this->isMsgBoxDisplayed())
        {
            if (cr_contains(ctr_preset_1, pos)) setPreset(1);
            if (cr_contains(ctr_preset_2, pos)) setPreset(2);
            if (cr_contains(ctr_preset_3, pos)) setPreset(3);
            if (cr_contains(ctr_preset_4, pos)) setPreset(4);
            if (cr_contains(ctr_preset_5, pos)) setPreset(5);

            if (sr_contains(preset_save, pos))
            {
                static char buf[48];
                snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
                this->showMsgBox(buf, NULL, "Confirm", "Cancel",
                    []() -> void {
                        Serial.println("save preset");
                        SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                        sendRestPacket(&pkt);
                        showDialog("Saved Preset!", lv_color_hex(THEME_COLOR_LIGHT));
                        requestPreset();
                    },
                    []() -> void {}, false);
            }

            if (sr_contains(preset_load, pos))
            {
                if (currentPreset == 1) {
                    currentScr->showMsgBox(
                        "Air out?",
                        "Preset 1 is typically air out. Please verify your car is not moving. Are you sure you wish to air out?",
                        "Confirm", "Cancel",
                        []() -> void { loadSelectedPreset(); },
                        []() -> void {}, false);
                } else {
                    loadSelectedPreset();
                }
            }
        }
    }
}

void ScrPresets::loop()
{
    Scr::loop();
}
