#ifndef util_h
#define util_h

#include <BTOas.h>
#include <preferencable.h>
#include "ui/components/Scr.h"
#include "lvgl.h"
#include "ui/components/option.h"

#include "device_lib_exports.h"

// Dynamic scaling based on current screen dimensions (supports rotation)
float getScaleX();
float getScaleY();
int getBaseWidth();
int getBaseHeight();

// Dynamic screen dimension helpers for rotation support
int getScreenWidth();
int getScreenHeight();
bool isLandscape();

// Scaled dimension helpers - scales from 240×320 reference
inline int scaled(int referenceValue) {
    return (int)(referenceValue * getScaleX());
}
inline int scaledX(int referenceValue) {
    return (int)(referenceValue * getScaleX());
}
inline int scaledY(int referenceValue) {
    return (int)(referenceValue * getScaleY());
}

// Dynamic UI constants that scale with display size
inline int getNavbarHeight() {
    return scaledY(50);
}

// Override compile-time constant with dynamic value
#ifdef NAVBAR_HEIGHT
#undef NAVBAR_HEIGHT
#endif
#define NAVBAR_HEIGHT getNavbarHeight()

// Legacy compile-time macros (use dynamic functions for rotation support)
#define SCALE_X getScaleX()
#define SCALE_Y getScaleY()

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

// Dynamic arrow button dimensions
float getArrowButtonWidth();
float getArrowButtonHeight();

// Dynamic touch area functions (recalculate on each call for rotation support)
// first column (left)
CenterRect get_ctr_row0col0up();
CenterRect get_ctr_row0col0down();
CenterRect get_ctr_row1col0up();
CenterRect get_ctr_row1col0down();

// second column (center)
CenterRect get_ctr_row0col1up();
CenterRect get_ctr_row0col1down();
CenterRect get_ctr_row1col1up();
CenterRect get_ctr_row1col1down();

// third column (right)
CenterRect get_ctr_row0col2up();
CenterRect get_ctr_row0col2down();
CenterRect get_ctr_row1col2up();
CenterRect get_ctr_row1col2down();

// bottom nav
SimpleRect get_navbarbtn_home();
SimpleRect get_navbarbtn_presets();
SimpleRect get_navbarbtn_settings();

// presets buttons
CenterRect get_ctr_preset(int num);

// preset save and load
SimpleRect get_preset_save();
SimpleRect get_preset_load();

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
void unsetValveBit(int bit);
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
    Preferencable screenRotation;
    // Theme colors
    Preferencable themeColorLight;
    Preferencable themeColorDark;
    Preferencable themeColorMedium;
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
headerDefineSaveFunc(screenRotation, byte);
headerDefineSaveFunc(themeColorLight, uint32_t);
headerDefineSaveFunc(themeColorDark, uint32_t);
headerDefineSaveFunc(themeColorMedium, uint32_t);

// Theme presets enum
enum ThemePreset {
    PRESET_CUSTOM = -1,
    PRESET_BLUE = 0,
    PRESET_PURPLE = 1,
    PRESET_GREEN = 2,
    PRESET_DESERT_SAND = 3
};

#define GENERIC_GREY_VERY_DARK 0x121212
#define GENERIC_GREY_DARK 0x1F1F1F
#define GENERIC_GREY 0x4e4954
#define GENERIC_GREY_LIGHT 0x6a6571

// Default theme colors (purple/lavender theme)
#define THEME_COLOR_PLUMP_PURPLE_LIGHT  0xA78BFA  // Light purple
#define THEME_COLOR_PLUMP_PURPLE_MEDIUM 0x8B5CF6  // Medium purple
#define THEME_COLOR_PLUMP_PURPLE_DARK   0x6D28D9  // Dark purple

#define THEME_COLOR_OCEAN_BLUE_LIGHT 0x60A5FA
#define THEME_COLOR_OCEAN_BLUE_MEDIUM 0x3B82F6
#define THEME_COLOR_OCEAN_BLUE_DARK 0x2563EB

#define THEME_COLOR_FOREST_GREEN_LIGHT 0x34D399
#define THEME_COLOR_FOREST_GREEN_MEDIUM 0x10B981
#define THEME_COLOR_FOREST_GREEN_DARK 0x059669

// Desert Sand (#E7D399) theme (light/medium/dark shades)
#define THEME_COLOR_DESERT_SAND_LIGHT  0xE7D399
#define THEME_COLOR_DESERT_SAND_MEDIUM 0xC4B382
#define THEME_COLOR_DESERT_SAND_DARK   0xA2946B

// Theme color accessors (use getters to get dynamic values)
#define THEME_COLOR_LIGHT  getthemeColorLight()
#define THEME_COLOR_MEDIUM getthemeColorMedium()
#define THEME_COLOR_DARK   getthemeColorDark()

void applyThemePreset(ThemePreset preset);
int getCurrentThemePreset();

void ta_event_cb(lv_event_t *e);
void slider_event_cb(lv_event_t *e);
bool isKeyboardHidden();
float getBrightnessFloat();

// Screen rotation helpers
#if SUPPORTS_ROTATION == 1
void applyScreenRotation(byte rotation);
#endif
void reinitializeScreens();


#endif