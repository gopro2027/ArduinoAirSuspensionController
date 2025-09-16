// ui_scrPresets.cpp — 240x320 authored → 320x480 panel, fixed 49px navbar
// Background now uses bg_remote image (no gradients). No image scaling is used.

#include <math.h>
#include "lvgl.h"
#include "ui_scrPresets.h"
#include "ui/ui.h"
#include "utils/util.h"   // SimpleRect, CenterRect, sr_contains/cr_contains if you still need them
#include "ui/components/ui_navbar.h"  // navbar_create()

// ----- Assets we still use -----
LV_IMG_DECLARE(navbar_presets);
LV_IMG_DECLARE(car_remote);
LV_IMG_DECLARE(wheels_remote);
LV_IMG_DECLARE(bg_remote);       // NEW: background image

// Global instance (project pattern)
ScrPresets scrPresets(navbar_presets, true);

// ===================== Authoring & panel geometry =====================
static constexpr int AUTH_W          = 240;
static constexpr int AUTH_H          = 320;
static constexpr int NAV_AUTH_H      = 49;                      // authored nav height
static constexpr int AUTH_CONTENT_H  = AUTH_H - NAV_AUTH_H;     // 271 (content only)

static constexpr int NAV_FINAL_H     = 49;                      // final navbar

static inline lv_coord_t CONTENT_W() { return DISPLAY_WIDTH; }                 // 320
static inline lv_coord_t CONTENT_H() { return DISPLAY_HEIGHT - NAV_FINAL_H; }  // 431

// 240 → 320, so scale = 1.333333...
static inline float      SCALE_F()       { return (float)DISPLAY_WIDTH / (float)AUTH_W; }
static inline lv_coord_t S(lv_coord_t v) { return (lv_coord_t)lroundf((float)v * SCALE_F()); }

static inline lv_coord_t STAGE_VIS_H() { return S(AUTH_CONTENT_H); }           // ~361
static inline lv_coord_t STAGE_Y()     { return (CONTENT_H() - STAGE_VIS_H()) / 2; }

// keep stage y if you still map touches anywhere manually (we won’t need it here)
static lv_coord_t g_stage_y = 0;

// -------------------- Authored placements --------------------
static inline int car_x_auth()    { return (AUTH_W - (int)car_remote.header.w) / 2; }
static inline int wheels_x_auth() { return (AUTH_W - (int)wheels_remote.header.w) / 2; }

static const int wheels_y_auth = 88;
static const int car_y_1_auth  = 88 - 21;
static const int car_y_2_auth  = 88 - 21 - 4;
static const int car_y_3_auth  = 88 - 21 - 8;
static const int car_y_4_auth  = 88 - 21 - 12;
static const int car_y_5_auth  = 88 - 21 - 16;

static constexpr lv_coord_t CENTER_X = 160;
static constexpr lv_coord_t CENTER_Y = 175;

// Wheel-well fixed X (screen coords) + baseline Y (screen coord)



// Computed after stage is placed
// static lv_coord_t g_ww_y_stage_base = 0;   // baseline Y in stage coords

static lv_coord_t g_car_left = 0;      // car top-left X at preset 3 baseline
static lv_coord_t g_car_top_base = 0;  // car top-left Y at preset 3 baseline
static lv_coord_t g_ctrl_shift_y = 0;

static inline lv_coord_t preset_dy_px(int preset){
    switch(preset){
        case 1: return S(car_y_1_auth - car_y_3_auth);
        case 2: return S(car_y_2_auth - car_y_3_auth);
        case 3: return 0;
        case 4: return S(car_y_4_auth - car_y_3_auth);
        case 5: return S(car_y_5_auth - car_y_3_auth);
        default: return 0;
    }
}

static constexpr lv_coord_t WW_W = 46;
static constexpr lv_coord_t WW_H = 35;

