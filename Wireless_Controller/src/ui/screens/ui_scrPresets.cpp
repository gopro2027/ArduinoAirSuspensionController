#include "ui_scrPresets.h"

ScrPresets scrPresets(true, true, NAV_PRESETS);

LV_IMG_DECLARE(img_car);
LV_IMG_DECLARE(img_wheels);

// Dynamic car positioning based on screen size
// In landscape mode, offset car to the right to account for Save/Load buttons on left
static int getCarX() {
    int offset = isLandscape() ? scaledX(50) : 0;
    return getScreenWidth() / 2 - img_car.header.w / 2 + offset;
}
static int getWheelsX() {
    int offset = isLandscape() ? scaledX(50) : 0;
    return getScreenWidth() / 2 - img_wheels.header.w / 2 + offset;
}
static int getWheelsY() {
    // Position wheels in available space between pressure labels and bottom buttons
    // On larger displays, use a smaller multiplier to avoid overlap with buttons
    int baseY = 88 * SCALE_Y;
    int maxY = getScreenHeight() - getNavbarHeight() - scaledY(100); // Leave room for buttons
    return (baseY < maxY) ? baseY : maxY;
}
static int getCarY1() { return getWheelsY() - 21 * SCALE_Y; }
static int getCarY2() { return getCarY1() - 4 * SCALE_Y; }
static int getCarY3() { return getCarY2() - 4 * SCALE_Y; }
static int getCarY4() { return getCarY3() - 4 * SCALE_Y; }
static int getCarY5() { return getCarY4() - 4 * SCALE_Y; }

// Legacy constants for animation functions
#define car_x getCarX()
#define wheels_x getWheelsX()
#define wheels_y getWheelsY()
#define car_y_1 getCarY1()
#define car_y_2 getCarY2()
#define car_y_3 getCarY3()
#define car_y_4 getCarY4()
#define car_y_5 getCarY5()

SimpleRect fender1Offset = {40 * SCALE_X, 37 * SCALE_Y, 72 * SCALE_X - 40 * SCALE_X, 63 * SCALE_Y - 37 * SCALE_Y};
SimpleRect fender2Offset = {166 * SCALE_X, 35 * SCALE_Y, 199 * SCALE_X - 166 * SCALE_X, 60 * SCALE_Y - 35 * SCALE_Y};

// Modern button styling constants
// Dynamic preset button size based on display scaling
static int getPresetBtnSize() {
    return scaledX(38);  // Circular button, use X scaling
}
#define PRESET_BTN_WIDTH getPresetBtnSize()
#define PRESET_BTN_HEIGHT getPresetBtnSize()
#define PRESET_BTN_RADIUS LV_RADIUS_CIRCLE  // Perfect circle regardless of size
#define PRESET_BTN_COLOR GENERIC_GREY_DARK
#define PRESET_BTN_ACTIVE_COLOR THEME_COLOR_LIGHT
#define PRESET_BTN_GLOW_COLOR THEME_COLOR_MEDIUM
#define PRESET_BTN_BORDER_COLOR THEME_COLOR_DARK
static const uint32_t PRESET_BTN_TEXT_COLOR = 0x8888AA;     // Muted text when inactive
static const uint32_t PRESET_BTN_TEXT_ACTIVE = 0xFFFFFF;    // Bright text when active

// Store label references for updating text color
static lv_obj_t* presetLabels[5] = {NULL};

// Forward declaration for load function used in lambdas
void loadSelectedPreset();

// Event callbacks for preset buttons
static void presetBtnEventCb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int presetNum = (int)(intptr_t)lv_event_get_user_data(e);
        scrPresets.setPreset(presetNum);
    }
}


