#include "alert.h"

Alert::Alert(Scr *scr)
{
    this->text = lv_label_create(scr->scr);
    lv_obj_set_style_text_color(this->text, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(this->text, LV_SIZE_CONTENT);
    lv_obj_set_height(this->text, LV_SIZE_CONTENT);
    lv_obj_set_x(this->text, 0);
    lv_obj_set_y(this->text, 0);
    lv_obj_set_align(this->text, LV_ALIGN_TOP_LEFT);
    lv_label_set_text(this->text, "");
    lv_obj_set_style_opa(this->text, LV_OPA_50, 0);

    this->rect = lv_obj_create(scr->scr);
    lv_obj_remove_style_all(this->rect);
    lv_obj_set_style_bg_opa(this->rect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(this->rect, DISPLAY_WIDTH, 20);
    lv_obj_set_align(this->rect, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(this->rect, lv_color_hex(esp_random()), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(this->rect, LV_OPA_50, 0);

    loop(); // hide it

    this->expiry = 0;
}

void Alert::loop()
{
    if (millis() > expiry)
    {
        if (!lv_obj_has_flag(this->text, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_add_flag(this->text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(this->rect, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void Alert::show(lv_color_t color, char *text, unsigned long expiry)
{

    this->expiry = expiry;

    lv_obj_remove_flag(this->text, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(this->rect, LV_OBJ_FLAG_HIDDEN);

    lv_obj_move_foreground(this->rect);
    lv_obj_move_foreground(this->text);

    lv_obj_set_style_bg_color(this->rect, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_fmt(this->text, "%s", text);
}