// Offsets inside the car bitmap (top-left anchors)
static constexpr lv_coord_t WW1_X_OFF = 51;
static constexpr lv_coord_t WW2_X_OFF = 211;
static constexpr lv_coord_t WW_Y_OFF  = 78;   // top edge inside the car image

static inline lv_coord_t WW_TOP_FROM_CENTER(lv_coord_t center){ return center - WW_H/2; }


// Wheel-well cutouts in *car bitmap* pixels (car art measured at 240×79 px)
static const SimpleRect FENDER1_AUTH = {40, 37, 36, 28}; // wider/taller
static const SimpleRect FENDER2_AUTH = {166, 35, 36, 28};

static SimpleRect g_fender1 = {0,0,0,0};
static SimpleRect g_fender2 = {0,0,0,0};

// -------------------- Styles --------------------
static lv_style_t st_title;

static lv_style_t st_num_btn;       // unselected: transparent circle
static lv_style_t st_num_btn_sel;   // selected: filled purple circle
static lv_style_t st_num_lbl_unsel;
static lv_style_t st_num_lbl_sel;

static lv_style_t st_action_btn;    // Save/Load buttons
static lv_style_t st_action_lbl;

// Helper to init styles once
static void init_styles_once()
{
    static bool inited = false;
    if(inited) return;
    inited = true;

    /* ===================== Title style ===================== */
    lv_style_init(&st_title);
    lv_style_set_text_color(&st_title, lv_color_hex(0xEDEDED));
    lv_style_set_text_font(&st_title, &lv_font_montserrat_20);

    /* ===================== Number button styles ===================== */
    // Transparent circle (unselected)
    lv_style_init(&st_num_btn);
    lv_style_set_radius(&st_num_btn, LV_RADIUS_CIRCLE);
    lv_style_set_bg_opa(&st_num_btn, LV_OPA_TRANSP);
    lv_style_set_border_width(&st_num_btn, 0);
    lv_style_set_pad_all(&st_num_btn, S(6));

    // Filled purple circle (selected)
    lv_style_init(&st_num_btn_sel);
    lv_style_set_radius(&st_num_btn_sel, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&st_num_btn_sel, lv_color_hex(0xBB86FC));
    lv_style_set_bg_opa(&st_num_btn_sel, LV_OPA_COVER);
    lv_style_set_border_width(&st_num_btn_sel, 0);
    lv_style_set_pad_all(&st_num_btn_sel, S(6));

    // Labels (you already flip between these in apply_num_styles)
    lv_style_init(&st_num_lbl_unsel);
    lv_style_set_text_color(&st_num_lbl_unsel, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&st_num_lbl_unsel, &lv_font_montserrat_20);

    lv_style_init(&st_num_lbl_sel);
    lv_style_set_text_color(&st_num_lbl_sel, lv_color_hex(0x171717));
    lv_style_set_text_font(&st_num_lbl_sel, &lv_font_montserrat_20);

    /* ===================== Action buttons ===================== */
    lv_style_init(&st_action_btn);
    lv_style_set_radius(&st_action_btn, S(10));
    lv_style_set_bg_color(&st_action_btn, lv_color_hex(0xBB86FC));
    lv_style_set_bg_opa(&st_action_btn, LV_OPA_COVER);
    lv_style_set_pad_hor(&st_action_btn, S(14));
    lv_style_set_pad_ver(&st_action_btn, S(10));

    lv_style_init(&st_action_lbl);
    lv_style_set_text_color(&st_action_lbl, lv_color_hex(0x171717));
    lv_style_set_text_font(&st_action_lbl, &lv_font_montserrat_20);
}

// -------------------- animation callback --------------------
static void car_anim_func(lv_obj_t *obj, int32_t y)
{
    // Move the car vertically
    lv_obj_set_y(obj, y);

    // Keep wheel-wells at fixed offsets inside the car bitmap
    const lv_coord_t car_x = lv_obj_get_x(obj);
    lv_obj_set_pos(scrPresets.ww1, car_x + WW1_X_OFF, y + WW_Y_OFF);
    lv_obj_set_pos(scrPresets.ww2, car_x + WW2_X_OFF, y + WW_Y_OFF);
}
static void animCarPreset(ScrPresets *scr, lv_coord_t end_scaled_y)
{
    const int duration_ms = 1000;
    const lv_coord_t start_scaled_y = lv_obj_get_y(scr->car);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, scr->car);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)car_anim_func);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_values(&a, start_scaled_y, end_scaled_y);
    lv_anim_start(&a);
}

