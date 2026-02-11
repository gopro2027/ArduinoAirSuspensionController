#ifndef statusbar_h
#define statusbar_h

#include <Arduino.h>
#include "lvgl.h"

class Statusbar {
public:
    // Main statusbar container
    lv_obj_t* container;

    // Top bar elements
    lv_obj_t* batteryIcon;        // Battery symbol
    lv_obj_t* batteryLabel;       // Battery percentage text
    lv_obj_t* alertIcon;          // Alert warning icon (left side)

    // Pull-down panel elements
    lv_obj_t* overlay;            // Dark overlay behind panel
    lv_obj_t* pullDownPanel;      // Main pull-down panel
    lv_obj_t* batterySection;     // Battery info section
    lv_obj_t* alertSection;       // Alert notification section
    lv_obj_t* brightnessSection;  // Brightness slider section
    lv_obj_t* brightnessSlider;   // Slider control
    lv_obj_t* handleBar;          // Bottom handle indicator
    lv_obj_t* panelBatteryIcon;   // Battery icon in panel
    lv_obj_t* panelBatteryLabel;  // Full battery text in panel
    lv_obj_t* panelAlertIcon;     // Alert icon in panel
    lv_obj_t* panelAlertLabel;    // Alert message in panel

    // Status section elements
    lv_obj_t* statusSection;      // Status info section
    lv_obj_t* compressorFrozenLabel;
    lv_obj_t* accStatusLabel;
    lv_obj_t* ebrakeStatusLabel;
    lv_obj_t* compressorStatusLabel;

    Statusbar();

    // Create statusbar at top of screen
    void create(lv_obj_t* parent);

    // Update statusbar values (call in loop)
    void update();

    // Show/hide the statusbar
    void show();
    void hide();

    // Panel methods
    void openPanel();
    void closePanel();
    void togglePanel();

    // Cleanup resources
    void cleanup();

    // Get statusbar height for layout calculations
    static int getHeight();

private:
    bool visible;
    bool panelOpen;               // Track panel state
    bool hasActiveAlert;          // Track alert state

    void createPullDownPanel(lv_obj_t* parent);
    void updateBatteryStatus();
    void updateAlertStatus();
    void updateStatusSection();
};

// Global statusbar instance
extern Statusbar globalStatusbar;

#endif
