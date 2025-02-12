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
uint16_t statusBittset = 0;
int profilePressures[5][4];
bool profileUpdated = false;

ConfigValuesPacket util_configValues(0, 0, 0, 0, 0, 0);

void sendConfigValuesPacket(bool saveToManifold)
{
    *util_configValues._setValues() = saveToManifold;
    sendRestPacket(&util_configValues);
}

Scr *screens[3];

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
void showDialog(char *text, lv_color_t color, unsigned long durationMS)
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
}

int getUnits()
{
    return _SaveData.unitsMode.get().i;
}
void setUnits(int value)
{
    if (getUnits() != value)
    {
        _SaveData.unitsMode.set(value);
    }
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
        closeKeyboard();
    }
    if (event_code == LV_EVENT_READY)
    {
        if (option != NULL)
        {
            Serial.println(lv_textarea_get_text(option->rightHandObj));
            option->event_cb((void *)atoi(lv_textarea_get_text(option->rightHandObj)));
        }
        closeKeyboard();
    }
}
void initKB(Option *option)
{
    closeKeyboard();
    kb = lv_keyboard_create(lv_scr_act());
    // lv_obj_set_height(cont, LV_VER_RES / 2);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, option);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x5A4673), LV_PART_MAIN | LV_STATE_DEFAULT); // lines in between buttons
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xBB86FC), LV_PART_ITEMS);                   // buttons
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

bool isKeyboardHidden()
{
    return kb == NULL; // lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN);
}