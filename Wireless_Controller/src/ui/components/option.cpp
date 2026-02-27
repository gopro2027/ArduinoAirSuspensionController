#include "option.h"
lv_style_t headerStyle;
static bool styleCreated = false;


// Use a minimum height to ensure usability
static int getOptionRowHeight() {
    int scaled = (int)(36 * getScaleY());
    return (scaled < 36) ? 36 : scaled;  // Minimum 36px height
}
#define OPTION_ROW_HEIGHT getOptionRowHeight()
// Dynamic margin that scales with display
static int getMargin() {
    return scaledX(10);
}
#define MARGIN getMargin()
static char strbuf[20];

void createStyle()
{
    if (styleCreated == false)
    {
        // create style
        lv_style_init(&headerStyle);
        lv_style_set_text_font(&headerStyle, &lv_font_montserrat_20);
        // Don't use transform scaling - it causes issues with rotation changes
        styleCreated = true;
    }
}

void Option::resetHeaderStyle()
{
    // Force style to be recreated on next Option creation
    styleCreated = false;
}
void ui_switch_changed(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
        bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
        option->setBooleanValue(checked, true);
    }
}
void Option::indentText(int multiplier)
{
    if (this->text != NULL)
    {
        lv_obj_set_x(this->text, MARGIN * multiplier);
    }
}
void ui_clicked_button(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        option->event_cb(option);
    }
}
Option::Option(lv_obj_t *parent, OptionType type, const char *text, OptionValue value, option_event_cb_t _event_cb, void *_extraEventClickData)
{
    this->text = NULL;
    this->root = NULL;
    this->rightHandObj = NULL;
    this->ui_switch = NULL;
    this->ui_slider_value_text = NULL;
    this->event_cb = NULL;
    this->extraEventClickData = _extraEventClickData;
    this->type = type;
    this->optionRowHeight = OPTION_ROW_HEIGHT;

    // Some types need to be larger in height
    if (this->type == OptionType::SLIDER)
    {
        this->optionRowHeight = OPTION_ROW_HEIGHT * 2;
    }

    createStyle();
    this->root = lv_obj_create(parent);
    lv_obj_remove_style_all(this->root);
    lv_obj_set_size(this->root, getScreenWidth(), this->optionRowHeight);

    if (type != OptionType::SPACE && type != OptionType::BUTTON)
    {
        setupPressureLabel(this->root, &this->text, MARGIN, 0, LV_ALIGN_LEFT_MID, text);
    }
    if (type == OptionType::TEXT_WITH_VALUE)
    {
        setupPressureLabel(this->root, &this->rightHandObj, -MARGIN, 0, LV_ALIGN_RIGHT_MID, value.STRING);
    }
    else if (type == OptionType::HEADER)
    {
        lv_obj_add_style(this->text, &headerStyle, LV_PART_MAIN);
    }
    else if (type == OptionType::ON_OFF)
    {
        this->ui_switch = lv_switch_create(this->root);
        lv_obj_set_size(this->ui_switch, scaledX(40), scaledY(22));
        lv_obj_set_align(this->ui_switch, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(this->ui_switch, -MARGIN);

        // Style the switch - off state (background)
        lv_obj_set_style_bg_color(this->ui_switch, lv_color_hex(GENERIC_GREY_DARK), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(this->ui_switch, LV_OPA_COVER, LV_PART_MAIN);

        // Style the switch - on state (indicator when checked)
        lv_obj_set_style_bg_color(this->ui_switch, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | (lv_style_selector_t)LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(this->ui_switch, LV_OPA_COVER, LV_PART_INDICATOR | (lv_style_selector_t)LV_STATE_CHECKED);

        // Style the knob
        lv_obj_set_style_bg_color(this->ui_switch, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
        lv_obj_set_style_bg_opa(this->ui_switch, LV_OPA_COVER, LV_PART_KNOB);

        lv_obj_add_event_cb(this->ui_switch, ui_switch_changed, LV_EVENT_VALUE_CHANGED, this);

        // Initialize switch state from passed value
        this->boolValue = (value.INT != 0);
        if (this->boolValue) {
            lv_obj_add_state(this->ui_switch, LV_STATE_CHECKED);
        }
    }
    else if (type == OptionType::BUTTON)
    {
        this->text = lv_button_create(this->root);

        lv_obj_t *btntext = lv_label_create(this->text);
        lv_label_set_text(btntext, text);

        lv_obj_set_style_bg_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN);    // bg
        lv_obj_set_style_border_color(this->text, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN); // border

        // disabled colors
        lv_obj_set_style_bg_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED);     // bg
        lv_obj_set_style_border_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED); // border

        lv_obj_add_flag(this->text, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->text, ui_clicked_button, LV_EVENT_ALL, this);

        this->indentText(1);
    }
    else if (type == OptionType::RADIO)
    {
        this->indentText(4); // lots of indent for the radio

        this->ui_switch = lv_checkbox_create(this->root);
        lv_checkbox_set_text(this->ui_switch, ""); // set blank because we render the text separately
        lv_obj_set_align(this->ui_switch, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(this->ui_switch, MARGIN);

        lv_obj_set_style_bg_color(this->ui_switch, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | (lv_style_selector_t)LV_STATE_CHECKED);
        lv_obj_set_style_border_color(this->ui_switch, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(this->ui_switch, lv_color_hex(THEME_COLOR_DARK), LV_PART_INDICATOR);

        // Make checkbox non-clickable so touches pass through to root
        lv_obj_remove_flag(this->ui_switch, LV_OBJ_FLAG_CLICKABLE);

        // Make text label non-clickable so touches pass through to root
        if (this->text != NULL) {
            lv_obj_remove_flag(this->text, LV_OBJ_FLAG_CLICKABLE);
        }

        // Make the entire root container clickable for larger touch area
        lv_obj_add_flag(this->root, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(this->root, ui_clicked_button, LV_EVENT_CLICKED, this);
    }
    else if (type == OptionType::KEYBOARD_INPUT_NUMBER || type == OptionType::KEYBOARD_INPUT_TEXT)
    {
        this->indentText(1);
        const int textAreaWidth = (type == OptionType::KEYBOARD_INPUT_TEXT) ? scaledX(150) : scaledX(70);
        const int textMaxWidth = getScreenWidth() - (MARGIN * 2 + MARGIN + textAreaWidth) - scaledX(6);
        lv_obj_set_width(this->text, textMaxWidth); // space between the start position and the text input

        this->rightHandObj = lv_textarea_create(this->root);
        lv_textarea_set_text(this->rightHandObj, (type == OptionType::KEYBOARD_INPUT_TEXT) ? value.STRING : itoa(value.INT, strbuf, 10));
        lv_textarea_set_placeholder_text(this->rightHandObj, "Input");
        lv_textarea_set_one_line(this->rightHandObj, true);

        // lv_textarea_set_cursor_hidden(ta, true);
        // lv_obj_set_event_cb(this->rightHandObj, ta_event_handler);

        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN);    // bg
        lv_obj_set_style_border_color(this->rightHandObj, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN); // border
        lv_obj_set_style_text_color(this->rightHandObj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);           // text
        lv_obj_set_style_text_color(this->rightHandObj, lv_color_hex(0xE0E0E0), LV_PART_TEXTAREA_PLACEHOLDER); // placeholder

        lv_obj_set_style_radius(this->rightHandObj, 5, LV_PART_MAIN);
        // lv_obj_set_style_border_width(this->rightHandObj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_width(this->rightHandObj, textAreaWidth);
        lv_obj_set_x(this->rightHandObj, -MARGIN);
        lv_obj_set_y(this->rightHandObj, 0);
        lv_obj_set_align(this->rightHandObj, LV_ALIGN_RIGHT_MID);

        lv_obj_add_flag(this->rightHandObj, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->rightHandObj, ta_event_cb, LV_EVENT_ALL, this);
    }
    else if (type == OptionType::SLIDER)
    {
        lv_obj_set_align(this->text, LV_ALIGN_TOP_MID);
        lv_obj_set_y(this->text, OPTION_ROW_HEIGHT / 2);
        lv_obj_set_x(this->text, 0);

        this->rightHandObj = lv_slider_create(this->root);
        lv_slider_set_range(this->rightHandObj, 0, 9999999);
        lv_slider_set_value(this->rightHandObj, value.INT, LV_ANIM_OFF);

        lv_obj_set_width(this->rightHandObj, getScreenWidth() - (MARGIN * 4));
        lv_obj_set_x(this->rightHandObj, 0);
        lv_obj_set_y(this->rightHandObj, -OPTION_ROW_HEIGHT / 4);
        lv_obj_set_align(this->rightHandObj, LV_ALIGN_BOTTOM_MID);

        // lv_obj_set_style_line_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | LV_STATE_DEFAULT); // border

        // lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN);      // bg
        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR); // border
        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_DARK), LV_PART_KNOB);       // border

        lv_obj_add_flag(this->rightHandObj, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->rightHandObj, slider_event_cb, LV_EVENT_ALL, this);

        setupPressureLabel(this->root, &this->ui_slider_value_text, 0, 5, LV_ALIGN_CENTER, itoa(value.INT, strbuf, 10));
        lv_obj_set_style_text_color(this->ui_slider_value_text, lv_color_hex3(0x888), 0);
    }

    if (_event_cb != NULL)
    {
        this->event_cb = _event_cb;
    }
}

