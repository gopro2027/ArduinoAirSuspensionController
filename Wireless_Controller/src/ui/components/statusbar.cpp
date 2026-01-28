#include "statusbar.h"
#include "utils/util.h"
#include "alert.h"
#include "waveshare/waveshare.h"
#include "waveshare/board_driver_util.h"

Statusbar globalStatusbar;

// Styling constants
static const uint32_t STATUSBAR_BG_COLOR = 0x0D0D0D;
static const uint32_t STATUSBAR_TEXT_COLOR = 0xE0E0E0;
static const uint32_t STATUSBAR_ICON_COLOR = 0xB0B0B0;
static const uint32_t PANEL_BG_COLOR = 0x141414;
static const uint32_t PANEL_SECTION_COLOR = 0x1F1F1F;
static const uint32_t PANEL_BORDER_COLOR = 0x2A2A2A;
static const uint32_t PANEL_HANDLE_COLOR = 0x4A4A4A;
static const uint32_t PANEL_SLIDER_TRACK_COLOR = 0x4A4A5A;

// Forward declaration for callbacks
static void statusbar_click_cb(lv_event_t* e);
static void statusbar_overlay_click_cb(lv_event_t* e);
static void brightness_slider_cb(lv_event_t* e);

Statusbar::Statusbar() {
    container = nullptr;
    batteryIcon = nullptr;
    batteryLabel = nullptr;
    alertIcon = nullptr;
    overlay = nullptr;
    pullDownPanel = nullptr;
    batterySection = nullptr;
    alertSection = nullptr;
    brightnessSection = nullptr;
    brightnessSlider = nullptr;
    handleBar = nullptr;
    panelBatteryIcon = nullptr;
    panelBatteryLabel = nullptr;
    panelAlertIcon = nullptr;
    panelAlertLabel = nullptr;
    visible = true;
    panelOpen = false;
    hasActiveAlert = false;
    isClosing = false;
}

