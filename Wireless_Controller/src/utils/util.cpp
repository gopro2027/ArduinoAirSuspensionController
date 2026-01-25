#include "util.h"

// Dynamic screen dimension helpers for rotation support
int getScreenWidth() {
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        return lv_display_get_horizontal_resolution(disp);
    }
    return LCD_WIDTH;
}

int getScreenHeight() {
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        return lv_display_get_vertical_resolution(disp);
    }
    return LCD_HEIGHT;
}

bool isLandscape() {
    return getScreenWidth() > getScreenHeight();
}

// Get base resolution for scaling calculations
// This detects the smallest dimension in portrait mode
int getBaseWidth() {
    int w = LCD_WIDTH;
    int h = LCD_HEIGHT;
    // Normalize to portrait (width < height)
    if (w > h) {
        int temp = w;
        w = h;
        h = temp;
    }
    return w;  // Will be 240, 320, or 480
}

int getBaseHeight() {
    int w = LCD_WIDTH;
    int h = LCD_HEIGHT;
    // Normalize to portrait (width < height)
    if (w > h) {
        int temp = w;
        w = h;
        h = temp;
    }
    return h;  // Will be 320, 480, or 640
}

// Dynamic scaling functions (recalculate based on current screen size)
// Scale relative to 240×320 reference design
float getScaleX() {
    int baseWidth = getBaseWidth();
    int currentWidth = isLandscape() ? getScreenHeight() : getScreenWidth();
    return currentWidth / 240.0f;
}

float getScaleY() {
    int baseHeight = getBaseHeight();
    int currentHeight = isLandscape() ? getScreenWidth() : getScreenHeight();
    return currentHeight / 320.0f;
}

// Dynamic arrow button dimensions
float getArrowButtonWidth() {
    return 54 * getScaleX();
}

float getArrowButtonHeight() {
    return 44 * getScaleY();
}

void scale_obj(lv_obj_t *obj, int w, int h) {
    // lv_image_set_scale_x(obj, SCALE_X * 256);
    // lv_image_set_scale_y(obj, SCALE_Y * 256);
    // lv_obj_set_size(obj, w * SCALE_X, h * SCALE_Y);
}

void scale_img(lv_obj_t *obj, lv_image_dsc_t img) {
    // scale_obj(obj, img.header.w, img.header.h);
}

int sr_contains(SimpleRect r, SimplePoint p)
{
    return p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}

int cr_contains(CenterRect cr, SimplePoint p)
{
    SimpleRect sr = {cr.cx - (cr.w / 2), cr.cy - (cr.h / 2), cr.w, cr.h};
    return sr_contains(sr, p);
}

// Dynamic touch area functions (recalculate on each call for rotation support)
// In landscape mode: horizontal pills (left=up, right=down)
// In portrait mode: vertical pills (top=up, bottom=down)

// Pill dimensions for touch areas
// Dynamic pill dimensions that match ui_scrHome.cpp calculations
static int getPillWidth() {
    if (isLandscape()) {
        return 100;  // Landscape horizontal pills
    } else {
        return 54;   // Portrait vertical pills
    }
}

static int getPillHeight() {
    const int screenHeight = getScreenHeight();
    const int pressureAreaHeight = scaledY(55);
    const int navbarHeight = getNavbarHeight();
    const int contentHeight = screenHeight - pressureAreaHeight - navbarHeight;

    if (isLandscape()) {
        return (contentHeight - 24) / 2;  // Match ui_scrHome.cpp calculation
    } else {
        return 90;  // Portrait vertical pills
    }
}

