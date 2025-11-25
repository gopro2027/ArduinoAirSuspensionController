#include "util.h"
int sr_contains(SimpleRect r, SimplePoint p)
{
    return p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}

int cr_contains(CenterRect cr, SimplePoint p)
{
    SimpleRect sr = {cr.cx - (cr.w / 2), cr.cy - (cr.h / 2), cr.w, cr.h};
    return sr_contains(sr, p);
}

// first column (left)
CenterRect ctr_row0col0up = {48, 89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col0down = {48, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col0up = {48, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col0down = {48, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// second column (center)
CenterRect ctr_row0col1up = {118, 78, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col1down = {118, 126, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col1up = {118, 204, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col1down = {118, 252, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// third column (right)
CenterRect ctr_row0col2up = {189, 89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col2down = {189, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col2up = {189, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col2down = {189, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// bottom nav
SimpleRect navbarbtn_home = {0, 291, 80, 29};
SimpleRect navbarbtn_presets = {80, 291, 80, 29};
SimpleRect navbarbtn_settings = {160, 291, 80, 29};

// presets buttons
CenterRect ctr_preset_1 = {48 / 2 + 48 * 0, 182, 48, 48};
CenterRect ctr_preset_2 = {48 / 2 + 48 * 1, 182, 48, 48};
CenterRect ctr_preset_3 = {48 / 2 + 48 * 2, 182, 48, 48};
CenterRect ctr_preset_4 = {48 / 2 + 48 * 3, 182, 48, 48};
CenterRect ctr_preset_5 = {48 / 2 + 48 * 4, 182, 48, 48};

// preset save and load
SimpleRect preset_save = {18, 235, 91 - 18, 251 - 235};
SimpleRect preset_load = {110, 225, 221 - 110, 256 - 225};

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

ConfigValuesPacket util_configValues(0, 0, 0, 0, 0, 0, 0);

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
        screens[0]->alert->show(dialogColor, dialogText, dialogEndTime);
        screens[1]->alert->show(dialogColor, dialogText, dialogEndTime);
        screens[2]->alert->show(dialogColor, dialogText, dialogEndTime);
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
void closeValves()
{
    valveControlValue = 0;
}
#pragma endregion

void setupPressureLabel(lv_obj_t *parent, lv_obj_t **label, int x, int y, lv_align_t align, const char *defaultText)
{
    *label = lv_label_create(parent);
    lv_obj_set_style_text_color(*label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(*label, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(*label, LV_SIZE_CONTENT); /// 1
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
}

createSaveFuncInt(unitsMode, int);
createSaveFuncInt(blePasskey, uint32_t);
createSaveFuncInt(screenDimTimeM, uint32_t);
createSaveFuncInt(updateMode, bool);
createSaveFuncString(wifiSSID);
createSaveFuncString(wifiPassword);
createSaveFuncInt(updateResult, byte);
createSaveFuncInt(brightness, byte);

float getBrightnessFloat()
{
    int brightnessInt = getbrightness();
    if (brightnessInt < 0)
        brightnessInt = 0;
    if (brightnessInt > 100)
        brightnessInt = 100;
    return brightnessInt / 100.0f;
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
    kb = lv_keyboard_create(lv_scr_act());
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
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);    // lines in between buttons
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_ITEMS | LV_STATE_DEFAULT);  // buttons
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_ITEMS | LV_STATE_CHECKED);  // buttons (keyboard and checkmark)
    lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_COLOR_MEDIUM), LV_PART_ITEMS | LV_STATE_FOCUSED); // When pressing down on the buttons
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