#ifndef THEME_COLORS_H
#define THEME_COLORS_H

#include <stdint.h>

// Default theme colors (purple/lavender theme)
#define DEFAULT_THEME_COLOR_LIGHT  0xA78BFA  // Light purple
#define DEFAULT_THEME_COLOR_MEDIUM 0x8B5CF6  // Medium purple
#define DEFAULT_THEME_COLOR_DARK   0x6D28D9  // Dark purple

// Undefine old static color definitions from user_defines.h
#ifdef THEME_COLOR_LIGHT
#undef THEME_COLOR_LIGHT
#endif
#ifdef THEME_COLOR_MEDIUM
#undef THEME_COLOR_MEDIUM
#endif
#ifdef THEME_COLOR_DARK
#undef THEME_COLOR_DARK
#endif

// Theme color accessors (use getters to get dynamic values)
#define THEME_COLOR_LIGHT  getThemeColorLight()
#define THEME_COLOR_MEDIUM getThemeColorMedium()
#define THEME_COLOR_DARK   getThemeColorDark()

// Getter functions - return current theme colors
uint32_t getThemeColorLight();
uint32_t getThemeColorMedium();
uint32_t getThemeColorDark();

// Setter functions - update theme colors and save to NVS
void setThemeColorLight(uint32_t color);
void setThemeColorMedium(uint32_t color);
void setThemeColorDark(uint32_t color);

// Initialize theme colors from NVS
void initThemeColors();

// Theme presets
enum ThemePreset {
    PRESET_PURPLE = 0,
    PRESET_BLUE = 1,
    PRESET_GREEN = 2,
    PRESET_CUSTOM = -1
};

// Apply a theme preset
void applyThemePreset(ThemePreset preset);

// Get current theme preset (-1 if custom)
int getCurrentThemePreset();

#endif // THEME_COLORS_H
