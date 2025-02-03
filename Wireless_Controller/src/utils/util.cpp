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
int profilePressures[5][4];
bool profileUpdated = false;

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