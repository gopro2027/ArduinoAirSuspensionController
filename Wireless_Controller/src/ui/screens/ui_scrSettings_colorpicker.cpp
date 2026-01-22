#include "ui_scrSettings.h"
#include "../theme_colors.h"
#include "../../utils/util.h"

// Forward declaration
extern void reinitializeScreens();

struct ColorPickerData {
    lv_obj_t *overlay;
    lv_obj_t *previewCircle;
    lv_obj_t *hexLabel;
    lv_obj_t *redSlider;
    lv_obj_t *greenSlider;
    lv_obj_t *blueSlider;
    lv_obj_t *redValueLabel;
    lv_obj_t *greenValueLabel;
    lv_obj_t *blueValueLabel;
};

// Forward declarations
static void slider_changed_cb(lv_event_t *e);

// Update preview when sliders change
static void updatePreview(ColorPickerData *data) {
    uint8_t r = lv_slider_get_value(data->redSlider);
    uint8_t g = lv_slider_get_value(data->greenSlider);
    uint8_t b = lv_slider_get_value(data->blueSlider);

    // Update preview circle
    lv_obj_set_style_bg_color(data->previewCircle, lv_color_make(r, g, b), 0);

    // Update hex label
    lv_label_set_text_fmt(data->hexLabel, "#%02X%02X%02X", r, g, b);

    // Update value labels
    lv_label_set_text_fmt(data->redValueLabel, "%d", r);
    lv_label_set_text_fmt(data->greenValueLabel, "%d", g);
    lv_label_set_text_fmt(data->blueValueLabel, "%d", b);
}

static void slider_changed_cb(lv_event_t *e) {
    ColorPickerData *data = (ColorPickerData *)lv_event_get_user_data(e);
    updatePreview(data);
}