void Statusbar::create(lv_obj_t* parent) {
    const int screenWidth = getScreenWidth();
    const int statusbarHeight = STATUSBAR_HEIGHT;
    const int padding = scaledX(8);

    // Create main statusbar container
    container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, screenWidth, statusbarHeight);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(STATUSBAR_BG_COLOR), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(container, statusbar_click_cb, LV_EVENT_CLICKED, this);

    // Alert icon (left side, hidden by default)
    alertIcon = lv_label_create(container);
    lv_label_set_text(alertIcon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(alertIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(alertIcon, &lv_font_montserrat_10, 0);
    lv_obj_align(alertIcon, LV_ALIGN_LEFT_MID, padding, 0);
    lv_obj_add_flag(alertIcon, LV_OBJ_FLAG_HIDDEN);

    // Battery icon (right side)
    batteryIcon = lv_label_create(container);
    lv_label_set_text(batteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(batteryIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(batteryIcon, &lv_font_montserrat_10, 0);
    lv_obj_align(batteryIcon, LV_ALIGN_RIGHT_MID, -padding - scaledX(30), 0);

    // Battery percentage label
    batteryLabel = lv_label_create(container);
    lv_label_set_text(batteryLabel, "100%");
    lv_obj_set_style_text_color(batteryLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_10, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -padding, 0);

    // Center line marker (pull indicator)
    lv_obj_t* centerLine = lv_obj_create(container);
    lv_obj_remove_style_all(centerLine);
    lv_obj_set_size(centerLine, scaledX(30), scaledY(3));
    lv_obj_align(centerLine, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(centerLine, lv_color_hex(PANEL_HANDLE_COLOR), 0);
    lv_obj_set_style_bg_opa(centerLine, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(centerLine, scaledY(2), 0);
    lv_obj_remove_flag(centerLine, LV_OBJ_FLAG_CLICKABLE);

    // Create the pull-down panel
    createPullDownPanel(parent);

    visible = true;
}

void Statusbar::createPullDownPanel(lv_obj_t* parent) {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();
    const int sectionWidth = screenWidth - scaledX(24);
    const int sectionHeight = scaledY(50);
    const int sectionRadius = scaledY(8);
    const int sectionPadding = scaledX(10);

    // Create overlay (dark background)
    overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, screenWidth, screenHeight);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, statusbar_overlay_click_cb, LV_EVENT_CLICKED, this);

    // Create pull-down panel (initially off-screen)
    pullDownPanel = lv_obj_create(parent);
    lv_obj_remove_style_all(pullDownPanel);
    lv_obj_set_size(pullDownPanel, screenWidth, screenHeight);
    lv_obj_set_pos(pullDownPanel, 0, -screenHeight);
    lv_obj_set_style_bg_color(pullDownPanel, lv_color_hex(PANEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa(pullDownPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pullDownPanel, 1, 0);
    lv_obj_set_style_border_color(pullDownPanel, lv_color_hex(PANEL_BORDER_COLOR), 0);
    lv_obj_set_style_border_side(pullDownPanel, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_remove_flag(pullDownPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pullDownPanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pullDownPanel, statusbar_overlay_click_cb, LV_EVENT_CLICKED, this);

    // Battery section (y=45)
    batterySection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(batterySection);
    lv_obj_set_size(batterySection, sectionWidth, sectionHeight);
    lv_obj_align(batterySection, LV_ALIGN_TOP_MID, 0, scaledY(45));
    lv_obj_set_style_bg_color(batterySection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(batterySection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(batterySection, sectionRadius, 0);
    lv_obj_set_style_pad_all(batterySection, sectionPadding, 0);
    lv_obj_remove_flag(batterySection, LV_OBJ_FLAG_SCROLLABLE);

    // Battery icon in panel
    panelBatteryIcon = lv_label_create(batterySection);
    lv_label_set_text(panelBatteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(panelBatteryIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(panelBatteryIcon, &lv_font_montserrat_20, 0);
    lv_obj_align(panelBatteryIcon, LV_ALIGN_LEFT_MID, 0, 0);

    // Battery label in panel
    panelBatteryLabel = lv_label_create(batterySection);
    lv_label_set_text(panelBatteryLabel, "Battery: 100%");
    lv_obj_set_style_text_color(panelBatteryLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(panelBatteryLabel, &lv_font_montserrat_10, 0);
    lv_obj_align(panelBatteryLabel, LV_ALIGN_LEFT_MID, scaledX(30), 0);

    // Alert section (y=100)
    alertSection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(alertSection);
    lv_obj_set_size(alertSection, sectionWidth, sectionHeight);
    lv_obj_align(alertSection, LV_ALIGN_TOP_MID, 0, scaledY(100));
    lv_obj_set_style_bg_color(alertSection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(alertSection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(alertSection, sectionRadius, 0);
    lv_obj_set_style_pad_all(alertSection, sectionPadding, 0);
    lv_obj_remove_flag(alertSection, LV_OBJ_FLAG_SCROLLABLE);

    // Alert icon in panel
    panelAlertIcon = lv_label_create(alertSection);
    lv_label_set_text(panelAlertIcon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(panelAlertIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(panelAlertIcon, &lv_font_montserrat_20, 0);
    lv_obj_align(panelAlertIcon, LV_ALIGN_LEFT_MID, 0, 0);

    // Alert label in panel
    panelAlertLabel = lv_label_create(alertSection);
    lv_label_set_text(panelAlertLabel, "No alerts");
    lv_obj_set_style_text_color(panelAlertLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(panelAlertLabel, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(panelAlertLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(panelAlertLabel, sectionWidth - scaledX(50));
    lv_obj_align(panelAlertLabel, LV_ALIGN_LEFT_MID, scaledX(30), 0);

    // Brightness section (y=155)
    brightnessSection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(brightnessSection);
    lv_obj_set_size(brightnessSection, sectionWidth, sectionHeight);
    lv_obj_align(brightnessSection, LV_ALIGN_TOP_MID, 0, scaledY(155));
    lv_obj_set_style_bg_color(brightnessSection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(brightnessSection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(brightnessSection, sectionRadius, 0);
    lv_obj_set_style_pad_all(brightnessSection, sectionPadding, 0);
    lv_obj_remove_flag(brightnessSection, LV_OBJ_FLAG_SCROLLABLE);

    // Brightness icon
    lv_obj_t* brightnessIcon = lv_label_create(brightnessSection);
    lv_label_set_text(brightnessIcon, LV_SYMBOL_IMAGE);  // Sun/brightness icon
    lv_obj_set_style_text_color(brightnessIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(brightnessIcon, &lv_font_montserrat_20, 0);
    lv_obj_align(brightnessIcon, LV_ALIGN_LEFT_MID, 0, 0);

    // Brightness slider
    brightnessSlider = lv_slider_create(brightnessSection);
    lv_obj_set_width(brightnessSlider, sectionWidth - scaledX(60));
    lv_obj_set_height(brightnessSlider, scaledY(10));
    lv_obj_align(brightnessSlider, LV_ALIGN_LEFT_MID, scaledX(35), 0);
    lv_slider_set_range(brightnessSlider, 1, 100);
    lv_slider_set_value(brightnessSlider, (int)(getBrightnessFloat() * 100), LV_ANIM_OFF);

    // Slider styling
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(PANEL_SLIDER_TRACK_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_KNOB);
    lv_obj_set_style_pad_all(brightnessSlider, scaledY(3), LV_PART_KNOB);

    lv_obj_add_event_cb(brightnessSlider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, this);

    // Handle bar at bottom
    handleBar = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(handleBar);
    lv_obj_set_size(handleBar, scaledX(40), scaledY(4));
    lv_obj_align(handleBar, LV_ALIGN_BOTTOM_MID, 0, -scaledY(8));
    lv_obj_set_style_bg_color(handleBar, lv_color_hex(PANEL_HANDLE_COLOR), 0);
    lv_obj_set_style_bg_opa(handleBar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(handleBar, scaledY(2), 0);
    lv_obj_remove_flag(handleBar, LV_OBJ_FLAG_CLICKABLE);
}

void Statusbar::openPanel() {
    if (panelOpen || !pullDownPanel || !overlay) return;

    panelOpen = true;
    isClosing = false;

    const int screenHeight = getScreenHeight();

    // Sync brightness slider value
    if (brightnessSlider) {
        lv_slider_set_value(brightnessSlider, (int)(getBrightnessFloat() * 100), LV_ANIM_OFF);
    }

    // Show and animate overlay
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_to_index(overlay, -1);  // Bring to front

    lv_anim_t overlayAnim;
    lv_anim_init(&overlayAnim);
    lv_anim_set_var(&overlayAnim, overlay);
    lv_anim_set_values(&overlayAnim, LV_OPA_TRANSP, LV_OPA_50);
    lv_anim_set_time(&overlayAnim, 200);
    lv_anim_set_path_cb(&overlayAnim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&overlayAnim, [](void* obj, int32_t v) {
        lv_obj_set_style_bg_opa((lv_obj_t*)obj, v, 0);
    });
    lv_anim_start(&overlayAnim);

    // Show and animate panel
    lv_obj_move_to_index(pullDownPanel, -1);  // Bring to front

    lv_anim_t panelAnim;
    lv_anim_init(&panelAnim);
    lv_anim_set_var(&panelAnim, pullDownPanel);
    lv_anim_set_values(&panelAnim, -screenHeight, 0);
    lv_anim_set_time(&panelAnim, 200);
    lv_anim_set_path_cb(&panelAnim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&panelAnim, [](void* obj, int32_t v) {
        lv_obj_set_y((lv_obj_t*)obj, v);
    });
    lv_anim_start(&panelAnim);
}

// Animation complete callback for close
void statusbar_panel_close_anim_cb(lv_anim_t* a) {
    Statusbar* statusbar = (Statusbar*)lv_anim_get_user_data(a);
    if (statusbar && statusbar->overlay) {
        lv_obj_add_flag(statusbar->overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (statusbar) {
        statusbar->isClosing = false;
    }
}

void Statusbar::closePanel() {
    if (!panelOpen || !pullDownPanel || !overlay || isClosing) return;

    panelOpen = false;
    isClosing = true;

    const int screenHeight = getScreenHeight();

    // Animate overlay fade out
    lv_anim_t overlayAnim;
    lv_anim_init(&overlayAnim);
    lv_anim_set_var(&overlayAnim, overlay);
    lv_anim_set_values(&overlayAnim, LV_OPA_50, LV_OPA_TRANSP);
    lv_anim_set_time(&overlayAnim, 200);
    lv_anim_set_path_cb(&overlayAnim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&overlayAnim, [](void* obj, int32_t v) {
        lv_obj_set_style_bg_opa((lv_obj_t*)obj, v, 0);
    });
    lv_anim_set_user_data(&overlayAnim, this);
    lv_anim_set_completed_cb(&overlayAnim, statusbar_panel_close_anim_cb);
    lv_anim_start(&overlayAnim);

    // Animate panel slide up
    lv_anim_t panelAnim;
    lv_anim_init(&panelAnim);
    lv_anim_set_var(&panelAnim, pullDownPanel);
    lv_anim_set_values(&panelAnim, 0, -screenHeight);
    lv_anim_set_time(&panelAnim, 200);
    lv_anim_set_path_cb(&panelAnim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&panelAnim, [](void* obj, int32_t v) {
        lv_obj_set_y((lv_obj_t*)obj, v);
    });
    lv_anim_start(&panelAnim);
}

void Statusbar::togglePanel() {
    if (panelOpen) {
        closePanel();
    } else {
        openPanel();
    }
}

void Statusbar::update() {
    updateBatteryStatus();
    updateAlertStatus();
}

void Statusbar::updateBatteryStatus() {
    if (!batteryLabel || !batteryIcon) return;

    char* battStr = getBatteryVoltageString();
    if (!battStr) return;

    // Update top bar battery label (just percentage)
    lv_label_set_text(batteryLabel, battStr);

    // Update panel battery label with charging status
    if (panelBatteryLabel) {
        static char fullBattStr[48];
        if (isBatteryCharging()) {
            snprintf(fullBattStr, sizeof(fullBattStr), "Battery: %s (Charging)", battStr);
        } else {
            snprintf(fullBattStr, sizeof(fullBattStr), "Battery: %s", battStr);
        }
        lv_label_set_text(panelBatteryLabel, fullBattStr);
    }

    // Determine battery icon
    bool isCharging = isBatteryCharging();
    int percent = atoi(battStr);  // Parse percentage from "XX%"

    const char* icon;
    if (isCharging) {
        icon = LV_SYMBOL_CHARGE;
    } else if (percent > 75) {
        icon = LV_SYMBOL_BATTERY_FULL;
    } else if (percent > 50) {
        icon = LV_SYMBOL_BATTERY_3;
    } else if (percent > 25) {
        icon = LV_SYMBOL_BATTERY_2;
    } else if (percent > 10) {
        icon = LV_SYMBOL_BATTERY_1;
    } else {
        icon = LV_SYMBOL_BATTERY_EMPTY;
    }

    lv_label_set_text(batteryIcon, icon);
    if (panelBatteryIcon) {
        lv_label_set_text(panelBatteryIcon, icon);
    }
}

void Statusbar::updateAlertStatus() {
    if (!alertIcon) return;

    // Always show alert icon when there's an active alert (ignore dismissed state)
    bool hasAlert = globalAlertState.hasActiveAlert;

    if (hasAlert) {
        lv_obj_remove_flag(alertIcon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(alertIcon, globalAlertState.currentColor, 0);

        // Update panel alert section
        if (panelAlertLabel) {
            lv_label_set_text(panelAlertLabel, globalAlertState.currentMessage);
        }
        if (panelAlertIcon) {
            lv_obj_set_style_text_color(panelAlertIcon, globalAlertState.currentColor, 0);
        }
    } else {
        lv_obj_add_flag(alertIcon, LV_OBJ_FLAG_HIDDEN);

        if (panelAlertLabel) {
            lv_label_set_text(panelAlertLabel, "No alerts");
        }
        if (panelAlertIcon) {
            lv_obj_set_style_text_color(panelAlertIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        }
    }

    hasActiveAlert = hasAlert;
}

void Statusbar::show() {
    if (container) {
        lv_obj_remove_flag(container, LV_OBJ_FLAG_HIDDEN);
    }
    visible = true;
}

void Statusbar::hide() {
    if (container) {
        lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    }
    visible = false;
}

void Statusbar::cleanup() {
    container = nullptr;
    batteryIcon = nullptr;
    batteryLabel = nullptr;
    alertIcon = nullptr;
    overlay = nullptr;
    pullDownPanel = nullptr;
    batterySection = nullptr;
    alertSection = nullptr;
    brightnessSection = nullptr;
    brightnessSlider = nullptr;
    handleBar = nullptr;
    panelBatteryIcon = nullptr;
    panelBatteryLabel = nullptr;
    panelAlertIcon = nullptr;
    panelAlertLabel = nullptr;
    visible = false;
    panelOpen = false;
    hasActiveAlert = false;
    isClosing = false;
}

int Statusbar::getHeight() {
    return STATUSBAR_HEIGHT;
}

// Static callback functions
static void statusbar_click_cb(lv_event_t* e) {
    Statusbar* statusbar = (Statusbar*)lv_event_get_user_data(e);
    if (statusbar) {
        statusbar->togglePanel();
    }
}

static void statusbar_overlay_click_cb(lv_event_t* e) {
    Statusbar* statusbar = (Statusbar*)lv_event_get_user_data(e);
    if (statusbar) {
        statusbar->closePanel();
    }
}

static void brightness_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    set_brightness(val / 100.0f);
}