// =============== Button callbacks ===============
static void evt_preset_btn(lv_event_t * e)
{
    auto * btn = (lv_obj_t *)lv_event_get_target(e);
    int num = (int)(intptr_t)lv_event_get_user_data(e);
    (void)btn;
    scrPresets.setPreset(num);
}

static void evt_save_btn(lv_event_t * e)
{
    (void)e;
    static char buf[48];
    snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
    scrPresets.showMsgBox(buf, NULL, "Confirm", "Cancel",
        []() -> void {
            SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
            sendRestPacket(&pkt);
            showDialog("Saved Preset!", lv_color_hex(THEME_COLOR_LIGHT));
            requestPreset();
        },
        []() -> void {}, false);
}

static void loadSelectedPreset()
{
    AirupQuickPacket pkt(currentPreset - 1);
    sendRestPacket(&pkt);
    showDialog("Loaded Preset!", lv_color_hex(0x22bb33));
}

static void evt_load_btn(lv_event_t * e)
{
    (void)e;
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

// ====================== lifecycle ======================
void ScrPresets::init()
{
    Scr::init();
    init_styles_once();

//     // ====== Navbar (fixed 320×49) ======
//     lv_obj_t *nav = lv_image_create(this->scr);
//     lv_image_set_src(nav, &navbar_presets);
//     lv_obj_set_size(nav, DISPLAY_WIDTH, NAV_FINAL_H);
// #if LVGL_VERSION_MAJOR >= 9
//     lv_image_set_align(nav, LV_IMAGE_ALIGN_STRETCH);
// #endif
//     lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
//     this->icon_navbar = nav;

    this->icon_navbar = navbar_create(this->scr, NAV_PRESETS);

    // ====== Content root (fills above navbar) ======
    lv_obj_t *content = lv_obj_create(this->scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, CONTENT_W(), CONTENT_H());
    lv_obj_set_pos(content, 0, 0);

    // --- Background image (exact 320x431) ---
    lv_obj_t *bg = lv_image_create(content);
    lv_image_set_src(bg, &bg_remote);

    // Use native size (no scaling) and pin to top-left of 'content'
    lv_obj_set_size(bg, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, 0);

    // Make sure it's inert and behind everything else
    lv_obj_clear_flag(bg,
        static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));    lv_obj_move_background(bg);

    // ====== Stage (centered vertically; holds authored content) ======
    lv_obj_t *stage = lv_obj_create(content);
    lv_obj_remove_style_all(stage);
    lv_obj_set_size(stage, CONTENT_W(), STAGE_VIS_H());
    g_stage_y = STAGE_Y();
    lv_obj_set_pos(stage, 0, g_stage_y);

    // Authored fender rects (no scaling now)
    g_fender1 = FENDER1_AUTH;
    g_fender2 = FENDER2_AUTH;

    // Compute center in stage coords
    const lv_coord_t center_y_stage = CENTER_Y - g_stage_y;

    /* -------- Car (no scaling), center at (160,175) baseline (preset 3) -------- */
    this->car = lv_image_create(stage);
    lv_image_set_src(this->car, &car_remote);
    lv_image_set_pivot(this->car, 0, 0);
    lv_obj_set_size(this->car, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // baseline top-left for "center at (160,175)"
    g_car_left     = CENTER_X - (lv_coord_t)car_remote.header.w / 2;
    g_car_top_base = center_y_stage - (lv_coord_t)car_remote.header.h / 2;
    lv_obj_set_pos(this->car, g_car_left, g_car_top_base);
    const lv_coord_t car_bottom_stage = g_car_top_base + (lv_coord_t)car_remote.header.h;
    const lv_coord_t desired_title_y  = car_bottom_stage + S(-10);     // 12px gap below car
    const lv_coord_t authored_title_y = S(130);
    g_ctrl_shift_y = (desired_title_y > authored_title_y) ? (desired_title_y - authored_title_y) : 0;

    /* -------- Wheels (no scaling), also centered at (160,175) -------- */
    this->wheels = lv_image_create(stage);
    lv_image_set_src(this->wheels, &wheels_remote);
    lv_image_set_pivot(this->wheels, 0, 0);
    lv_obj_set_size(this->wheels, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(this->wheels,
        CENTER_X - (lv_coord_t)wheels_remote.header.w / 2,
        center_y_stage - (lv_coord_t)wheels_remote.header.h / 2);

    /* -------- Wheel-well masks (solid), positioned RELATIVE TO car (no S()) -------- */
    this->ww1 = lv_obj_create(stage);
    lv_obj_remove_style_all(this->ww1);
    // lv_obj_set_size(this->ww1, (lv_coord_t)g_fender1.w, (lv_coord_t)g_fender1.h);
    lv_obj_set_size(this->ww1, WW_W, WW_H);
    lv_obj_set_style_bg_color(this->ww1, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(this->ww1, LV_OPA_COVER, 0);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(this->ww1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(this->ww1, g_car_left + WW1_X_OFF, g_car_top_base + WW_Y_OFF);

    this->ww2 = lv_obj_create(stage);
    lv_obj_remove_style_all(this->ww2);
    // lv_obj_set_size(this->ww2, (lv_coord_t)g_fender2.w, (lv_coord_t)g_fender2.h);
    lv_obj_set_size(this->ww2, WW_W, WW_H);
    lv_obj_set_style_bg_color(this->ww2, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(this->ww2, LV_OPA_COVER, 0);
    lv_obj_remove_flag(this->ww2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(this->ww2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(this->ww2, g_car_left + WW2_X_OFF, g_car_top_base + WW_Y_OFF);

    // Layering
    lv_obj_move_background(this->ww1);
    lv_obj_move_background(this->ww2);
    lv_obj_move_foreground(this->wheels);
    lv_obj_move_foreground(this->car);


    // ====== Title "Preset" ======
    lv_obj_t *lblTitle = lv_label_create(stage);
    lv_obj_add_style(lblTitle, &st_title, 0);
    lv_label_set_text(lblTitle, "Preset");
    lv_obj_set_pos(lblTitle, S(16), S(130) + g_ctrl_shift_y);  // <-- shifted

    // ====== Number buttons (1..5) ======
    const lv_coord_t num_btn_diam_auth = 36;    // ~27px authored → ~36px final
    const lv_coord_t gap_auth          = 10;    // horizontal gap
    const lv_coord_t first_x_auth      = 10;    // left inset for "1"
    const lv_coord_t row_y_auth        = 160;

    lv_obj_t * nums[5];
    for(int i=0;i<5;i++){
        lv_obj_t *b = lv_button_create(stage);
        lv_obj_remove_style_all(b);
        lv_obj_add_style(b, &st_num_btn, 0);
        lv_obj_set_size(b, S(num_btn_diam_auth), S(num_btn_diam_auth));
        lv_obj_set_pos(b,
            S(first_x_auth + i*(num_btn_diam_auth + gap_auth)),
            S(row_y_auth) + g_ctrl_shift_y);
        lv_obj_add_event_cb(b, evt_preset_btn, LV_EVENT_CLICKED, (void*)(intptr_t)(i+1));

        lv_obj_t *lbl = lv_label_create(b);
        char txt[2] = {(char)('0' + (i+1)), 0};
        lv_label_set_text(lbl, txt);
        lv_obj_center(lbl);

        // assign unselected label style initially
        lv_obj_add_style(lbl, &st_num_lbl_unsel, 0);

        nums[i] = b;
    }

    // Save for styling in setPreset()
    this->btnPreset1 = nums[0];
    this->btnPreset2 = nums[1];
    this->btnPreset3 = nums[2];
    this->btnPreset4 = nums[3];
    this->btnPreset5 = nums[4];

    // ====== Save / Load buttons ======
    // "Save preset" link-like
    lv_obj_t *btnSave = lv_button_create(stage);
    lv_obj_remove_style_all(btnSave);
    lv_obj_set_size(btnSave, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(btnSave, S(16), S(210) + g_ctrl_shift_y);    // <-- shifted
    lv_obj_add_event_cb(btnSave, evt_save_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lblSave = lv_label_create(btnSave);
    lv_label_set_text(lblSave, "Save preset");
    lv_obj_add_style(lblSave, &st_title, 0);
    static lv_style_t st_uline;
    static bool st_uline_inited = false;
    if(!st_uline_inited) {
        lv_style_init(&st_uline);
        lv_style_set_text_decor(&st_uline, LV_TEXT_DECOR_UNDERLINE);
        st_uline_inited = true;
    }
    lv_obj_add_style(lblSave, &st_uline, 0);

    // "Load preset" pill button
    lv_obj_t *btnLoad = lv_button_create(stage);
    lv_obj_remove_style_all(btnLoad);
    lv_obj_add_style(btnLoad, &st_action_btn, 0);
    lv_obj_set_pos(btnLoad, S(120), S(204) + g_ctrl_shift_y);   // <-- shifted
    lv_obj_add_event_cb(btnLoad, evt_load_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lblLoad = lv_label_create(btnLoad);
    lv_obj_add_style(lblLoad, &st_action_lbl, 0);
    lv_label_set_text(lblLoad, "Load preset");
    lv_obj_center(lblLoad);

    // Bring HUD bits forward (siblings of content)
    lv_obj_move_foreground(this->icon_navbar);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);

    // Initialize UI selection
    this->setPreset(3);
}

// Replace overlay show/hide with button style toggling
static void apply_num_styles(int selected, lv_obj_t* b, int idx)
{
    // Button
    lv_obj_remove_style(b, &st_num_btn, 0);
    lv_obj_remove_style(b, &st_num_btn_sel, 0);

    // Label (always first child of button)
    lv_obj_t *lbl = lv_obj_get_child(b, 0);
    lv_obj_remove_style(lbl, &st_num_lbl_unsel, 0);
    lv_obj_remove_style(lbl, &st_num_lbl_sel, 0);

    if(idx == selected) {
        lv_obj_add_style(b, &st_num_btn_sel, 0);
        lv_obj_add_style(lbl, &st_num_lbl_sel, 0);
    } else {
        lv_obj_add_style(b, &st_num_btn, 0);
        lv_obj_add_style(lbl, &st_num_lbl_unsel, 0);
    }
}


void ScrPresets::hideSelectors()
{
    // Not needed anymore, but keep to avoid linker errors if called elsewhere
}

void ScrPresets::setPreset(int num)
{
    if (currentPreset == num) {
        this->showPresetDialog();
    }
    currentPreset = num;

    // NEW: single target computed from baseline + per-preset delta
    lv_coord_t end_y = g_car_top_base + preset_dy_px(num);
    animCarPreset(this, end_y);

    // Style number buttons (1..5)
    apply_num_styles(num, this->btnPreset1, 1);
    apply_num_styles(num, this->btnPreset2, 2);
    apply_num_styles(num, this->btnPreset3, 3);
    apply_num_styles(num, this->btnPreset4, 4);
    apply_num_styles(num, this->btnPreset5, 5);

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

void ScrPresets::runTouchInput(SimplePoint pos, bool down)
{
    // No manual hit-testing needed anymore: LVGL handles button input.
    Scr::runTouchInput(pos, down);
}

void ScrPresets::loop()
{
    Scr::loop();
}
