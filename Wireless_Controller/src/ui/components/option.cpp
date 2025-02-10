#include "option.h"
lv_style_t stylepointer;
static bool styleCreated = false;
LV_IMG_DECLARE(imgOn);
LV_IMG_DECLARE(imgOff);
#define OPTION_ROW_HEIGHT 36
void createStyle()
{
    if (styleCreated == false)
    {
        // create style
        lv_style_init(&stylepointer);
        // lv_style_set_bg_color(&stylepointer, lv_color_grey());
        // lv_style_set_bg_opa(&stylepointer, LV_OPA_50);
        // lv_style_set_border_width(&stylepointer, 2);
        // lv_style_set_border_color(&stylepointer, lv_color_black());
        lv_style_set_text_font(&stylepointer, &lv_font_montserrat_26);

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
void Option::indentText()
{
    lv_obj_set_x(this->text, 32);
    this->bar = lv_obj_create(this->root);
    lv_obj_remove_style_all(this->bar);
    lv_obj_set_style_bg_opa(this->bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(this->bar, 1, OPTION_ROW_HEIGHT);
    lv_obj_set_style_bg_color(this->bar, lv_color_hex(0xBB86FC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_x(this->bar, 16);
}
Option::Option(lv_obj_t *parent, OptionType type, const char *text, OptionValue value, option_event_cb_t _event_cb)
{
    this->event_cb = NULL;
    this->type = type;
    createStyle();
    this->root = lv_obj_create(parent);
    lv_obj_remove_style_all(this->root);
    lv_obj_set_size(this->root, DISPLAY_WIDTH, OPTION_ROW_HEIGHT);

    if (type != OptionType::SPACE)
    {
        setupPressureLabel(this->root, &this->text, 16, 0, LV_ALIGN_LEFT_MID, text);
    }
    if (type == OptionType::TEXT_WITH_VALUE)
    {
        this->indentText();
        setupPressureLabel(this->root, &this->rightHandObj, -16, 0, LV_ALIGN_RIGHT_MID, value.STRING);
    }
    else if (type == OptionType::HEADER)
    {
        lv_obj_add_style(this->text, &stylepointer, LV_PART_MAIN);
    }
    else if (type == OptionType::ON_OFF)
    {
        this->indentText();

        this->ui_imgOn = lv_image_create(this->root);
        lv_image_set_src(this->ui_imgOn, &imgOn);
        // lv_obj_set_x(this->ui_imgOn, DISPLAY_WIDTH - 16 - imgOn.header.w);
        lv_obj_set_align(this->ui_imgOn, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(this->ui_imgOn, -16);

        this->ui_imgOff = lv_image_create(this->root);
        lv_image_set_src(this->ui_imgOff, &imgOff);
        // lv_obj_set_x(this->ui_imgOff, DISPLAY_WIDTH - 16 - imgOff.header.w);
        lv_obj_set_align(this->ui_imgOff, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(this->ui_imgOff, -16);

        // lv_obj_set_click(this->ui_imgOn, true);
        lv_obj_add_flag(this->ui_imgOn, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->ui_imgOn, ui_clicked_imgOn, LV_EVENT_ALL, this);

        // lv_obj_set_click(this->ui_imgOff, true);
        lv_obj_add_flag(this->ui_imgOff, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_event_cb(this->ui_imgOff, ui_clicked_imgOff, LV_EVENT_ALL, this);

        this->setBooleanValue(false);
    }

    if (_event_cb != NULL)
    {
        this->event_cb = _event_cb;
    }
}

void Option::setRightHandText(const char *text)
{
    if (this->type == OptionType::TEXT_WITH_VALUE)
    {
        lv_label_set_text(this->rightHandObj, text);
        // lv_label_set_text_fmt(this->rightHandObj, "Compressor Frozen: %s", statusBittset & (1 << COMPRESSOR_FROZEN) ? "Yes" : "No");
    }
}

void Option::setBooleanValue(bool value, bool netSend)
{
    if (this->type == OptionType::ON_OFF)
    {
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
            this->event_cb(&value);
        }
    }
}