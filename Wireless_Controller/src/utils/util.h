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

// Physical (DPI-based) sizing. Use for touch targets that must be a consistent real-world
// size regardless of panel resolution (e.g. settings rows). DEVICE_DPI is the panel's true
// pixels-per-inch, defined per-device in each device_libs/<dev>/device_lib_exports.h. There is
// no way to read physical size from the hardware, so it must be declared per panel.
#ifndef DEVICE_DPI
#define DEVICE_DPI 143 // fallback: 2.8in 240x320 reference panel (~143 dpi)
#endif
inline int mmToPx(float mm) {
    return (int)(mm * (float)DEVICE_DPI / 25.4f + 0.5f);
}

// DPI the hardcoded UI font sizes were originally tuned at (ws2p8, 2.8in 240x320 ~143 dpi).
#define UI_BASELINE_DPI 143

// Cap on how much text is enlarged for high-DPI panels. Glyphs are rasterized on the CPU (no GPU)
// and blend cost grows ~quadratically with size; on the 480x640 RGB 2.8b panel a full 2.0x
// (286/143 dpi) font scale dropped scroll to ~5 FPS. Capping the *font* multiplier (touch targets
// still use mmToPx for row height) roughly halves glyph blend cost while keeping text clearly
// larger than baseline. Only panels above this ratio are affected: ws2p8b ~2.0x -> clamped;
// ws3p5/ws3p5b ~1.15x and ws1p8knob ~1.4x are below the cap and unchanged. Per-device overridable
// in device_lib_exports.h.
#ifndef UI_MAX_FONT_SCALE
#define UI_MAX_FONT_SCALE 1.5f
#endif

// getScaledFont(baselinePx) -> a compiled LVGL font whose pixel size approximates `baselinePx`
// scaled from the baseline-DPI design to this device's real DPI (capped by UI_MAX_FONT_SCALE),
// snapped to the nearest available compiled size. Use it instead of a raw &lv_font_montserrat_N for
// any on-screen text/icons; LVGL bitmap fonts can't be scaled at runtime, so heights only track DPI
// if we pick a bigger font here.
//
// This resolves ENTIRELY AT COMPILE TIME (it's a macro over the templates below), so each call site
// references exactly one lv_font_montserrat_* symbol. Fonts that no call site selects on a given
// device are never referenced, so the linker never pulls their translation unit out of the LVGL
// archive -> they don't bloat that variant's .bin. (A runtime lookup table, by contrast, names
// every font and forces all of them into every device's firmware.)
//
// Requirements:
//  - `baselinePx` MUST be a compile-time constant at every call site (all current callers pass a
//    literal). Passing a runtime value is a compile error (it can't be a template argument).
//  - DEVICE_DPI/UI_BASELINE_DPI/UI_MAX_FONT_SCALE must be defined before this point (they are, and
//    util.h pulls in device_lib_exports.h up top so DEVICE_DPI is the panel's real value).
//  - Every size listed in uiFontForPx() must be enabled in include/lv_conf.h (LV_FONT_MONTSERRAT_*)
//    so its symbol is declared; keep uiSnapFontPx()'s list and uiFontForPx()'s cases in sync.

// Design px -> this panel's px, using the panel DPI ratio capped at UI_MAX_FONT_SCALE.
constexpr int uiScaledTargetPx(int baselinePx) {
    float scale = (float)DEVICE_DPI / (float)UI_BASELINE_DPI;
    if (scale > (float)UI_MAX_FONT_SCALE) scale = (float)UI_MAX_FONT_SCALE;
    return (int)((float)baselinePx * scale + 0.5f);
}

// Snap a target px to the nearest size that has a case in uiFontForPx() below.
constexpr int uiSnapFontPx(int target) {
    const int sizes[] = {10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 40};
    int best = sizes[0];
    int bestDiff = target - best;
    if (bestDiff < 0) bestDiff = -bestDiff;
    for (int i = 1; i < (int)(sizeof(sizes) / sizeof(sizes[0])); i++) {
        int diff = target - sizes[i];
        if (diff < 0) diff = -diff;
        if (diff < bestDiff) { bestDiff = diff; best = sizes[i]; }
    }
    return best;
}

// Map an already-snapped px to its font. `if constexpr` discards the non-matching branches for each
// instantiation, so only the selected font symbol is odr-used.
template <int px>
constexpr const lv_font_t *uiFontForPx() {
    if constexpr (px == 10) return &lv_font_montserrat_10;
    else if constexpr (px == 12) return &lv_font_montserrat_12;
    else if constexpr (px == 14) return &lv_font_montserrat_14;
    else if constexpr (px == 16) return &lv_font_montserrat_16;
    else if constexpr (px == 18) return &lv_font_montserrat_18;
    else if constexpr (px == 20) return &lv_font_montserrat_20;
    else if constexpr (px == 22) return &lv_font_montserrat_22;
    else if constexpr (px == 24) return &lv_font_montserrat_24;
    else if constexpr (px == 28) return &lv_font_montserrat_28;
    else if constexpr (px == 32) return &lv_font_montserrat_32;
    else if constexpr (px == 40) return &lv_font_montserrat_40;
    else return &lv_font_montserrat_14; // unreachable while the snap list matches the cases above
}

#define getScaledFont(baselinePx) (::uiFontForPx< ::uiSnapFontPx(::uiScaledTargetPx((baselinePx))) >())

// Dynamic UI constants that scale with display size
inline int getNavbarHeight() {
    return scaledY(50);
}
inline int getStatusbarHeight() {
    return scaledY(18);
}

// Override compile-time constant with dynamic value
#ifdef NAVBAR_HEIGHT
#undef NAVBAR_HEIGHT
#endif
#define NAVBAR_HEIGHT getNavbarHeight()

#ifdef STATUSBAR_HEIGHT
#undef STATUSBAR_HEIGHT
#endif
#define STATUSBAR_HEIGHT getStatusbarHeight()

// Legacy compile-time macros (use dynamic functions for rotation support)
#define SCALE_X getScaleX()
#define SCALE_Y getScaleY()

void scale_obj(lv_obj_t *obj, int w, int h);
void scale_img(lv_obj_t *obj, lv_image_dsc_t img);

class Scr;
class Option;

struct SimpleRect // used in ui_scrPresets.cpp.
{
    int x, y, w, h;
};

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
void setManifoldConfigValuesFlag(ConfigFlagsBit configFlagBit, bool value);
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

#ifdef HAS_ROTARY_ENCODER
#include "bidi_switch_knob.h"
extern knob_handle_t g_knob_handle;
#endif

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
    // Navigation
    Preferencable swipeNavigation;
};

extern SaveData _SaveData;
void beginSaveData();
headerDefineSaveFunc(unitsMode, int);
headerDefineSaveFunc(blePasskey, uint32_t);
headerDefineSaveFunc(screenDimTimeM, uint32_t);
unsigned long getScreenDimTimeMs();
headerDefineSaveFunc(updateMode, bool);
headerDefineSaveFunc(wifiSSID, String);
headerDefineSaveFunc(wifiPassword, String);
headerDefineSaveFunc(updateResult, byte);
headerDefineSaveFunc(brightness, byte);
headerDefineSaveFunc(screenRotation, byte);
headerDefineSaveFunc(themeColorLight, uint32_t);
headerDefineSaveFunc(themeColorDark, uint32_t);
headerDefineSaveFunc(themeColorMedium, uint32_t);
headerDefineSaveFunc(swipeNavigation, bool);

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