#ifndef util_h
#define util_h

#include <BTOas.h>
#include <preferencable.h>
#include "ui/components/Scr.h"
#include "lvgl.h"
#include "ui/components/option.h"

#include "device_lib_exports.h"

#define SCALE_X (LCD_WIDTH / 240.0f)
#define SCALE_Y (LCD_HEIGHT / 320.0f)

// ---------- Layout offsets for preset screen (design space 240x320) ----------
// These are expressed in "design" pixels (240x320) and then converted
// to device pixels using SCALE_X / SCALE_Y in the UI code.
//
// For each known resolution, you can hand-tune the offsets.
// For anything else, the fallback scales them proportionally.

#if (LCD_WIDTH == 240) && (LCD_HEIGHT == 320)
// Original device: no extra offsets
  #define CAR_PAGE_OFFSET_DESIGN_X         0.0f
  #define CAR_PAGE_OFFSET_DESIGN_Y         0.0f
  #define PRESET_OVERLAY_OFFSET_DESIGN_X   0.0f
  #define PRESET_OVERLAY_OFFSET_DESIGN_Y   0.0f

#elif (LCD_WIDTH == 320) && (LCD_HEIGHT == 480)
// New 320x480 device: values you tuned to look correct
  #define CAR_PAGE_OFFSET_DESIGN_X         40.0f
  #define CAR_PAGE_OFFSET_DESIGN_Y         0.0f
  #define PRESET_OVERLAY_OFFSET_DESIGN_X   5.0f
  #define PRESET_OVERLAY_OFFSET_DESIGN_Y   10.0f

#else
// Fallback for other resolutions: scale relative to a 320x480 baseline.
// You can tweak these formulas or add more #elif blocks for specific panels.
  #define CAR_PAGE_OFFSET_DESIGN_X       (40.0f * (LCD_WIDTH  / 320.0f))
  #define CAR_PAGE_OFFSET_DESIGN_Y       ( 0.0f * (LCD_HEIGHT / 480.0f))
  #define PRESET_OVERLAY_OFFSET_DESIGN_X ( 5.0f * (LCD_WIDTH  / 320.0f))
  #define PRESET_OVERLAY_OFFSET_DESIGN_Y (10.0f * (LCD_HEIGHT / 480.0f))
#endif

void scale_obj(lv_obj_t *obj, int w, int h);
void scale_img(lv_obj_t *obj, lv_image_dsc_t img);

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

#define ARROW_BUTTON_WIDTH 54 * SCALE_X
#define ARROW_BUTTON_HEIGHT 44 * SCALE_Y

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

void runNextFrame(std::function<void()> function);
void handleFunctionRunOnNextFrame();

extern int currentPressures[5];
extern uint32_t statusBittset;
extern uint8_t AIPercentage;
extern uint8_t AIReadyBittset;
extern int profilePressures[5][4];
extern bool profileUpdated;
extern int currentPreset;
void requestPreset();
extern ConfigValuesPacket util_configValues;
extern UpdateStatusRequestPacket util_statusRequestPacket;
void sendConfigValuesPacket(bool saveToManifold);
void onBLEConnectionCompleted();

// returns 0 if none to send
void clearPackets();
bool getBTRestPacketToSend(BTOasPacket *copyTo);
void sendRestPacket(BTOasPacket *packet);
void setupRestSemaphore();

void showDialog(const char *text, lv_color_t color = {0, 0, 0xff}, unsigned long durationMS = 5000);
void dialogLoop();

unsigned int getValveControlValue();
void setValveBit(int bit);
void closeValves();

void setupPressureLabel(lv_obj_t *parent, lv_obj_t **label, int x, int y, lv_align_t align, const char *defaultText);

extern Scr *screens[3];
extern Scr *currentScr;

enum UNITS_MODE
{
    PSI,
    BAR
};

class SaveData
{
public:
    Preferencable unitsMode;
    Preferencable blePasskey;
    Preferencable screenDimTimeM;
    Preferencable updateMode;
    Preferencable wifiSSID;
    Preferencable wifiPassword;
    Preferencable updateResult;
    Preferencable brightness;
};

extern SaveData _SaveData;
void beginSaveData();
headerDefineSaveFunc(unitsMode, int);
headerDefineSaveFunc(blePasskey, uint32_t);
headerDefineSaveFunc(screenDimTimeM, uint32_t);
headerDefineSaveFunc(updateMode, bool);
headerDefineSaveFunc(wifiSSID, String);
headerDefineSaveFunc(wifiPassword, String);
headerDefineSaveFunc(updateResult, byte);
headerDefineSaveFunc(brightness, byte);

void ta_event_cb(lv_event_t *e);
void slider_event_cb(lv_event_t *e);
bool isKeyboardHidden();
float getBrightnessFloat();
#endif