// Helper to create a modern circular preset button
static lv_obj_t* createPresetButton(lv_obj_t *parent, const char *text, int presetNum)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, PRESET_BTN_WIDTH, PRESET_BTN_HEIGHT);

    // Base styling - dark circular button
    lv_obj_set_style_bg_color(btn, lv_color_hex(PRESET_BTN_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, PRESET_BTN_RADIUS, LV_PART_MAIN);

    // Border with gradient effect
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(PRESET_BTN_BORDER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn, LV_OPA_70, LV_PART_MAIN);

    // Subtle shadow for depth (centered, no offset to prevent shifting)
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_x(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(btn, 0, LV_PART_MAIN);

    // Focus state styling
    lv_obj_set_style_bg_color(btn, lv_color_hex(PRESET_BTN_ACTIVE_COLOR), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(PRESET_BTN_ACTIVE_COLOR), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_FOCUSED);

    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    // Label with modern font
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(PRESET_BTN_TEXT_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(label);

    // Store label reference for color updates
    if (presetNum >= 1 && presetNum <= 5) {
        presetLabels[presetNum - 1] = label;
    }

    // Add click event
    lv_obj_add_event_cb(btn, presetBtnEventCb, LV_EVENT_CLICKED, (void*)(intptr_t)presetNum);

    return btn;
}



// square 1: 40,37 -> 71, 63
// square 2: 166,35 -> 198, 60

void car_anim_func(lv_obj_t *obj, int32_t y)
{

    lv_obj_set_y(obj, y);
    lv_obj_set_y(scrPresets.ww1, y + fender1Offset.y);
    lv_obj_set_y(scrPresets.ww2, y + fender2Offset.y);
    // lv_obj_move_foreground(scrPresets.wheels);
}

void animCarPreset(ScrPresets *scr, lv_coord_t end)
{
    lv_coord_t start = lv_obj_get_y(scr->car); // start is the current location
    int distance = abs(end - start);

    // Scale duration based on distance: ~150ms per 4 pixels, min 200ms, max 600ms
    int duration = (distance * 150) / 4;
    if (duration < 200) duration = 200;
    if (duration > 600) duration = 600;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)car_anim_func);
    lv_anim_set_var(&a, scr->car);
    lv_anim_set_time(&a, duration);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);  // Smooth easing
    lv_anim_start(&a);
}

