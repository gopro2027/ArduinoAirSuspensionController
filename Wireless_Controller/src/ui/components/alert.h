#ifndef alert_h
#define alert_h

#include <Arduino.h>
#include "lvgl.h"

#include "utils/util.h"
#include "utils/touch_lib.h"

#include "Scr.h"

class Scr;

// Global alert state - shared across all screens
struct GlobalAlertState
{
    char currentMessage[64];   // Current active message
    lv_color_t currentColor;   // Current message color
    bool hasActiveAlert;       // True if there's an active (non-dismissed) alert
    bool isDismissed;          // True if the alert was dismissed
    void *originScreen;        // Pointer to the screen that first showed this alert
};

extern GlobalAlertState globalAlertState;

// Helper functions to manage global state
void globalAlertSetActive(const char *message, lv_color_t color, void *originScreen);
void globalAlertDismiss();
void globalAlertClear();
bool isGlobalAlertActive();
bool isGlobalAlertDismissed();

class Alert
{
public:
    Scr *parentScr;
    lv_obj_t *container;      // Toast container at top
    lv_obj_t *text;           // Message text
    lv_obj_t *dismissBtn;     // X dismiss button
    lv_obj_t *statusIcon;     // Small persistent icon in top-left
    lv_obj_t *iconContainer;  // Container for status icon
    unsigned long expiry;
    bool dismissed;           // Track if user dismissed manually (local)
    lv_color_t lastColor;     // Store last notification color for icon
    char lastMessage[64];     // Store last message for icon tooltip
    bool showIconEnabled;     // Whether to show the persistent icon on this screen

    Alert(Scr *scr, bool enableIcon = true);
    void show(lv_color_t accentColor, char *text, unsigned long expiry);
    void showForced(lv_color_t accentColor, char *text, unsigned long expiry); // Force show even if dismissed
    void dismiss();
    void loop();
    void showIcon();
    void hideIcon();
    void syncFromGlobal();    // Sync icon state from global dismissed state
};

#endif
