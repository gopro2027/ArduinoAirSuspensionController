#include "statusbar.h"
#include "utils/util.h"
#include "alert.h"
#include "waveshare/waveshare.h"
#include "waveshare/board_driver_util.h"
#include <BTOas.h>

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
static void panel_gesture_cb(lv_event_t* e);

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
    statusSection = nullptr;
    compressorFrozenLabel = nullptr;
    accStatusLabel = nullptr;
    ebrakeStatusLabel = nullptr;
    compressorStatusLabel = nullptr;
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
    const int sectionRadius = scaledY(12);
    const int sectionPadding = scaledX(12);
    const int sectionGap = scaledY(8);

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
    lv_obj_set_style_border_width(pullDownPanel, 0, 0);
    lv_obj_set_style_pad_top(pullDownPanel, scaledY(20), 0);
    lv_obj_set_style_pad_bottom(pullDownPanel, scaledY(60), 0);  // Extra space for handle area
    lv_obj_set_style_pad_left(pullDownPanel, scaledX(12), 0);
    lv_obj_set_style_pad_right(pullDownPanel, scaledX(12), 0);
    lv_obj_set_style_pad_row(pullDownPanel, sectionGap, 0);
    lv_obj_set_flex_flow(pullDownPanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pullDownPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(pullDownPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(pullDownPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(pullDownPanel, LV_SCROLLBAR_MODE_AUTO);

    // Helper lambda to create a status row with icon and text
    auto createStatusRow = [&](lv_obj_t* parent, const char* icon, const char* text) -> lv_obj_t* {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, scaledX(8), 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* iconLabel = lv_label_create(row);
        lv_label_set_text(iconLabel, icon);
        lv_obj_set_style_text_color(iconLabel, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        lv_obj_set_style_text_font(iconLabel, &lv_font_montserrat_10, 0);

        lv_obj_t* textLabel = lv_label_create(row);
        lv_label_set_text(textLabel, text);
        lv_obj_set_style_text_color(textLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
        lv_obj_set_style_text_font(textLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_flex_grow(textLabel, 1);

        return textLabel;
    };

    // ========== BATTERY SECTION ==========
    batterySection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(batterySection);
    lv_obj_set_size(batterySection, sectionWidth, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(batterySection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(batterySection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(batterySection, sectionRadius, 0);
    lv_obj_set_style_pad_all(batterySection, sectionPadding, 0);
    lv_obj_set_flex_flow(batterySection, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(batterySection, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(batterySection, scaledX(12), 0);
    lv_obj_remove_flag(batterySection, LV_OBJ_FLAG_SCROLLABLE);

    panelBatteryIcon = lv_label_create(batterySection);
    lv_label_set_text(panelBatteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(panelBatteryIcon, lv_color_hex(0x4ADE80), 0);
    lv_obj_set_style_text_font(panelBatteryIcon, &lv_font_montserrat_20, 0);

    panelBatteryLabel = lv_label_create(batterySection);
    lv_label_set_text(panelBatteryLabel, "Battery: 100%");
    lv_obj_set_style_text_color(panelBatteryLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(panelBatteryLabel, &lv_font_montserrat_10, 0);

    // ========== ALERT SECTION ==========
    alertSection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(alertSection);
    lv_obj_set_size(alertSection, sectionWidth, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(alertSection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(alertSection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(alertSection, sectionRadius, 0);
    lv_obj_set_style_pad_all(alertSection, sectionPadding, 0);
    lv_obj_set_flex_flow(alertSection, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(alertSection, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(alertSection, scaledX(12), 0);
    lv_obj_remove_flag(alertSection, LV_OBJ_FLAG_SCROLLABLE);

    panelAlertIcon = lv_label_create(alertSection);
    lv_label_set_text(panelAlertIcon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(panelAlertIcon, lv_color_hex(STATUSBAR_ICON_COLOR), 0);
    lv_obj_set_style_text_font(panelAlertIcon, &lv_font_montserrat_16, 0);

    panelAlertLabel = lv_label_create(alertSection);
    lv_label_set_text(panelAlertLabel, "No alerts");
    lv_obj_set_style_text_color(panelAlertLabel, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(panelAlertLabel, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(panelAlertLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(panelAlertLabel, 1);

    // ========== BRIGHTNESS SECTION ==========
    brightnessSection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(brightnessSection);
    lv_obj_set_size(brightnessSection, sectionWidth, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(brightnessSection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(brightnessSection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(brightnessSection, sectionRadius, 0);
    lv_obj_set_style_pad_all(brightnessSection, sectionPadding, 0);
    lv_obj_set_flex_flow(brightnessSection, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(brightnessSection, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(brightnessSection, scaledX(12), 0);
    lv_obj_remove_flag(brightnessSection, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* brightnessIcon = lv_label_create(brightnessSection);
    lv_label_set_text(brightnessIcon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_color(brightnessIcon, lv_color_hex(0xFBBF24), 0);
    lv_obj_set_style_text_font(brightnessIcon, &lv_font_montserrat_20, 0);

    brightnessSlider = lv_slider_create(brightnessSection);
    lv_obj_set_flex_grow(brightnessSlider, 1);
    lv_obj_set_height(brightnessSlider, scaledY(8));
    lv_slider_set_range(brightnessSlider, 1, 100);
    lv_slider_set_value(brightnessSlider, (int)(getBrightnessFloat() * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(PANEL_SLIDER_TRACK_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_KNOB);
    lv_obj_set_style_pad_all(brightnessSlider, scaledY(4), LV_PART_KNOB);
    lv_obj_set_style_radius(brightnessSlider, scaledY(4), LV_PART_MAIN);
    lv_obj_set_style_radius(brightnessSlider, scaledY(4), LV_PART_INDICATOR);
    lv_obj_add_event_cb(brightnessSlider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, this);

    // ========== STATUS SECTION ==========
    statusSection = lv_obj_create(pullDownPanel);
    lv_obj_remove_style_all(statusSection);
    lv_obj_set_size(statusSection, sectionWidth, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(statusSection, lv_color_hex(PANEL_SECTION_COLOR), 0);
    lv_obj_set_style_bg_opa(statusSection, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(statusSection, sectionRadius, 0);
    lv_obj_set_style_pad_all(statusSection, sectionPadding, 0);
    lv_obj_set_style_pad_row(statusSection, scaledY(6), 0);
    lv_obj_set_flex_flow(statusSection, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(statusSection, LV_OBJ_FLAG_SCROLLABLE);

    // Section title
    lv_obj_t* statusTitle = lv_label_create(statusSection);
    lv_label_set_text(statusTitle, "System Status");
    lv_obj_set_style_text_color(statusTitle, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(statusTitle, &lv_font_montserrat_10, 0);
    lv_obj_set_style_pad_bottom(statusTitle, scaledY(4), 0);

    // Status rows with icons
    compressorFrozenLabel = createStatusRow(statusSection, LV_SYMBOL_WARNING, "Compressor Frozen: --");
    accStatusLabel = createStatusRow(statusSection, LV_SYMBOL_POWER, "ACC: --");
    ebrakeStatusLabel = createStatusRow(statusSection, LV_SYMBOL_STOP, "E-Brake: --");
    compressorStatusLabel = createStatusRow(statusSection, LV_SYMBOL_REFRESH, "Compressor: --");

    // ========== HANDLE BAR AREA (for swipe gesture) ==========
    // Create handle area as sibling to panel (on parent), positioned at bottom when panel is open
    lv_obj_t* handleArea = lv_obj_create(parent);
    lv_obj_remove_style_all(handleArea);
    lv_obj_set_size(handleArea, screenWidth, scaledY(50));
    lv_obj_set_pos(handleArea, 0, -scaledY(50));  // Start off-screen (will animate with panel)
    lv_obj_set_style_bg_opa(handleArea, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(handleArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(handleArea, LV_OBJ_FLAG_HIDDEN);  // Hidden initially
    lv_obj_add_event_cb(handleArea, panel_gesture_cb, LV_EVENT_GESTURE, this);
    lv_obj_add_event_cb(handleArea, panel_gesture_cb, LV_EVENT_CLICKED, this);
    lv_obj_remove_flag(handleArea, LV_OBJ_FLAG_SCROLLABLE);

    // Store handleArea pointer for animation
    this->handleBar = handleArea;  // Repurpose handleBar to store the touch area

    // Visual handle bar inside the touch area
    lv_obj_t* handleVisual = lv_obj_create(handleArea);
    lv_obj_remove_style_all(handleVisual);
    lv_obj_set_size(handleVisual, scaledX(50), scaledY(4));
    lv_obj_align(handleVisual, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(handleVisual, lv_color_hex(PANEL_HANDLE_COLOR), 0);
    lv_obj_set_style_bg_opa(handleVisual, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(handleVisual, scaledY(2), 0);
    lv_obj_remove_flag(handleVisual, LV_OBJ_FLAG_CLICKABLE);
}

void Statusbar::openPanel() {
    if (panelOpen || !pullDownPanel || !overlay) return;

    panelOpen = true;
    isClosing = false;

    const int screenHeight = getScreenHeight();
    const int handleHeight = scaledY(50);

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

    // Show and animate handle bar area (slides in from top to bottom of screen)
    if (handleBar) {
        lv_obj_remove_flag(handleBar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_to_index(handleBar, -1);  // Bring to front

        lv_anim_t handleAnim;
        lv_anim_init(&handleAnim);
        lv_anim_set_var(&handleAnim, handleBar);
        lv_anim_set_values(&handleAnim, -handleHeight, screenHeight - handleHeight);
        lv_anim_set_time(&handleAnim, 200);
        lv_anim_set_path_cb(&handleAnim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&handleAnim, [](void* obj, int32_t v) {
            lv_obj_set_y((lv_obj_t*)obj, v);
        });
        lv_anim_start(&handleAnim);
    }
}

// Animation complete callback for close
void statusbar_panel_close_anim_cb(lv_anim_t* a) {
    Statusbar* statusbar = (Statusbar*)lv_anim_get_user_data(a);
    if (statusbar && statusbar->overlay) {
        lv_obj_add_flag(statusbar->overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (statusbar && statusbar->handleBar) {
        lv_obj_add_flag(statusbar->handleBar, LV_OBJ_FLAG_HIDDEN);
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
    const int handleHeight = scaledY(50);

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

    // Animate handle bar area slide up
    if (handleBar) {
        lv_anim_t handleAnim;
        lv_anim_init(&handleAnim);
        lv_anim_set_var(&handleAnim, handleBar);
        lv_anim_set_values(&handleAnim, screenHeight - handleHeight, -handleHeight);
        lv_anim_set_time(&handleAnim, 200);
        lv_anim_set_path_cb(&handleAnim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&handleAnim, [](void* obj, int32_t v) {
            lv_obj_set_y((lv_obj_t*)obj, v);
        });
        lv_anim_start(&handleAnim);
    }
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
    updateStatusSection();
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

    // Determine battery color based on level
    uint32_t battColor;
    if (isCharging) {
        battColor = 0x60A5FA;  // Blue when charging
    } else if (percent > 50) {
        battColor = 0x4ADE80;  // Green for good
    } else if (percent > 20) {
        battColor = 0xFBBF24;  // Yellow for medium
    } else {
        battColor = 0xFF6B6B;  // Red for low
    }

    lv_label_set_text(batteryIcon, icon);
    lv_obj_set_style_text_color(batteryIcon, lv_color_hex(battColor), 0);

    if (panelBatteryIcon) {
        lv_label_set_text(panelBatteryIcon, icon);
        lv_obj_set_style_text_color(panelBatteryIcon, lv_color_hex(battColor), 0);
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

void Statusbar::updateStatusSection() {
    if (!statusSection) return;

    // Compressor Frozen
    if (compressorFrozenLabel) {
        bool frozen = statusBittset & (1 << StatusPacketBittset::COMPRESSOR_FROZEN);
        lv_label_set_text(compressorFrozenLabel, frozen ? "Frozen: Yes" : "Frozen: No");
        lv_obj_t* row = lv_obj_get_parent(compressorFrozenLabel);
        lv_obj_t* icon = lv_obj_get_child(row, 0);
        if (icon) {
            lv_obj_set_style_text_color(icon, frozen ? lv_color_hex(0xFF6B6B) : lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        }
        lv_obj_set_style_text_color(compressorFrozenLabel,
            frozen ? lv_color_hex(0xFF6B6B) : lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    }

    // ACC Status
    if (accStatusLabel) {
        bool accOn = statusBittset & (1 << StatusPacketBittset::ACC_STATUS_ON);
        lv_label_set_text(accStatusLabel, accOn ? "ACC: On" : "ACC: Off");
        lv_obj_t* row = lv_obj_get_parent(accStatusLabel);
        lv_obj_t* icon = lv_obj_get_child(row, 0);
        if (icon) {
            lv_obj_set_style_text_color(icon, accOn ? lv_color_hex(0x4ADE80) : lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        }
        lv_obj_set_style_text_color(accStatusLabel,
            accOn ? lv_color_hex(0x4ADE80) : lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    }

    // E-Brake Status
    if (ebrakeStatusLabel) {
        bool ebrakeOn = statusBittset & (1 << StatusPacketBittset::EBRAKE_STATUS_ON);
        lv_label_set_text(ebrakeStatusLabel, ebrakeOn ? "E-Brake: On" : "E-Brake: Off");
        lv_obj_t* row = lv_obj_get_parent(ebrakeStatusLabel);
        lv_obj_t* icon = lv_obj_get_child(row, 0);
        if (icon) {
            lv_obj_set_style_text_color(icon, ebrakeOn ? lv_color_hex(0xFBBF24) : lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        }
        lv_obj_set_style_text_color(ebrakeStatusLabel,
            ebrakeOn ? lv_color_hex(0xFBBF24) : lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    }

    // Compressor Status
    if (compressorStatusLabel) {
        bool compressorOn = statusBittset & (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON);
        lv_label_set_text(compressorStatusLabel, compressorOn ? "Compressor: Running" : "Compressor: Off");
        lv_obj_t* row = lv_obj_get_parent(compressorStatusLabel);
        lv_obj_t* icon = lv_obj_get_child(row, 0);
        if (icon) {
            lv_obj_set_style_text_color(icon, compressorOn ? lv_color_hex(0x4ADE80) : lv_color_hex(STATUSBAR_ICON_COLOR), 0);
        }
        lv_obj_set_style_text_color(compressorStatusLabel,
            compressorOn ? lv_color_hex(0x4ADE80) : lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    }
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
    statusSection = nullptr;
    compressorFrozenLabel = nullptr;
    accStatusLabel = nullptr;
    ebrakeStatusLabel = nullptr;
    compressorStatusLabel = nullptr;
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

static void panel_gesture_cb(lv_event_t* e) {
    Statusbar* statusbar = (Statusbar*)lv_event_get_user_data(e);
    if (!statusbar) return;

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        // Swipe up to close the panel
        if (dir == LV_DIR_TOP) {
            statusbar->closePanel();
        }
    } else if (code == LV_EVENT_CLICKED) {
        // Tap on handle area also closes panel
        statusbar->closePanel();
    }
}
