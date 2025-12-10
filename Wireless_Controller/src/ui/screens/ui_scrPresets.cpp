#include "ui_scrPresets.h"
#include "utils/util.h"   // for SCALE_X, SCALE_Y, SimpleRect, CenterRect, etc.

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

// ---------- Design-space helpers (original 240x320 layout) ----------

static inline lv_coord_t DESIGN_X(float x) { return (lv_coord_t)(x * SCALE_X + 0.5f); }
static inline lv_coord_t DESIGN_Y(float y) { return (lv_coord_t)(y * SCALE_Y + 0.5f); }

// Runtime-computed geometry (scaled)
static lv_coord_t car_x;
static lv_coord_t wheels_x;
static lv_coord_t wheels_y;
static lv_coord_t car_y_1;
static lv_coord_t car_y_2;
static lv_coord_t car_y_3;
static lv_coord_t car_y_4;
static lv_coord_t car_y_5;

static SimpleRect fender1Offset;
static SimpleRect fender2Offset;

// square 1: 40,37 -> 71, 63
// square 2: 166,35 -> 198, 60

static void compute_geometry()
{
    // Center X positions based on *scaled* image widths, then apply X offset
    const float design_x_offset = CAR_PAGE_OFFSET_DESIGN_X;

    car_x = LCD_WIDTH / 2
          - (lv_coord_t)((img_car.header.w * SCALE_X) / 2.0f)
          + DESIGN_X(design_x_offset);

    wheels_x = LCD_WIDTH / 2
             - (lv_coord_t)((img_wheels.header.w * SCALE_X) / 2.0f)
             + DESIGN_X(design_x_offset);

    // Base design-space vertical positions, plus optional Y offset
    const float design_wheels_y   = 88.0f + CAR_PAGE_OFFSET_DESIGN_Y;
    const float design_car_delta  = 21.0f;
    const float design_step_delta = 4.0f;

    wheels_y = DESIGN_Y(design_wheels_y);
    car_y_1  = wheels_y - DESIGN_Y(design_car_delta);
    car_y_2  = car_y_1  - DESIGN_Y(design_step_delta);
    car_y_3  = car_y_2  - DESIGN_Y(design_step_delta);
    car_y_4  = car_y_3  - DESIGN_Y(design_step_delta);
    car_y_5  = car_y_4  - DESIGN_Y(design_step_delta);

    // Fender cutouts (original design rects, scaled)
    // fender1: (40,37) -> (72,63)
    fender1Offset.x = 40.0f * SCALE_X;
    fender1Offset.y = 37.0f * SCALE_Y;
    fender1Offset.w = (72.0f - 40.0f) * SCALE_X;
    fender1Offset.h = (63.0f - 37.0f) * SCALE_Y;

    // fender2: (166,35) -> (199,60)
    fender2Offset.x = 166.0f * SCALE_X;
    fender2Offset.y = 35.0f  * SCALE_Y;
    fender2Offset.w = (199.0f - 166.0f) * SCALE_X;
    fender2Offset.h = (60.0f  - 35.0f)  * SCALE_Y;
}

void car_anim_func(lv_obj_t *obj, int32_t y)
{
    lv_obj_set_y(obj, y);
    lv_obj_set_y(scrPresets.ww1, y + (lv_coord_t)fender1Offset.y);
    lv_obj_set_y(scrPresets.ww2, y + (lv_coord_t)fender2Offset.y);
    // lv_obj_move_foreground(scrPresets.wheels);
}

void animCarPreset(ScrPresets *scr, lv_coord_t end)
{
    int duration = 1000;                       // animation duration in ms
    lv_coord_t start = lv_obj_get_y(scr->car); // start is the current location
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)car_anim_func);
    lv_anim_set_var(&a, scr->car);
    lv_anim_set_time(&a, duration);
    lv_anim_set_values(&a, start, end);
    lv_anim_start(&a);
}

