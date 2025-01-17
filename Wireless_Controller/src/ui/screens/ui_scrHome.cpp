#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

LV_IMG_DECLARE(home_bg);
LV_IMG_DECLARE(navbar_home);

ScrHome scrHome(navbar_home);

void setupPressureLabel(ScrHome *scr, lv_obj_t **label, int x, int y, lv_align_t align) {
    *label = lv_label_create(scr->scr);
    lv_obj_set_style_text_color(*label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(*label, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(*label, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(*label, x);
    lv_obj_set_y(*label, y);
    lv_obj_set_align(*label, align);
    lv_label_set_text(*label, "0");

    lv_obj_move_foreground(*label);
}

void ScrHome::init(void)
{
    Scr::init();
    lv_obj_add_flag(this->rect_bg, LV_OBJ_FLAG_HIDDEN); // hide grey background, we have a different one on this page

    // background image
    this->icon_home_bg = lv_image_create(this->scr);
    lv_image_set_src(this->icon_home_bg, &home_bg);
    lv_obj_center(this->icon_home_bg);

    lv_obj_move_foreground(this->icon_navbar); // bring navbar to foreground

    // air pressures at top
    const int xPadding = 45;
    setupPressureLabel(this, &this->ui_lblPressureFrontDriver, xPadding, 10, LV_ALIGN_TOP_LEFT);
    setupPressureLabel(this, &this->ui_lblPressureRearDriver, xPadding, 40, LV_ALIGN_TOP_LEFT);
    setupPressureLabel(this, &this->ui_lblPressureFrontPassenger, -xPadding, 10, LV_ALIGN_TOP_RIGHT);
    setupPressureLabel(this, &this->ui_lblPressureRearPassenger, -xPadding, 40, LV_ALIGN_TOP_RIGHT);
    setupPressureLabel(this, &this->ui_lblPressureTank, 0, 10, LV_ALIGN_TOP_MID);
    
}

// down = true when just pressed, false when just released
void ScrHome::runTouchInput(SimplePoint pos, bool down)
{
    Scr::runTouchInput(pos,down);
    if (cr_contains(ctr_row0col0up, pos))
    {
        log_i("pressed ctr_row0col0up");
        static int cnt = 0;
        cnt++;
        lv_label_set_text_fmt(this->ui_lblPressureTank, "%u", cnt);
        lv_label_set_text_fmt(this->ui_lblPressureFrontDriver, "%u", cnt);
        lv_label_set_text_fmt(this->ui_lblPressureRearDriver, "%u", cnt);
        lv_label_set_text_fmt(this->ui_lblPressureFrontPassenger, "%u", cnt);
        lv_label_set_text_fmt(this->ui_lblPressureRearPassenger, "%u", cnt);
    }
}

void ScrHome::loop()
{
    Scr::loop();
}