void ScrSettings::showColorPickerModal() {
    const int screenWidth = getScreenWidth();
    const int screenHeight = getScreenHeight();

    // Get current theme color
    uint32_t currentColor = getThemeColorLight();
    uint8_t currentR = (currentColor >> 16) & 0xFF;
    uint8_t currentG = (currentColor >> 8) & 0xFF;
    uint8_t currentB = currentColor & 0xFF;

    // Allocate persistent data
    ColorPickerData *data = new ColorPickerData();

    // Create full-screen overlay with dark background
    lv_obj_t *overlay = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);  // Block clicks from passing through
    lv_obj_move_foreground(overlay);  // Ensure it's on top

    // Add event handler to consume all clicks on overlay (prevents clicks from passing through)
    lv_obj_add_event_cb(overlay, [](lv_event_t *e) {
        // Stop all events from propagating to elements below
        lv_event_stop_bubbling(e);
    }, LV_EVENT_ALL, NULL);

    data->overlay = overlay;

    // Main container - SCROLLABLE for small screens
    lv_obj_t *container = lv_obj_create(overlay);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, screenWidth, screenHeight);
    lv_obj_set_style_pad_all(container, scaledX(20), 0);
    lv_obj_set_style_pad_row(container, scaledY(8), 0);
    lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);  // Block clicks from passing through
    // Make scrollable if content doesn't fit
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ========== Title ==========
    lv_obj_t *titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, "Pick Color");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_bottom(titleLabel, scaledY(8), 0);

    // ========== Preview Circle ==========
    const int previewSize = scaledX(70);
    data->previewCircle = lv_obj_create(container);
    lv_obj_remove_style_all(data->previewCircle);
    lv_obj_set_size(data->previewCircle, previewSize, previewSize);
    lv_obj_set_style_radius(data->previewCircle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(data->previewCircle, lv_color_make(currentR, currentG, currentB), 0);
    lv_obj_set_style_bg_opa(data->previewCircle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(data->previewCircle, scaledX(3), 0);
    lv_obj_set_style_border_color(data->previewCircle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(data->previewCircle, LV_OPA_30, 0);
    lv_obj_set_style_shadow_width(data->previewCircle, scaledX(20), 0);
    lv_obj_set_style_shadow_opa(data->previewCircle, LV_OPA_50, 0);
    lv_obj_set_style_shadow_color(data->previewCircle, lv_color_make(currentR, currentG, currentB), 0);

    // ========== Hex Code Label ==========
    data->hexLabel = lv_label_create(container);
    lv_label_set_text_fmt(data->hexLabel, "#%02X%02X%02X", currentR, currentG, currentB);
    lv_obj_set_style_text_font(data->hexLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(data->hexLabel, lv_color_hex(0xFFFFFF), 0);

    // ========== Sliders ==========
    lv_obj_t *slidersContainer = lv_obj_create(container);
    lv_obj_remove_style_all(slidersContainer);
    lv_obj_set_width(slidersContainer, LV_PCT(100));
    lv_obj_set_height(slidersContainer, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(slidersContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(slidersContainer, scaledY(16), 0);

    // Helper to create compact slider row
    auto createSlider = [&](const char* label, uint8_t initialValue, lv_color_t color,
                           lv_obj_t **sliderOut, lv_obj_t **valueLabelOut) {
        lv_obj_t *row = lv_obj_create(slidersContainer);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(row, scaledY(4), 0);
        lv_obj_set_style_pad_top(row, scaledY(4), 0);
        lv_obj_set_style_pad_bottom(row, scaledY(8), 0);  // Extra padding at bottom for knob
        lv_obj_set_style_clip_corner(row, false, 0);  // Don't clip rounded corners/overflow

        // Label and value on same line
        lv_obj_t *topRow = lv_obj_create(row);
        lv_obj_remove_style_all(topRow);
        lv_obj_set_size(topRow, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(topRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(topRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *nameLabel = lv_label_create(topRow);
        lv_label_set_text(nameLabel, label);
        lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(nameLabel, lv_color_hex(0x999999), 0);

        *valueLabelOut = lv_label_create(topRow);
        lv_label_set_text_fmt(*valueLabelOut, "%d", initialValue);
        lv_obj_set_style_text_font(*valueLabelOut, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(*valueLabelOut, lv_color_hex(0xFFFFFF), 0);

        // Slider
        *sliderOut = lv_slider_create(row);
        lv_slider_set_range(*sliderOut, 0, 255);
        lv_slider_set_value(*sliderOut, initialValue, LV_ANIM_OFF);
        lv_obj_set_width(*sliderOut, LV_PCT(100));
        lv_obj_set_height(*sliderOut, scaledY(20));

        // Main track styling
        lv_obj_set_style_bg_color(*sliderOut, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(*sliderOut, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(*sliderOut, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(*sliderOut, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(*sliderOut, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(*sliderOut, 0, LV_PART_MAIN);

        // Indicator (filled part)
        lv_obj_set_style_bg_color(*sliderOut, color, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(*sliderOut, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(*sliderOut, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

        // Knob (draggable circle)
        lv_obj_set_style_bg_color(*sliderOut, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
        lv_obj_set_style_radius(*sliderOut, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        lv_obj_set_style_pad_all(*sliderOut, scaledX(6), LV_PART_KNOB);
        lv_obj_set_style_border_width(*sliderOut, 0, LV_PART_KNOB);
        lv_obj_set_style_outline_width(*sliderOut, 0, LV_PART_KNOB);
        lv_obj_set_style_shadow_width(*sliderOut, scaledX(8), LV_PART_KNOB);
        lv_obj_set_style_shadow_opa(*sliderOut, LV_OPA_30, LV_PART_KNOB);
        lv_obj_set_style_shadow_color(*sliderOut, lv_color_hex(0x000000), LV_PART_KNOB);
    };

    createSlider("Red", currentR, lv_color_hex(0xEF4444), &data->redSlider, &data->redValueLabel);
    createSlider("Green", currentG, lv_color_hex(0x10B981), &data->greenSlider, &data->greenValueLabel);
    createSlider("Blue", currentB, lv_color_hex(0x3B82F6), &data->blueSlider, &data->blueValueLabel);

    lv_obj_add_event_cb(data->redSlider, slider_changed_cb, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_add_event_cb(data->greenSlider, slider_changed_cb, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_add_event_cb(data->blueSlider, slider_changed_cb, LV_EVENT_VALUE_CHANGED, data);

    // ========== Buttons ==========
    lv_obj_t *btnContainer = lv_obj_create(container);
    lv_obj_remove_style_all(btnContainer);
    lv_obj_set_size(btnContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btnContainer, scaledX(12), 0);
    lv_obj_set_style_pad_top(btnContainer, scaledY(16), 0);

    const int btnWidth = scaledX(90);
    const int btnHeight = scaledY(36);

    // Cancel button
    lv_obj_t *cancelBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(cancelBtn, btnWidth, btnHeight);
    lv_obj_set_style_bg_color(cancelBtn, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_radius(cancelBtn, scaledX(18), 0);
    lv_obj_set_style_border_width(cancelBtn, 0, 0);

    lv_obj_t *cancelLabel = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_set_style_text_font(cancelLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(cancelLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_center(cancelLabel);

    lv_obj_add_event_cb(cancelBtn, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            ColorPickerData *data = (ColorPickerData *)lv_event_get_user_data(e);
            lv_obj_delete(data->overlay);
            delete data;
        }
        // Stop all events from propagating through
        lv_event_stop_bubbling(e);
    }, LV_EVENT_ALL, data);

    // Apply button
    lv_obj_t *applyBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(applyBtn, btnWidth, btnHeight);
    lv_obj_set_style_bg_color(applyBtn, lv_color_hex(getThemeColorLight()), 0);
    lv_obj_set_style_radius(applyBtn, scaledX(18), 0);
    lv_obj_set_style_border_width(applyBtn, 0, 0);
    lv_obj_set_style_shadow_color(applyBtn, lv_color_hex(getThemeColorLight()), 0);
    lv_obj_set_style_shadow_width(applyBtn, scaledX(12), 0);
    lv_obj_set_style_shadow_opa(applyBtn, LV_OPA_40, 0);

    lv_obj_t *applyLabel = lv_label_create(applyBtn);
    lv_label_set_text(applyLabel, "Apply");
    lv_obj_set_style_text_font(applyLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(applyLabel, lv_color_hex(0x000000), 0);
    lv_obj_center(applyLabel);

    lv_obj_add_event_cb(applyBtn, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            ColorPickerData *data = (ColorPickerData *)lv_event_get_user_data(e);

            uint8_t r = lv_slider_get_value(data->redSlider);
            uint8_t g = lv_slider_get_value(data->greenSlider);
            uint8_t b = lv_slider_get_value(data->blueSlider);

            uint32_t hex = (r << 16) | (g << 8) | b;
            setThemeColorLight(hex);
            setThemeColorDark(((r / 2) << 16) | ((g / 2) << 8) | (b / 2));
            setThemeColorMedium(((r * 3 / 4) << 16) | ((g * 3 / 4) << 8) | (b * 3 / 4));

            if (scrSettings.ui_themePreset) {
                scrSettings.ui_themePreset->setSelectedOption(-1, false);
            }

            lv_obj_delete(data->overlay);
            delete data;
            runNextFrame([]() { reinitializeScreens(); });
        }
        // Stop all events from propagating through
        lv_event_stop_bubbling(e);
    }, LV_EVENT_ALL, data);
}
