#include "theme_colors.h"
#include <Preferences.h>

// Current theme color values
static uint32_t currentThemeColorLight = DEFAULT_THEME_COLOR_LIGHT;
static uint32_t currentThemeColorMedium = DEFAULT_THEME_COLOR_MEDIUM;
static uint32_t currentThemeColorDark = DEFAULT_THEME_COLOR_DARK;

// NVS keys
static const char* NVS_NAMESPACE = "theme";
static const char* KEY_COLOR_LIGHT = "color_light";
static const char* KEY_COLOR_MEDIUM = "color_med";
static const char* KEY_COLOR_DARK = "color_dark";

// Getter functions
uint32_t getThemeColorLight() {
    return currentThemeColorLight;
}

uint32_t getThemeColorMedium() {
    return currentThemeColorMedium;
}

uint32_t getThemeColorDark() {
    return currentThemeColorDark;
}

// Setter functions
void setThemeColorLight(uint32_t color) {
    currentThemeColorLight = color;

    // Save to NVS
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putUInt(KEY_COLOR_LIGHT, color);
        prefs.end();
    }
}

void setThemeColorMedium(uint32_t color) {
    currentThemeColorMedium = color;

    // Save to NVS
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putUInt(KEY_COLOR_MEDIUM, color);
        prefs.end();
    }
}

void setThemeColorDark(uint32_t color) {
    currentThemeColorDark = color;

    // Save to NVS
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putUInt(KEY_COLOR_DARK, color);
        prefs.end();
    }
}

// Initialize theme colors from NVS
void initThemeColors() {
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, true)) {  // Read-only
        currentThemeColorLight = prefs.getUInt(KEY_COLOR_LIGHT, DEFAULT_THEME_COLOR_LIGHT);
        currentThemeColorMedium = prefs.getUInt(KEY_COLOR_MEDIUM, DEFAULT_THEME_COLOR_MEDIUM);
        currentThemeColorDark = prefs.getUInt(KEY_COLOR_DARK, DEFAULT_THEME_COLOR_DARK);
        prefs.end();
    }
}

// Apply a theme preset
void applyThemePreset(ThemePreset preset) {
    switch (preset) {
        case PRESET_PURPLE:
            setThemeColorLight(0xA78BFA);   // Light purple
            setThemeColorMedium(0x8B5CF6);  // Medium purple
            setThemeColorDark(0x6D28D9);    // Dark purple
            break;
        case PRESET_BLUE:
            setThemeColorLight(0x60A5FA);   // Light blue
            setThemeColorMedium(0x3B82F6);  // Medium blue
            setThemeColorDark(0x2563EB);    // Dark blue
            break;
        case PRESET_GREEN:
            setThemeColorLight(0x34D399);   // Light green
            setThemeColorMedium(0x10B981);  // Medium green
            setThemeColorDark(0x059669);    // Dark green
            break;
        case PRESET_CUSTOM:
        default:
            // Do nothing for custom
            break;
    }
}

// Get current theme preset (-1 if custom)
int getCurrentThemePreset() {
    // Check if current colors match any preset
    if (currentThemeColorLight == 0xA78BFA &&
        currentThemeColorMedium == 0x8B5CF6 &&
        currentThemeColorDark == 0x6D28D9) {
        return PRESET_PURPLE;
    }
    if (currentThemeColorLight == 0x60A5FA &&
        currentThemeColorMedium == 0x3B82F6 &&
        currentThemeColorDark == 0x2563EB) {
        return PRESET_BLUE;
    }
    if (currentThemeColorLight == 0x34D399 &&
        currentThemeColorMedium == 0x10B981 &&
        currentThemeColorDark == 0x059669) {
        return PRESET_GREEN;
    }
    return PRESET_CUSTOM;
}