// Helper to get pill touch area parameters - must match ui_scrHome.cpp pill positions
static void getPillParams(int col, int row, float &cx, float &cy, float &halfW, float &halfH) {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int pressureAreaHeight = scaledY(55);
    const int navbarHeight = getNavbarHeight();
    const int contentHeight = screenHeight - pressureAreaHeight - navbarHeight;

    const int pillW = getPillWidth();
    const int pillH = getPillHeight();

    if (isLandscape()) {
        // Landscape: 3 horizontal pills per row with scaled gap between them
        const int gap = scaledX(12);
        const int totalPillWidth = 3 * pillW + 2 * gap;
        const int sidePadding = (screenWidth - totalPillWidth) / 2;

        cx = sidePadding + pillW / 2 + col * (pillW + gap);
        cy = pressureAreaHeight + (contentHeight / 2 - pillH) / 2 + pillH / 2 + row * (pillH + scaledY(24));
        halfW = pillW / 2.0f;
        halfH = pillH / 2.0f;
    } else {
        // Portrait: 3 vertical pills per row
        const int spacing = (screenWidth - 3 * pillW) / 4;
        const int rowSpacing = (contentHeight - 2 * pillH) / 3;

        cx = spacing + pillW / 2 + col * (pillW + spacing);
        cy = pressureAreaHeight + rowSpacing + pillH / 2 + row * (pillH + rowSpacing);
        halfW = pillW / 2.0f;
        halfH = pillH / 2.0f;
    }
}