void ScrPresets::init()
{
    Scr::init();

    // Reset static label references on reinit
    for (int i = 0; i < 5; i++) {
        presetLabels[i] = NULL;
    }

    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();

    // wheel well 1
    this->ww1 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww1);
    lv_obj_get_style_border_width(this->ww1, LV_PART_MAIN);
    lv_obj_set_size(this->ww1, fender1Offset.w, fender1Offset.h);
    scale_obj(this->ww1, fender1Offset.w, fender1Offset.h);
    lv_obj_set_style_bg_color(this->ww1, lv_color_hex(0x0), LV_PART_MAIN);
    lv_obj_remove_flag(this->ww1, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_set_style_bg_opa(this->ww1, 255, LV_PART_MAIN);
    lv_obj_set_x(this->ww1, car_x + fender1Offset.x);
    lv_obj_set_y(this->ww1, car_y_1 + fender1Offset.y);

    // wheel well 2
    this->ww2 = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->ww2);
    lv_obj_get_style_border_width(this->ww2, LV_PART_MAIN);
    lv_obj_set_size(this->ww2, fender2Offset.w, fender2Offset.h);
    scale_obj(this->ww2, fender2Offset.w, fender2Offset.h);
    lv_obj_set_style_bg_color(this->ww2, lv_color_hex(0x0), LV_PART_MAIN);
    lv_obj_remove_flag(this->ww2, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_set_style_bg_opa(this->ww2, 255, LV_PART_MAIN);
    lv_obj_set_x(this->ww2, car_x + fender2Offset.x);
    lv_obj_set_y(this->ww2, car_y_1 + fender2Offset.y);

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

    // --- Preset buttons section ---
    // Position above navbar - dynamic for rotation support
    const int navbarHeight = NAVBAR_HEIGHT;
    const int presetAreaHeight = scaledY(50);
    const int presetAreaY = screenHeight - navbarHeight - presetAreaHeight - scaledY(5);

    // Different layout for landscape vs portrait
    const bool landscape = isLandscape();

    // --- Preset buttons container  ---
    this->presetButtonsContainer = lv_obj_create(this->scr);
    lv_obj_remove_style_all(this->presetButtonsContainer);
    lv_obj_set_flex_flow(this->presetButtonsContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(this->presetButtonsContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(this->presetButtonsContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    // circular preset buttons
    this->btnPreset1 = createPresetButton(this->presetButtonsContainer, "1", 1);
    this->btnPreset2 = createPresetButton(this->presetButtonsContainer, "2", 2);
    this->btnPreset3 = createPresetButton(this->presetButtonsContainer, "3", 3);
    this->btnPreset4 = createPresetButton(this->presetButtonsContainer, "4", 4);
    this->btnPreset5 = createPresetButton(this->presetButtonsContainer, "5", 5);

    // --- Save/Load buttons container ---
    lv_obj_t *actionContainer = lv_obj_create(this->scr);
    lv_obj_remove_style_all(actionContainer);
    // Save button
    this->btnSave = lv_btn_create(actionContainer);
    // Load button
    this->btnLoad = lv_btn_create(actionContainer);

    if (landscape) {
        // LANDSCAPE LAYOUT: Preset buttons at bottom, Save/Load on left side vertically

        // Leave space on left for save/load buttons
        const int presetsWidth = screenWidth - scaledX(100);  // 100px for save/load on left
        lv_obj_set_size(this->presetButtonsContainer, presetsWidth, presetAreaHeight);
        lv_obj_set_pos(this->presetButtonsContainer, scaledX(100), presetAreaY);  // Offset to the right

        const int actionWidth = scaledX(85);
        const int actionHeight = scaledY(80);
        const int actionY = screenHeight - navbarHeight - actionHeight - scaledY(8);
        lv_obj_set_size(actionContainer, actionWidth, actionHeight);
        lv_obj_set_pos(actionContainer, scaledX(8), actionY);
        lv_obj_set_flex_flow(actionContainer, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(this->btnSave, scaledX(75), scaledY(32));
        lv_obj_set_size(this->btnLoad, scaledX(75), scaledY(32));

    } else {
        // PORTRAIT LAYOUT: Original stacked layout
        lv_obj_set_size(this->presetButtonsContainer, screenWidth, presetAreaHeight);
        lv_obj_set_pos(this->presetButtonsContainer, 0, presetAreaY);

        const int actionAreaHeight = scaledY(40);
        const int actionAreaY = presetAreaY - actionAreaHeight - scaledY(5);
        lv_obj_set_size(actionContainer, screenWidth, actionAreaHeight);
        lv_obj_set_pos(actionContainer, 0, actionAreaY);

        lv_obj_set_flex_flow(actionContainer, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(this->btnSave, scaledX(90), scaledY(32));
        lv_obj_set_size(this->btnLoad, scaledX(90), scaledY(32));

    }

    lv_obj_set_flex_align(actionContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(actionContainer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(this->btnSave, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN);
    lv_obj_set_style_radius(this->btnSave, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(this->btnSave, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(this->btnSave, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_MAIN);
    lv_obj_t *saveLabel = lv_label_create(this->btnSave);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_set_style_text_color(saveLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_center(saveLabel);
    lv_obj_add_event_cb(this->btnSave, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            static char buf[40];
            snprintf(buf, sizeof(buf), "Save current height to preset %i?", currentPreset);
            scrPresets.showMsgBox(buf, NULL, "Confirm", "Cancel", []() {
                Serial.println("save preset");
                SaveCurrentPressuresToProfilePacket pkt(currentPreset - 1);
                sendRestPacket(&pkt);
                showDialog("Saved Preset!", lv_color_hex(THEME_COLOR_LIGHT));
                requestPreset();
            }, []() {}, false);
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_set_style_bg_color(this->btnLoad, lv_color_hex(PRESET_BTN_ACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_radius(this->btnLoad, 16, LV_PART_MAIN);
    lv_obj_t *loadLabel = lv_label_create(this->btnLoad);
    lv_label_set_text(loadLabel, "Load");
    lv_obj_set_style_text_color(loadLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(loadLabel);
    lv_obj_add_event_cb(this->btnLoad, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            if (currentPreset == 1) {
                scrPresets.showMsgBox("Air out?", "Preset 1 is typically air out. Please verify your car is not moving.", "Confirm", "Cancel", []() {
                    loadSelectedPreset();
                }, []() {}, false);
            } else {
                loadSelectedPreset();
            }
        }
    }, LV_EVENT_CLICKED, NULL);

    // Bring overlays to foreground
    if (this->navbar_container) lv_obj_move_foreground(this->navbar_container);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);

    // Reset currentPreset before setting to avoid showing dialog on reinit
    // (setPreset shows dialog when clicking same preset twice)
    int savedPreset = currentPreset;
    currentPreset = -1;
    this->setPreset(savedPreset > 0 ? savedPreset : 3);
}

void ScrPresets::updateButtonStyles()
{
    // Update button styles based on current preset
    lv_obj_t* btns[] = {btnPreset1, btnPreset2, btnPreset3, btnPreset4, btnPreset5};
    for (int i = 0; i < 5; i++) {
        if (i + 1 == currentPreset) {
            // Active button - purple with glow (same shadow width to prevent shifting)
            lv_obj_set_style_bg_color(btns[i], lv_color_hex(PRESET_BTN_ACTIVE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_border_color(btns[i], lv_color_hex(PRESET_BTN_ACTIVE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_border_opa(btns[i], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_shadow_color(btns[i], lv_color_hex(PRESET_BTN_GLOW_COLOR), LV_PART_MAIN);
            lv_obj_set_style_shadow_opa(btns[i], LV_OPA_60, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btns[i], 8, LV_PART_MAIN);
            // Update label color
            if (presetLabels[i]) {
                lv_obj_set_style_text_color(presetLabels[i], lv_color_hex(PRESET_BTN_TEXT_ACTIVE), LV_PART_MAIN);
            }
        } else {
            // Inactive button - dark with subtle styling
            lv_obj_set_style_bg_color(btns[i], lv_color_hex(PRESET_BTN_COLOR), LV_PART_MAIN);
            lv_obj_set_style_border_color(btns[i], lv_color_hex(PRESET_BTN_BORDER_COLOR), LV_PART_MAIN);
            lv_obj_set_style_border_opa(btns[i], LV_OPA_70, LV_PART_MAIN);
            lv_obj_set_style_shadow_color(btns[i], lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_set_style_shadow_opa(btns[i], LV_OPA_30, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(btns[i], 8, LV_PART_MAIN);
            // Update label color
            if (presetLabels[i]) {
                lv_obj_set_style_text_color(presetLabels[i], lv_color_hex(PRESET_BTN_TEXT_COLOR), LV_PART_MAIN);
            }
        }
    }
}
void ScrPresets::setPreset(int num)
{
    if (currentPreset == num)
    {
        this->showPresetDialog();
    }
    currentPreset = num;
    updateButtonStyles();
    switch (num)
    {
    case 1:
        animCarPreset(this, car_y_1);
        break;
    case 2:
        animCarPreset(this, car_y_2);
        break;
    case 3:
        animCarPreset(this, car_y_3);
        break;
    case 4:
        animCarPreset(this, car_y_4);
        break;
    case 5:
        animCarPreset(this, car_y_5);
        break;
    }
    requestPreset();
}

void ScrPresets::showPresetDialog()
{
    static char text[100];
    static char title[10];
    // This is honestly quite shit
    snprintf(text, sizeof(text), "  fd: %i                        fp: %i\n  rd: %i                        rp: %i", profilePressures[currentPreset - 1][WHEEL_FRONT_DRIVER], profilePressures[currentPreset - 1][WHEEL_FRONT_PASSENGER], profilePressures[currentPreset - 1][WHEEL_REAR_DRIVER], profilePressures[currentPreset - 1][WHEEL_REAR_PASSENGER]);
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


void ScrPresets::loop()
{
    Scr::loop();
}
