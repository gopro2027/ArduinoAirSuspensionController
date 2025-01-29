#ifndef util_h
#define util_h

#include <BTOas.h>
#include "ui/components/Scr.h"
#include "lvgl.h"

class Scr;

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

extern int currentPressures[5];

// returns 0 if none to send
bool getBTRestPacketToSend(BTOasPacket *copyTo);
void sendRestPacket(BTOasPacket *packet);
void setupRestSemaphore();

void showDialog(char *text, lv_color_t color = {0, 0, 0xff}, unsigned long durationMS = 5000);
void dialogLoop(Scr *scr);

#endif