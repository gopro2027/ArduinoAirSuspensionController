#include "option.h"
lv_style_t headerStyle;
static bool styleCreated = false;
LV_IMG_DECLARE(imgOn);
LV_IMG_DECLARE(imgOff);
#define OPTION_ROW_HEIGHT (36 * SCALE_Y)
#define MARGIN (10 * SCALE_X) // originally 16
static char strbuf[20];

void createStyle()
{
    if (styleCreated == false)
    {
        // create style
        lv_style_init(&headerStyle);
        // lv_style_set_bg_color(&headerStyle, lv_color_grey());
        // lv_style_set_bg_opa(&headerStyle, LV_OPA_50);
        // lv_style_set_border_width(&headerStyle, 2);
        // lv_style_set_border_color(&headerStyle, lv_color_black());
        lv_style_set_text_font(&headerStyle, &lv_font_montserrat_20);

        // scale per devices
        lv_style_set_transform_scale_x(&headerStyle, SCALE_X * 256);
        lv_style_set_transform_scale_y(&headerStyle, SCALE_Y * 256);

        styleCreated = true;
    }
}
void ui_clicked_imgOff(lv_event_t *e)
{
    // log_i("interact a");
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        option->setBooleanValue(true, true);
    }
}
void ui_clicked_imgOn(lv_event_t *e)
{
    // log_i("interact b");
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        option->setBooleanValue(false, true);
    }
}
void Option::indentText(int extraX)
{
    if (this->text != NULL)
    {
        lv_obj_set_x(this->text, MARGIN * 2 + extraX);
    }
    this->bar = lv_obj_create(this->root);
    lv_obj_remove_style_all(this->bar);
    lv_obj_set_style_bg_opa(this->bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(this->bar, 1, this->optionRowHeight);
    lv_obj_set_style_bg_color(this->bar, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->bar, MARGIN);
}
void ui_clicked_button(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    Option *option = (Option *)lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        option->event_cb(option);
    }
}
Option::Option(lv_obj_t *parent, OptionType type, const char *text, OptionValue value, option_event_cb_t _event_cb, void *_extraEventClickData)
{
    this->text = NULL;
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
    lv_obj_set_size(this->root, LCD_WIDTH, this->optionRowHeight);

    if (type != OptionType::SPACE && type != OptionType::BUTTON)
    {
        setupPressureLabel(this->root, &this->text, MARGIN, 0, LV_ALIGN_LEFT_MID, text);
    }
    if (type == OptionType::TEXT_WITH_VALUE)
    {
        this->indentText();
        setupPressureLabel(this->root, &this->rightHandObj, -MARGIN, 0, LV_ALIGN_RIGHT_MID, value.STRING);
    }
    else if (type == OptionType::HEADER)
    {
        lv_obj_add_style(this->text, &headerStyle, LV_PART_MAIN);
    }
    else if (type == OptionType::ON_OFF)
    {
        this->indentText();

        this->ui_imgOn = lv_image_create(this->root);
        lv_image_set_src(this->ui_imgOn, &imgOn);
        // lv_obj_set_x(this->ui_imgOn, DISPLAY_WIDTH - MARGIN - imgOn.header.w);
        lv_obj_set_align(this->ui_imgOn, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(this->ui_imgOn, -MARGIN);

        this->ui_imgOff = lv_image_create(this->root);
        lv_image_set_src(this->ui_imgOff, &imgOff);
        // lv_obj_set_x(this->ui_imgOff, DISPLAY_WIDTH - MARGIN - imgOff.header.w);
        lv_obj_set_align(this->ui_imgOff, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(this->ui_imgOff, -MARGIN);

        // lv_obj_set_click(this->ui_imgOn, true);
        lv_obj_add_flag(this->ui_imgOn, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->ui_imgOn, ui_clicked_imgOn, LV_EVENT_ALL, this);

        // lv_obj_set_click(this->ui_imgOff, true);
        lv_obj_add_flag(this->ui_imgOff, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->ui_imgOff, ui_clicked_imgOff, LV_EVENT_ALL, this);

        // set it to true so that when it sets to false it actually runs the code (hacky fix)
        this->boolValue = true;
        this->setBooleanValue(false);
    }
    else if (type == OptionType::BUTTON)
    {
        this->text = lv_button_create(this->root);

        lv_obj_t *btntext = lv_label_create(this->text);
        lv_label_set_text(btntext, text);

        lv_obj_set_style_bg_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);    // bg
        lv_obj_set_style_border_color(this->text, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN | LV_STATE_DEFAULT); // border

        // disabled colors
        lv_obj_set_style_bg_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DISABLED);     // bg
        lv_obj_set_style_border_color(this->text, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DISABLED); // border

        lv_obj_add_flag(this->text, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->text, ui_clicked_button, LV_EVENT_ALL, this);

        this->indentText();
    }
    else if (type == OptionType::RADIO)
    {
        this->indentText(MARGIN + 16); // lots of indent for the radio

        this->ui_imgOn = lv_checkbox_create(this->root);
        lv_checkbox_set_text(this->ui_imgOn, ""); // set blank because we render the text separately
        lv_obj_set_align(this->ui_imgOn, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(this->ui_imgOn, MARGIN * 2);

        lv_obj_set_style_bg_color(this->ui_imgOn, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(this->ui_imgOn, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(this->ui_imgOn, lv_color_hex(THEME_COLOR_DARK), LV_PART_INDICATOR | LV_STATE_DEFAULT);

        // only want the off image to be clickable
        lv_obj_add_flag(this->ui_imgOn, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->ui_imgOn, ui_clicked_button, LV_EVENT_ALL, this);
    }
    else if (type == OptionType::KEYBOARD_INPUT_NUMBER || type == OptionType::KEYBOARD_INPUT_TEXT)
    {
        this->indentText();
        const int textAreaWidth = (type == OptionType::KEYBOARD_INPUT_TEXT) ? 150 : 70;
        const int textMaxWidth = LCD_WIDTH - (MARGIN * 2 + MARGIN + textAreaWidth) - 6;
        lv_obj_set_width(this->text, textMaxWidth); // space between the start position and the text input

        this->rightHandObj = lv_textarea_create(this->root);
        // lv_obj_remove_style_all(this->rightHandObj);
        //  lv_cont_set_fit2(ta, LV_FIT_PARENT, LV_FIT_NONE);
        lv_textarea_set_text(this->rightHandObj, (type == OptionType::KEYBOARD_INPUT_TEXT) ? value.STRING : itoa(value.INT, strbuf, 10));
        lv_textarea_set_placeholder_text(this->rightHandObj, "Input");
        lv_textarea_set_one_line(this->rightHandObj, true);

        // lv_textarea_set_cursor_hidden(ta, true);
        // lv_obj_set_event_cb(this->rightHandObj, ta_event_handler);

        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);    // bg
        lv_obj_set_style_border_color(this->rightHandObj, lv_color_hex(THEME_COLOR_DARK), LV_PART_MAIN | LV_STATE_DEFAULT); // border

        lv_obj_set_style_radius(this->rightHandObj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
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
        // This one is a different height (2x) so it gets some weird calculations for placement
        this->indentText();

        lv_obj_set_align(this->text, LV_ALIGN_TOP_MID);
        lv_obj_set_y(this->text, OPTION_ROW_HEIGHT / 2);
        lv_obj_set_x(this->text, 0);

        this->rightHandObj = lv_slider_create(this->root);
        lv_slider_set_range(this->rightHandObj, 0, 9999999); // will be updated later
        lv_slider_set_value(this->rightHandObj, value.INT, LV_ANIM_OFF);

        lv_obj_set_width(this->rightHandObj, LCD_WIDTH - (MARGIN * 5));
        lv_obj_set_x(this->rightHandObj, MARGIN / 2);
        lv_obj_set_y(this->rightHandObj, -OPTION_ROW_HEIGHT / 4);
        lv_obj_set_align(this->rightHandObj, LV_ALIGN_BOTTOM_MID);

        // lv_obj_set_style_line_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | LV_STATE_DEFAULT); // border

        // lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);      // bg
        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_LIGHT), LV_PART_INDICATOR | LV_STATE_DEFAULT); // border
        lv_obj_set_style_bg_color(this->rightHandObj, lv_color_hex(THEME_COLOR_DARK), LV_PART_KNOB | LV_STATE_DEFAULT);       // border

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
        // lv_label_set_text_fmt(this->rightHandObj, "Compressor Frozen: %s", statusBittset & (1 << COMPRESSOR_FROZEN) ? "Yes" : "No");
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
                lv_obj_add_flag(this->ui_imgOff, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(this->ui_imgOn, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(this->ui_imgOn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(this->ui_imgOff, LV_OBJ_FLAG_HIDDEN);
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
            lv_obj_add_state(this->ui_imgOn, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_remove_state(this->ui_imgOn, LV_STATE_CHECKED);
        }
    }
}