// first column (left) - col=0
CenterRect get_ctr_row0col0up() {
    float cx, cy, halfW, halfH;
    getPillParams(0, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        // Left half of horizontal pill
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        // Top half of vertical pill
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row0col0down() {
    float cx, cy, halfW, halfH;
    getPillParams(0, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        // Right half of horizontal pill
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        // Bottom half of vertical pill
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col0up() {
    float cx, cy, halfW, halfH;
    getPillParams(0, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col0down() {
    float cx, cy, halfW, halfH;
    getPillParams(0, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}

// second column (center) - col=1
CenterRect get_ctr_row0col1up() {
    float cx, cy, halfW, halfH;
    getPillParams(1, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row0col1down() {
    float cx, cy, halfW, halfH;
    getPillParams(1, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col1up() {
    float cx, cy, halfW, halfH;
    getPillParams(1, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col1down() {
    float cx, cy, halfW, halfH;
    getPillParams(1, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}

// third column (right) - col=2
CenterRect get_ctr_row0col2up() {
    float cx, cy, halfW, halfH;
    getPillParams(2, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row0col2down() {
    float cx, cy, halfW, halfH;
    getPillParams(2, 0, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col2up() {
    float cx, cy, halfW, halfH;
    getPillParams(2, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx - halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy - halfH / 2, halfW * 2, halfH};
    }
}
CenterRect get_ctr_row1col2down() {
    float cx, cy, halfW, halfH;
    getPillParams(2, 1, cx, cy, halfW, halfH);
    if (isLandscape()) {
        return {cx + halfW / 2, cy, halfW, halfH * 2};
    } else {
        return {cx, cy + halfH / 2, halfW * 2, halfH};
    }
}

// bottom nav - dynamic for all screen sizes and rotation
SimpleRect get_navbarbtn_home() {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int navbarHeight = getNavbarHeight();
    const int btnWidth = screenWidth / 3;
    const int navbarY = screenHeight - navbarHeight;
    return {0, navbarY, btnWidth, navbarHeight};
}
SimpleRect get_navbarbtn_presets() {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int navbarHeight = getNavbarHeight();
    const int btnWidth = screenWidth / 3;
    const int navbarY = screenHeight - navbarHeight;
    return {btnWidth, navbarY, btnWidth, navbarHeight};
}
SimpleRect get_navbarbtn_settings() {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int navbarHeight = getNavbarHeight();
    const int btnWidth = screenWidth / 3;
    const int navbarY = screenHeight - navbarHeight;
    return {btnWidth * 2, navbarY, btnWidth, navbarHeight};
}

// presets buttons (dynamic based on preset number)
CenterRect get_ctr_preset(int num) {
    return {(48.0 / 2 + 48 * (num - 1)) * getScaleX(), 182 * getScaleY(), 48 * getScaleX(), 48 * getScaleY()};
}

// preset save and load
SimpleRect get_preset_save() {
    return {18 * getScaleX(), 235 * getScaleY(), (91 - 18) * getScaleX(), (251 - 235) * getScaleY()};
}
SimpleRect get_preset_load() {
    return {110 * getScaleX(), 225 * getScaleY(), (221 - 110) * getScaleX(), (256 - 225) * getScaleY()};
}

int currentPressures[5];
uint32_t statusBittset = 0;
uint8_t AIPercentage = 0;
uint8_t AIReadyBittset = 0;
int profilePressures[5][4];
bool profileUpdated = false;
int currentPreset = -1;
void requestPreset()
{
    PresetPacket pkt(currentPreset - 1, 0, 0, 0, 0);
    sendRestPacket(&pkt);
}

ConfigValuesPacket util_configValues(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

void sendConfigValuesPacket(bool saveToManifold)
{
    *util_configValues._setValues() = saveToManifold;
    sendRestPacket(&util_configValues);
}

UpdateStatusRequestPacket util_statusRequestPacket;

void sendUpdateStatusRequestPacket()
{
    util_statusRequestPacket.setStatus("UNKNOWN");
    util_statusRequestPacket._setStatus = true;
    sendRestPacket(&util_statusRequestPacket);
}

Scr *screens[3];
Scr *currentScr = NULL;

std::function<void()> functionToRunOnNextFrame = NULL;
bool doRunFunctionNextFrame = false;
void runNextFrame(std::function<void()> function)
{
    functionToRunOnNextFrame = function;
    doRunFunctionNextFrame = true;
}
void handleFunctionRunOnNextFrame()
{
    if (doRunFunctionNextFrame)
    {
        doRunFunctionNextFrame = false;
        functionToRunOnNextFrame();
    }
}

#pragma region bluetooth packets

#define BTOASPACKETCOUNT 10
struct PacketEntry
{
    bool taken;
    BTOasPacket packet;
};
PacketEntry packets[BTOASPACKETCOUNT];
static SemaphoreHandle_t restMutex;
void setupRestSemaphore()
{
    restMutex = xSemaphoreCreateMutex();
    memset(packets, 0, sizeof(packets));
}
void waitRestSemaphore()
{
    while (xSemaphoreTake(restMutex, 1) != pdTRUE)
    {
        delay(1);
    }
}
void giveRestSemaphore()
{
    xSemaphoreGive(restMutex);
}

void clearPackets()
{
    waitRestSemaphore();
    for (int i = 0; i < BTOASPACKETCOUNT; i++)
    {
        packets[i].taken = false;
    }
    giveRestSemaphore();
}

bool getBTRestPacketToSend(BTOasPacket *copyTo)
{
    bool ret = false;
    waitRestSemaphore();
    for (int i = 0; i < BTOASPACKETCOUNT; i++)
    {
        if (packets[i].taken)
        {
            packets[i].taken = false;
            memcpy(copyTo, &packets[i].packet, BTOAS_PACKET_SIZE);
            ret = true;
            break;
        }
    }
    giveRestSemaphore();
    return ret;
}
void sendRestPacket(BTOasPacket *packet)
{
    waitRestSemaphore();
    for (int i = 0; i < BTOASPACKETCOUNT; i++)
    {
        if (!packets[i].taken)
        {
            packets[i].taken = true;
            memcpy(&packets[i].packet, packet, BTOAS_PACKET_SIZE);
            break;
        }
    }
    giveRestSemaphore();
}

#pragma endregion

unsigned long dialogEndTime = 0;
lv_color_t dialogColor;
char dialogText[50];
bool updateDialog = false;
void showDialog(const char *text, lv_color_t color, unsigned long durationMS)
{
    strncpy(dialogText, text, sizeof(dialogText));
    memcpy(&dialogColor, &color, sizeof(lv_color_t));
    dialogEndTime = millis() + durationMS;
    updateDialog = true;
}
void dialogLoop()
{
    if (updateDialog)
    {
        // Only show on current screen - other screens will sync when switched to
        if (currentScr != NULL && currentScr->alert != NULL)
        {
            currentScr->alert->show(dialogColor, dialogText, dialogEndTime);
        }
        updateDialog = false;
    }
}

#pragma region valveControl
unsigned int valveControlValue = 0;
unsigned int getValveControlValue()
{
    return valveControlValue;
}
void setValveBit(int bit)
{
    valveControlValue = valveControlValue | (1 << bit);
}
void unsetValveBit(int bit)
{
    valveControlValue = valveControlValue & ~(1 << bit);
}
void closeValves()
{
    valveControlValue = 0;
}
#pragma endregion

void setupPressureLabel(lv_obj_t *parent, lv_obj_t **label, int x, int y, lv_align_t align, const char *defaultText)
{
    *label = lv_label_create(parent);

    // Modern styling with better font and color
    lv_obj_set_style_text_color(*label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_14, 0);

    lv_obj_set_style_text_opa(*label, LV_OPA_COVER, 0);

    lv_obj_set_width(*label, LV_SIZE_CONTENT);
    lv_obj_set_height(*label, LV_SIZE_CONTENT);
    lv_obj_set_x(*label, x);
    lv_obj_set_y(*label, y);
    lv_obj_set_align(*label, align);
    lv_label_set_text(*label, defaultText);

    lv_obj_move_foreground(*label);
}

SaveData _SaveData;
void beginSaveData()
{
    _SaveData.unitsMode.load("units", UNITS_MODE::PSI);
    _SaveData.blePasskey.load("blePasskey", 202777);
    _SaveData.screenDimTimeM.load("screenDimTimeM", 3);
    _SaveData.updateMode.load("updateMode", false);
    _SaveData.wifiSSID.loadString("wifiSSID", "");
    _SaveData.wifiPassword.loadString("wifiPassword", "");
    _SaveData.updateResult.load("updateResult", 0);
    _SaveData.brightness.load("brightness", 80);
    _SaveData.screenRotation.load("screenRotation", 0);
    // Theme colors - using default purple/lavender theme values
    _SaveData.themeColorLight.load("themeColorLight", THEME_COLOR_OCEAN_BLUE_LIGHT);
    _SaveData.themeColorDark.load("themeColorDark", THEME_COLOR_OCEAN_BLUE_DARK);
    _SaveData.themeColorMedium.load("themeColorMedium", THEME_COLOR_OCEAN_BLUE_MEDIUM);
}

createSaveFuncInt(unitsMode, int);
createSaveFuncInt(blePasskey, uint32_t);
createSaveFuncInt(screenDimTimeM, uint32_t);
createSaveFuncInt(updateMode, bool);
createSaveFuncString(wifiSSID);
createSaveFuncString(wifiPassword);
createSaveFuncInt(updateResult, byte);
createSaveFuncInt(brightness, byte);
createSaveFuncInt(screenRotation, byte);
createSaveFuncInt(themeColorLight, uint32_t);
createSaveFuncInt(themeColorDark, uint32_t);
createSaveFuncInt(themeColorMedium, uint32_t);

float getBrightnessFloat()
{
    int brightnessInt = getbrightness();
    if (brightnessInt < 1)
        brightnessInt = 1;
    if (brightnessInt > 100)
        brightnessInt = 100;
    return brightnessInt / 100.0f;
}

// Forward declaration
void ui_reinit(void);

#if SUPPORTS_ROTATION == 1
void applyScreenRotation(byte rotation)
{
    lv_display_t *disp = lv_display_get_default();
    if (!disp) return;

    // Apply hardware rotation via MADCTL register
    LCD_SetRotation(rotation);

    // Use actual LCD dimensions (works for all display sizes)
    // LCD_WIDTH and LCD_HEIGHT are defined in board JSON as compile-time constants
    if (rotation == 1) {
        // Landscape: swap width and height
        lv_display_set_resolution(disp, LCD_HEIGHT, LCD_WIDTH);
    } else {
        // Portrait: use native dimensions
        lv_display_set_resolution(disp, LCD_WIDTH, LCD_HEIGHT);
    }
    // Do NOT use lv_display_set_rotation - we're using hardware rotation
}
#endif

void reinitializeScreens()
{
    ui_reinit();
}

static lv_obj_t *kb = NULL;
void closeKeyboard()
{
    if (kb != NULL)
    {
        lv_keyboard_set_textarea(kb, NULL);
        // lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(kb);
        kb = NULL;
    }
}
void defocus(Option *option)
{
    lv_obj_send_event(option->root, LV_EVENT_FOCUSED, NULL);
    lv_obj_send_event(option->rightHandObj, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_remove_state(option->rightHandObj, LV_STATE_FOCUSED);
    lv_group_focus_obj(option->root);
    // lv_group_focus_prev(lv_obj_get_group(option->root));
}
static void kb_event_cb(lv_event_t *e)
{

    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CANCEL)
    {
        // lv_obj_set_height(cont, LV_VER_RES);
        // lv_obj_del(kb);
        // kb = NULL;
        if (option != NULL)
        {
            defocus(option);
        }

        closeKeyboard();
    }
    if (event_code == LV_EVENT_READY)
    {
        if (option != NULL)
        {
            defocus(option);
            Serial.println(lv_textarea_get_text(option->rightHandObj));
            if (option->type == OptionType::KEYBOARD_INPUT_NUMBER)
            {
                option->event_cb((void *)atoi(lv_textarea_get_text(option->rightHandObj)));
            }
            else
            {
                option->event_cb((void *)lv_textarea_get_text(option->rightHandObj));
            }
        }
        closeKeyboard();
    }
}
void initKB(Option *option)
{
    closeKeyboard();
    kb = lv_keyboard_create(lv_screen_active());
    // lv_obj_set_height(cont, LV_VER_RES / 2);
    if (option->type == OptionType::KEYBOARD_INPUT_NUMBER)
    {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    }
    else
    {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    }
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, option);
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN);    // lines in between buttons
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_ITEMS);  // buttons
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_CHECKED);  // buttons (keyboard and checkmark)
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_FOCUSED); // When pressing down on the buttons
}

void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        initKB(option);
        lv_keyboard_set_textarea(kb, ta);
        // lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        //  lv_obj_move_foreground(kb);
    }
    if (code == LV_EVENT_DEFOCUSED)
    {
        closeKeyboard();
    }
}

static char strbuf[20];
void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);

    if (event_code == option->slider_trigger_event)
    {
        // if (!lv_obj_has_state(target, LV_STATE_PRESSED))
        //     return;
        int32_t val = lv_slider_get_value(target);
        lv_label_set_text(option->ui_slider_value_text, itoa(val, strbuf, 10));
        option->event_cb((void *)val);
    }

    if (event_code == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
        lv_event_set_ext_draw_size(e, 50);
    }
    else if (event_code == LV_EVENT_DRAW_MAIN_END)
    {
        if (!lv_obj_has_state(target, LV_STATE_PRESSED))
            return;

        // get the exact bounding box of the slider
        lv_area_t slider_area;
        lv_obj_get_coords(target, &slider_area);

        // shrink slider area so that is is only the width of the selected part
        lv_area_set_width(&slider_area, lv_area_get_width(&slider_area) * (lv_slider_get_value(target) - option->slider_min) / (option->slider_max - option->slider_min));

        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d", (int)lv_slider_get_value(target));

        // calculate size of text so that we have the exact size of an area to draw the label in
        lv_point_t label_size;
        lv_text_get_size(&label_size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_area_t label_area;
        label_area.x1 = 0;
        label_area.x2 = label_size.x - 1;
        label_area.y1 = 0;
        label_area.y2 = label_size.y - 1;

        // position the text area that we are rendering in, within the bounding box of the slider
        lv_area_align(&slider_area, &label_area, LV_ALIGN_RIGHT_MID, label_size.x / 2, 0); // LV_ALIGN_OUT_TOP_MID would be just above the bar

        lv_draw_label_dsc_t label_draw_dsc;
        lv_draw_label_dsc_init(&label_draw_dsc);
        label_draw_dsc.color = lv_color_hex3(0x888);
        label_draw_dsc.text = buf;
        label_draw_dsc.text_local = true;
        lv_layer_t *layer = lv_event_get_layer(e);
        lv_draw_label(layer, &label_draw_dsc, &label_area);
    }

    // lv_obj_t *slider = lv_event_get_target_obj(e);
    // char buf[8];
    // lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
    // lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

bool isKeyboardHidden()
{
    return kb == NULL; // lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

void onBLEConnectionCompleted()
{
    sendConfigValuesPacket(false);   // sends a request of the manifold to send out the manifolds save data
    requestPreset();                 // sends a request of the manifold to send out the current presets values
    sendUpdateStatusRequestPacket(); // sends a request of the manifold to send out the current update status
}

// Apply a theme preset
void applyThemePreset(ThemePreset preset) {
    switch (preset) {
        case PRESET_PURPLE:
            setthemeColorLight(THEME_COLOR_PLUMP_PURPLE_LIGHT);   // Light purple
            setthemeColorMedium(THEME_COLOR_PLUMP_PURPLE_MEDIUM);  // Medium purple
            setthemeColorDark(THEME_COLOR_PLUMP_PURPLE_DARK);    // Dark purple
            break;
        case PRESET_BLUE:
            setthemeColorLight(THEME_COLOR_OCEAN_BLUE_LIGHT);   // Light blue
            setthemeColorMedium(THEME_COLOR_OCEAN_BLUE_MEDIUM);  // Medium blue
            setthemeColorDark(THEME_COLOR_OCEAN_BLUE_DARK);    // Dark blue
            break;
        case PRESET_GREEN:
            setthemeColorLight(THEME_COLOR_FOREST_GREEN_LIGHT);   // Light green
            setthemeColorMedium(THEME_COLOR_FOREST_GREEN_MEDIUM);  // Medium green
            setthemeColorDark(THEME_COLOR_FOREST_GREEN_DARK);    // Dark green
            break;
        case PRESET_DESERT_SAND:
            setthemeColorLight(THEME_COLOR_DESERT_SAND_LIGHT);    // Desert Sand (light)
            setthemeColorMedium(THEME_COLOR_DESERT_SAND_MEDIUM);  // Desert Sand (medium)
            setthemeColorDark(THEME_COLOR_DESERT_SAND_DARK);      // Desert Sand (dark)
            break;
        case PRESET_CUSTOM:
        default:
            // Do nothing for custom
            break;
    }
}

// Get current theme preset (-1 if custom)
int getCurrentThemePreset() {
    uint32_t light = getthemeColorLight();
    uint32_t medium = getthemeColorMedium();
    uint32_t dark = getthemeColorDark();
    
    // Check if current colors match any preset
    if (light == THEME_COLOR_PLUMP_PURPLE_LIGHT && medium == THEME_COLOR_PLUMP_PURPLE_MEDIUM && dark == THEME_COLOR_PLUMP_PURPLE_DARK) {
        return PRESET_PURPLE;
    }
    if (light == THEME_COLOR_OCEAN_BLUE_LIGHT && medium == THEME_COLOR_OCEAN_BLUE_MEDIUM && dark == THEME_COLOR_OCEAN_BLUE_DARK) {
        return PRESET_BLUE;
    }
    if (light == THEME_COLOR_FOREST_GREEN_LIGHT && medium == THEME_COLOR_FOREST_GREEN_MEDIUM && dark == THEME_COLOR_FOREST_GREEN_DARK) {
        return PRESET_GREEN;
    }
    if (light == THEME_COLOR_DESERT_SAND_LIGHT && medium == THEME_COLOR_DESERT_SAND_MEDIUM && dark == THEME_COLOR_DESERT_SAND_DARK) {
        return PRESET_DESERT_SAND;
    }
    return PRESET_CUSTOM;
}