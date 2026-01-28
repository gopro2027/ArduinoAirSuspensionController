#ifndef alert_h
#define alert_h

#include <Arduino.h>
#include "lvgl.h"
#include "../ui_helpers.h"
#include "../ui_events.h"

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
    unsigned long expiry;
    bool dismissed;           // Track if user dismissed manually (local)
    lv_color_t lastColor;     // Store last notification color
    char lastMessage[64];     // Store last message

    Alert(Scr *scr);
    void show(lv_color_t accentColor, char *text, unsigned long expiry);
    void showForced(lv_color_t accentColor, char *text, unsigned long expiry);
    void dismiss();
    void loop();
    void syncFromGlobal();
};

#endif