Option::~Option()
{
    // Option owns the LVGL widget tree rooted at `root`.
    // Deleting `root` also deletes all children (labels, switches, sliders, etc.).
    // This prevents LVGL from later firing events with `user_data == this`.
    if (this->root != NULL)
    {
        lv_obj_del(this->root); // Still here because if we happen to decide to delete an option by ittself this is needed. Otherwise, whenever we delete the screen this is actually already done automatically, so it currently has no effect.
        this->root = NULL;
    }

    // Clear other pointers for safety (not strictly required).
    this->text = NULL;
    this->rightHandObj = NULL;
    this->ui_switch = NULL;
    this->ui_slider_value_text = NULL;
    this->event_cb = NULL;
    this->extraEventClickData = NULL;
}

void Option::setSliderParams(int min, int max, bool display_above_value, lv_event_code_t trigger_event)
{
    if (this->type == OptionType::SLIDER)
    {
        this->slider_min = min;
        this->slider_max = max;
        this->slider_display_value_above = display_above_value;
        this->slider_trigger_event = trigger_event;
        lv_slider_set_range(this->rightHandObj, slider_min, slider_max);
        if (this->slider_display_value_above)
        {
            lv_obj_clear_flag(this->ui_slider_value_text, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(this->ui_slider_value_text, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void Option::setRightHandText(const char *text)
{
    if (this->type == OptionType::TEXT_WITH_VALUE)
    {
        if (strcmp(lv_label_get_text(this->rightHandObj), text) != 0)
        {
            lv_label_set_text(this->rightHandObj, text);
        }
    }
    else if (type == OptionType::KEYBOARD_INPUT_NUMBER || type == OptionType::KEYBOARD_INPUT_TEXT)
    {
        if (strcmp(lv_textarea_get_text(this->rightHandObj), text) != 0)
        {
            lv_textarea_set_text(this->rightHandObj, text); // itoa(value.INT, strbuf, 10)
        }
    }
    else if (type == OptionType::BUTTON)
    {
        lv_obj_t *label = lv_obj_get_child(this->text, 0);
        if (strcmp(lv_label_get_text(label), text) != 0)
        {
            lv_label_set_text(label, text);
        }
    }
    else if (this->type == OptionType::SLIDER)
    {
        lv_slider_set_value(this->rightHandObj, atoi(text), LV_ANIM_ON);
        if (this->slider_display_value_above)
        {
            if (strcmp(lv_label_get_text(this->ui_slider_value_text), text) != 0)
            {
                lv_label_set_text(this->ui_slider_value_text, text);
            }
        }
    }
}

void Option::setBooleanValue(bool value, bool netSend)
{
    if (this->type == OptionType::ON_OFF)
    {
        if (value != this->boolValue)
        {
            this->boolValue = value;
            if (value)
            {
                lv_obj_add_state(this->ui_switch, LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_remove_state(this->ui_switch, LV_STATE_CHECKED);
            }
            if (netSend && this->event_cb != NULL)
            {
                this->event_cb((void *)value);
            }
        }
    }
    else if (this->type == OptionType::RADIO)
    {
        if (value)
        {
            lv_obj_add_state(this->ui_switch, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_remove_state(this->ui_switch, LV_STATE_CHECKED);
        }
    }
}