void ScrPresets::init()
{
    Scr::init();

    // Set screen background so any gaps between image and navbar
    // blend with the presets page instead of showing white.
    lv_obj_set_style_bg_color(this->scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(this->scr, LV_OPA_COVER, 0);

    // Compute our scaled geometry now that LCD size & SCALE_* are valid
    compute_geometry();

    lv_obj_add_flag(this->rect_bg, LV_OBJ_FLAG_HIDDEN); // hide grey background, we have a different one on this page
    lv_obj_delete(this->rect_bg);

    // background image
    this->rect_bg = lv_image_create(this->scr);
    lv_image_set_src(this->rect_bg, &presets_bg);
    scale_img(this->rect_bg, presets_bg);
    lv_obj_set_align(this->rect_bg, LV_ALIGN_TOP_MID);

    // wheel well 1
    this->ww1 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww1);
    lv_obj_get_style_border_width(this->ww1, 0);

    // fender1Offset.w/h are scaled; scale_obj expects design dims -> divide back out
    lv_obj_set_size(this->ww1, (lv_coord_t)fender1Offset.w, (lv_coord_t)fender1Offset.h);
    scale_obj(this->ww1,
              (int)(fender1Offset.w / SCALE_X),
              (int)(fender1Offset.h / SCALE_Y));

    lv_obj_set_style_bg_color(this->ww1, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(this->ww1, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)); /// Flags
    lv_obj_set_style_bg_opa(this->ww1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->ww1, car_x + (lv_coord_t)fender1Offset.x);
    lv_obj_set_y(this->ww1, car_y_1 + (lv_coord_t)fender1Offset.y);

    // wheel well 2
    this->ww2 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww2);
    lv_obj_get_style_border_width(this->ww2, 0);

    lv_obj_set_size(this->ww2, (lv_coord_t)fender2Offset.w, (lv_coord_t)fender2Offset.h);
    scale_obj(this->ww2,
              (int)(fender2Offset.w / SCALE_X),
              (int)(fender2Offset.h / SCALE_Y));

    lv_obj_set_style_bg_color(this->ww2, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(this->ww2, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)); /// Flags
    lv_obj_set_style_bg_opa(this->ww2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->ww2, car_x + (lv_coord_t)fender2Offset.x);
    lv_obj_set_y(this->ww2, car_y_1 + (lv_coord_t)fender2Offset.y);

    // wheels
    this->wheels = lv_image_create(this->scr);
    lv_image_set_src(this->wheels, &img_wheels);
    scale_img(this->wheels, img_wheels);
    lv_obj_set_x(this->wheels, wheels_x);
    lv_obj_set_y(this->wheels, wheels_y);

    // car
    this->car = lv_image_create(this->scr);
    lv_image_set_src(this->car, &img_car);
    scale_img(this->car, img_car);
    lv_obj_set_x(this->car, car_x);
    lv_obj_set_y(this->car, car_y_1);

    // ---------- Preset selector overlays ----------
    // We assume ctr_preset_* are already in *device* coordinates.
    // We only adjust for the scaled image sizes when centering
    // and apply a global overlay offset in design units.

    const float sel1_w = selected_1.header.w * SCALE_X;
    const float sel1_h = selected_1.header.h * SCALE_Y;
    const float sel2_w = selected_2.header.w * SCALE_X;
    const float sel2_h = selected_2.header.h * SCALE_Y;
    const float sel3_w = selected_3.header.w * SCALE_X;
    const float sel3_h = selected_3.header.h * SCALE_Y;
    const float sel4_w = selected_4.header.w * SCALE_X;
    const float sel4_h = selected_4.header.h * SCALE_Y;
    const float sel5_w = selected_5.header.w * SCALE_X;
    const float sel5_h = selected_5.header.h * SCALE_Y;

    // Global offset for all overlays (converted from design -> device coords)
    const lv_coord_t ov_dx = DESIGN_X(PRESET_OVERLAY_OFFSET_DESIGN_X);
    const lv_coord_t ov_dy = DESIGN_Y(PRESET_OVERLAY_OFFSET_DESIGN_Y);

    this->btnPreset1 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset1, &selected_1);
    scale_img(this->btnPreset1, selected_1);
    lv_obj_set_x(this->btnPreset1,
                 (lv_coord_t)(ctr_preset_1.cx - sel1_w / 2.0f) + ov_dx);
    lv_obj_set_y(this->btnPreset1,
                 (lv_coord_t)(ctr_preset_1.cy - sel1_h / 2.0f) + ov_dy);

    this->btnPreset2 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset2, &selected_2);
    scale_img(this->btnPreset2, selected_2);
    lv_obj_set_x(this->btnPreset2,
                 (lv_coord_t)(ctr_preset_2.cx - sel2_w / 2.0f) + ov_dx);
    lv_obj_set_y(this->btnPreset2,
                 (lv_coord_t)(ctr_preset_2.cy - sel2_h / 2.0f) + ov_dy);

    this->btnPreset3 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset3, &selected_3);
    scale_img(this->btnPreset3, selected_3);
    lv_obj_set_x(this->btnPreset3,
                 (lv_coord_t)(ctr_preset_3.cx - sel3_w / 2.0f) + ov_dx);
    lv_obj_set_y(this->btnPreset3,
                 (lv_coord_t)(ctr_preset_3.cy - sel3_h / 2.0f) + ov_dy);

    this->btnPreset4 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset4, &selected_4);
    scale_img(this->btnPreset4, selected_4);
    lv_obj_set_x(this->btnPreset4,
                 (lv_coord_t)(ctr_preset_4.cx - sel4_w / 2.0f) + ov_dx);
    lv_obj_set_y(this->btnPreset4,
                 (lv_coord_t)(ctr_preset_4.cy - sel4_h / 2.0f) + ov_dy);

    this->btnPreset5 = lv_image_create(this->scr);
    lv_image_set_src(this->btnPreset5, &selected_5);
    scale_img(this->btnPreset5, selected_5);
    lv_obj_set_x(this->btnPreset5,
                 (lv_coord_t)(ctr_preset_5.cx - sel5_w / 2.0f) + ov_dx);
    lv_obj_set_y(this->btnPreset5,
                 (lv_coord_t)(ctr_preset_5.cy - sel5_h / 2.0f) + ov_dy);


    lv_obj_move_foreground(this->icon_navbar);                  // bring navbar to foreground
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger); // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);  // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);    // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);     // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureTank);           // pressures to foreground front

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
    if (currentPreset == num)
    {
        this->showPresetDialog();
    }
    currentPreset = num;
    hideSelectors();

    switch (num)
    {
    case 1:
        animCarPreset(this, car_y_1);
        lv_obj_remove_flag(btnPreset1, LV_OBJ_FLAG_HIDDEN);
        break;
    case 2:
        animCarPreset(this, car_y_2);
        lv_obj_remove_flag(btnPreset2, LV_OBJ_FLAG_HIDDEN);
        break;
    case 3:
        animCarPreset(this, car_y_3);
        lv_obj_remove_flag(btnPreset3, LV_OBJ_FLAG_HIDDEN);
        break;
    case 4:
        animCarPreset(this, car_y_4);
        lv_obj_remove_flag(btnPreset4, LV_OBJ_FLAG_HIDDEN);
        break;
    case 5:
        animCarPreset(this, car_y_5);
        lv_obj_remove_flag(btnPreset5, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    requestPreset();
}

void ScrPresets::showPresetDialog()
{
    static char text[100];
    static char title[10];

    snprintf(text, sizeof(text),
             "  fd: %i                        fp: %i\n  rd: %i                        rp: %i",
             profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER],
             profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER],
             profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER],
             profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER]);
    snprintf(title, sizeof(title), "Preset %i", currentPreset);

    this->showMsgBox(title, text, NULL, "OK", []() -> void {}, []() -> void {}, false);
}

