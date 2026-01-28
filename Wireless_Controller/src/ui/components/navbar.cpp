#include "navbar.h"
#include "utils/util.h"
#include "utils/touch_lib.h"

Navbar globalNavbar;

// Static callback for tabview value change
void tabview_value_changed_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        Navbar* navbar = (Navbar*)lv_event_get_user_data(e);
        uint32_t idx = lv_tabview_get_tab_active(navbar->tabview);

        navbar->updateTabColors(idx);
        navbar->activeTabIndex = idx;

        if (navbar->changeCallback) {
            navbar->changeCallback(idx);
        }
    }
}

Navbar::Navbar() {
    tabview = nullptr;
    tabbar = nullptr;
    activeTabIndex = 0;
    changeCallback = nullptr;
    swipeEnabled = false;
    for (int i = 0; i < 3; i++) {
        tabs[i] = nullptr;
        tabIcons[i] = nullptr;
        tabLabels[i] = nullptr;
    }
}

lv_obj_t* Navbar::create(lv_obj_t* parent) {
    const int navbarHeight = getNavbarHeight();
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();

    // Create tabview with bottom tab bar
    this->tabview = lv_tabview_create(parent);
    lv_tabview_set_tab_bar_position(this->tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(this->tabview, navbarHeight);

    // Remove default tabview styles (white borders/lines)
    lv_obj_set_size(this->tabview, screenWidth, screenHeight);
    lv_obj_set_style_bg_opa(this->tabview, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(this->tabview, 0, 0);
    lv_obj_set_style_pad_all(this->tabview, 0, 0);
    lv_obj_remove_flag(this->tabview, LV_OBJ_FLAG_SCROLLABLE);

    // Get and style the content container
    lv_obj_t* content = lv_tabview_get_content(this->tabview);
    if (content) {
        lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(content, 0, 0);
        lv_obj_set_style_pad_all(content, 0, 0);
    }

    // Get the tab bar for styling
    this->tabbar = lv_tabview_get_tab_bar(this->tabview);

    // Add the three tabs
    const char* tabNames[] = {"Home", "Presets", "Settings"};
    for (int i = 0; i < 3; i++) {
        this->tabs[i] = lv_tabview_add_tab(this->tabview, tabNames[i]);
        lv_obj_remove_flag(this->tabs[i], LV_OBJ_FLAG_SCROLLABLE);
        // Remove default styles from tab content
        lv_obj_set_style_bg_opa(this->tabs[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(this->tabs[i], 0, 0);
        lv_obj_set_style_pad_all(this->tabs[i], 0, 0);
    }

    // Style the tab bar
    styleTabBar();

    // Disable swipe by default
    setSwipeEnabled(false);

    // Register value changed callback
    lv_obj_add_event_cb(this->tabview, tabview_value_changed_cb,
                        LV_EVENT_VALUE_CHANGED, this);

    return this->tabview;
}

void Navbar::styleTabBar() {
    // Colors - matching existing design
    const uint32_t bgColor = GENERIC_GREY_VERY_DARK;

    // Style the tab bar container - just style, don't change layout
    lv_obj_set_style_bg_color(this->tabbar, lv_color_hex(bgColor), 0);
    lv_obj_set_style_bg_opa(this->tabbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(this->tabbar, 0, 0);
    lv_obj_set_style_pad_all(this->tabbar, 0, 0);
    lv_obj_set_style_radius(this->tabbar, 0, 0);
    lv_obj_remove_flag(this->tabbar, LV_OBJ_FLAG_SCROLLABLE);

    // Icons for each tab
    const char* icons[] = {
        LV_SYMBOL_HOME,
        LV_SYMBOL_LIST,
        LV_SYMBOL_SETTINGS
    };

    // Style the tab buttons - children (0, 1, 2) of tabbar
    uint32_t childCount = lv_obj_get_child_count(this->tabbar);
    for (uint32_t i = 0; i < childCount && i < 3; i++) {
        lv_obj_t* tabBtn = lv_obj_get_child(this->tabbar, i);

        // Style each tab button - remove all default styling
        lv_obj_set_style_bg_opa(tabBtn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(tabBtn, LV_OPA_TRANSP, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(tabBtn, 0, 0);
        lv_obj_set_style_border_width(tabBtn, 0, LV_STATE_CHECKED);
        lv_obj_set_style_shadow_width(tabBtn, 0, 0);
        lv_obj_set_style_outline_width(tabBtn, 0, 0);
        lv_obj_set_style_outline_width(tabBtn, 0, LV_STATE_CHECKED);

        // Set flex layout for icon + label vertical arrangement
        lv_obj_set_flex_flow(tabBtn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tabBtn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tabBtn, scaledY(2), 0);

        // Find and modify the existing label (created by lv_tabview_add_tab)
        lv_obj_t* existingLabel = lv_obj_get_child(tabBtn, 0);
        if (existingLabel) {
            // Store as text label
            this->tabLabels[i] = existingLabel;
            lv_obj_set_style_text_font(existingLabel, &lv_font_montserrat_10, 0);
            lv_obj_remove_flag(existingLabel, LV_OBJ_FLAG_CLICKABLE);

            // Create icon above the text label
            this->tabIcons[i] = lv_label_create(tabBtn);
            lv_label_set_text(this->tabIcons[i], icons[i]);
            lv_obj_set_style_text_font(this->tabIcons[i], &lv_font_montserrat_16, 0);
            lv_obj_remove_flag(this->tabIcons[i], LV_OBJ_FLAG_CLICKABLE);

            // Move icon to be first child (above text)
            lv_obj_move_to_index(this->tabIcons[i], 0);
        }
    }

    // Update tab colors for initial state
    updateTabColors(this->activeTabIndex);
}

lv_obj_t* Navbar::getTabContent(uint32_t index) {
    if (index < 3) {
        return this->tabs[index];
    }
    return nullptr;
}

void Navbar::setActiveTab(uint32_t index, bool animate) {
    if (index >= 3) return;
    lv_tabview_set_active(this->tabview, index,
                          animate ? LV_ANIM_ON : LV_ANIM_OFF);
}

uint32_t Navbar::getActiveTab() {
    if (this->tabview) {
        return lv_tabview_get_tab_active(this->tabview);
    }
    return 0;
}

void Navbar::setSwipeEnabled(bool enabled) {
    this->swipeEnabled = enabled;
    if (this->tabview) {
        // Get the content container of the tabview
        lv_obj_t* content = lv_tabview_get_content(this->tabview);
        if (content) {
            if (enabled) {
                lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_scroll_snap_x(content, LV_SCROLL_SNAP_CENTER);
            } else {
                lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
            }
        }
    }
}

void Navbar::updateTabColors(uint32_t activeIndex) {
    if (!this->tabbar) return;

    const uint32_t accentColor = THEME_COLOR_LIGHT;       // Active icon color
    const uint32_t activeTextColor = 0xFFFFFF;            // Active label color (white)
    const uint32_t inactiveTextColor = 0x64748B;          // Inactive color (both)

    // Update colors for icons and labels separately
    for (uint32_t i = 0; i < 3; i++) {
        bool isActive = (i == activeIndex);

        // Icon color: accent when active, gray when inactive
        if (this->tabIcons[i]) {
            uint32_t iconColor = isActive ? accentColor : inactiveTextColor;
            lv_obj_set_style_text_color(this->tabIcons[i], lv_color_hex(iconColor), 0);
        }

        // Label color: white when active, gray when inactive
        if (this->tabLabels[i]) {
            uint32_t labelColor = isActive ? activeTextColor : inactiveTextColor;
            lv_obj_set_style_text_color(this->tabLabels[i], lv_color_hex(labelColor), 0);
        }
    }
}

void Navbar::updateStyles() {
    if (!this->tabbar) return;

    // Update tab colors
    updateTabColors(this->activeTabIndex);
}

void Navbar::setChangeCallback(NavbarChangeCallback callback) {
    this->changeCallback = callback;
}

void Navbar::cleanup() {
    // Reset pointers - LVGL handles deletion when parent is deleted
    tabview = nullptr;
    tabbar = nullptr;
    activeTabIndex = 0;
    for (int i = 0; i < 3; i++) {
        tabs[i] = nullptr;
        tabIcons[i] = nullptr;
        tabLabels[i] = nullptr;
    }
}
