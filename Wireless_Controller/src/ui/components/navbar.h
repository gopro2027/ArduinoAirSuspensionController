#ifndef navbar_h
#define navbar_h

#include <Arduino.h>
#include "lvgl.h"

// Forward declarations
class Navbar;

// Navbar tab indices
enum NavbarTab {
    TAB_HOME = 0,
    TAB_PRESETS = 1,
    TAB_SETTINGS = 2
};

// Callback type for tab changes
typedef void (*NavbarChangeCallback)(uint32_t newTabIndex);

class Navbar {
public:
    lv_obj_t* tabview;           // The main tabview object
    lv_obj_t* tabbar;            // The tab bar (navbar) object
    lv_obj_t* tabs[3];           // Tab content objects
    lv_obj_t* tabIcons[3];       // Icon labels for each tab
    lv_obj_t* tabLabels[3];      // Text labels for each tab

    Navbar();

    // Create the tabview with bottom navbar
    // Returns the tabview object
    lv_obj_t* create(lv_obj_t* parent);

    // Get tab content object by index (for adding screen content)
    lv_obj_t* getTabContent(uint32_t index);

    // Set the active tab with optional animation
    void setActiveTab(uint32_t index, bool animate = true);

    // Get the current active tab index
    uint32_t getActiveTab();

    // Enable/disable swipe navigation
    void setSwipeEnabled(bool enabled);

    // Update navbar styling (for theme changes)
    void updateStyles();

    // Register callback for tab changes
    void setChangeCallback(NavbarChangeCallback callback);

    // Cleanup resources
    void cleanup();

private:
    uint32_t activeTabIndex;
    NavbarChangeCallback changeCallback;
    bool swipeEnabled;

    // Internal: style the tab bar to match current design
    void styleTabBar();

    // Internal: update tab button colors based on active state
    void updateTabColors(uint32_t activeIndex);

    // Friend function for event callback
    friend void tabview_value_changed_cb(lv_event_t* e);
};

// Global navbar instance
extern Navbar globalNavbar;

#endif
