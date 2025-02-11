#ifndef util_h
#define util_h

#include <BTOas.h>
#include <preferencable.h>
#include "ui/components/Scr.h"
#include "lvgl.h"
#include "ui/components/option.h"

class Scr;
class Option;

struct SimplePoint
{
    double x, y;
};

struct SimpleRect
{
    double x, y, w, h;
};

struct CenterRect
{
    double cx, cy, w, h;
};

int sr_contains(SimpleRect r, SimplePoint p);

int cr_contains(CenterRect cr, SimplePoint p);

#define ARROW_BUTTON_WIDTH 54
#define ARROW_BUTTON_HEIGHT 44

// first column (left)
extern CenterRect ctr_row0col0up;
extern CenterRect ctr_row0col0down;

extern CenterRect ctr_row1col0up;
extern CenterRect ctr_row1col0down;

// second column (center)
extern CenterRect ctr_row0col1up;
extern CenterRect ctr_row0col1down;

extern CenterRect ctr_row1col1up;
extern CenterRect ctr_row1col1down;

// third column (right)
extern CenterRect ctr_row0col2up;
extern CenterRect ctr_row0col2down;

extern CenterRect ctr_row1col2up;
extern CenterRect ctr_row1col2down;

// bottom nav
extern SimpleRect navbarbtn_home;
extern SimpleRect navbarbtn_presets;
extern SimpleRect navbarbtn_settings;

// presets buttons
extern CenterRect ctr_preset_1;
extern CenterRect ctr_preset_2;
extern CenterRect ctr_preset_3;
extern CenterRect ctr_preset_4;
extern CenterRect ctr_preset_5;

// preset save and load
extern SimpleRect preset_save;
extern SimpleRect preset_load;

extern int currentPressures[5];
extern uint16_t statusBittset;
extern int profilePressures[5][4];
extern bool profileUpdated;

// returns 0 if none to send
bool getBTRestPacketToSend(BTOasPacket *copyTo);
void sendRestPacket(BTOasPacket *packet);
void setupRestSemaphore();

void showDialog(char *text, lv_color_t color = {0, 0, 0xff}, unsigned long durationMS = 5000);
void dialogLoop();

unsigned int getValveControlValue();
void setValveBit(int bit);
void closeValves();

void setupPressureLabel(lv_obj_t *parent, lv_obj_t **label, int x, int y, lv_align_t align, const char *defaultText);

extern Scr *screens[3];

enum UNITS_MODE
{
    PSI,
    BAR
};

class SaveData
{
public:
    Preferencable unitsMode;
};

extern SaveData _SaveData;
void beginSaveData();
int getUnits();
void setUnits(int value);

void initKB();
void ta_event_cb(lv_event_t *e);
bool isKeyboardHidden();
#endif