void loadSelectedPreset()
{
    Serial.println("load preset");
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos, down);
    if (down == false) // just released on button
    {
        if (!this->isMsgBoxDisplayed())
        {
            // Hit-testing in device coordinates; ctr_preset_* and preset_* assumed device-space.
            if (cr_contains(ctr_preset_1, pos))
            {
                setPreset(1);
            }
            if (cr_contains(ctr_preset_2, pos))
            {
                setPreset(2);
            }
            if (cr_contains(ctr_preset_3, pos))
            {
                setPreset(3);
            }
            if (cr_contains(ctr_preset_4, pos))
            {
                setPreset(4);
            }
            if (cr_contains(ctr_preset_5, pos))
            {
                setPreset(5);
            }
            if (sr_contains(preset_save, pos))
            {
                static char buf[40];
                snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
                this->showMsgBox(buf, NULL, "Confirm", "Cancel",
                                 []() -> void
                                 {
                                     Serial.println("save preset");
                                     SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                                     sendRestPacket(&pkt);
                                     showDialog("Saved Preset!", lv_color_hex(THEME_COLOR_LIGHT));
                                     requestPreset(); // update data now that it's saved
                                 },
                                 []() -> void {}, false);
            }
            if (sr_contains(preset_load, pos))
            {
                if (currentPreset == 1)
                {
                    currentScr->showMsgBox(
                        "Air out?",
                        "Preset 1 is typically air out. Please verify your car is not moving. Are you sure you wish to air out?",
                        "Confirm", "Cancel",
                        []() -> void { loadSelectedPreset(); },
                        []() -> void {},
                        false);
                }
                else
                {
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
