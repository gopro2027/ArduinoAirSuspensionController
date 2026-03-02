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
    int currentWidth = isLandscape() ? getScreenHeight() : getScreenWidth();
    return currentWidth / 240.0f;
}

float getScaleY() {
    int currentHeight = isLandscape() ? getScreenWidth() : getScreenHeight();
    return currentHeight / 320.0f;
}

void scale_obj(lv_obj_t *obj, int w, int h) {
    // lv_image_set_scale_x(obj, SCALE_X * 256);
    // lv_image_set_scale_y(obj, SCALE_Y * 256);
    // lv_obj_set_size(obj, w * SCALE_X, h * SCALE_Y);
}

void scale_img(lv_obj_t *obj, lv_image_dsc_t img) {
    // scale_obj(obj, img.header.w, img.header.h);
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

ConfigValuesPacket util_configValues(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

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
    _SaveData.swipeNavigation.load("swipeNav", false);
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
createSaveFuncInt(swipeNavigation, bool);

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