#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

LV_IMG_DECLARE(home_bg);
LV_IMG_DECLARE(navbar_home);

ScrHome scrHome(navbar_home);

void setupPressureLabel(ScrHome *scr, lv_obj_t **label, int x, int y, lv_align_t align)
{
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
    Scr::runTouchInput(pos, down);

    // AirupPacket aup;
    // sendRestPacket(&aup);

    if (down == false)
    {
        closeValves();
    }
    else
    {
        // driver side
        bool _FRONT_DRIVER_IN = cr_contains(ctr_row0col0up, pos);
        bool _FRONT_DRIVER_OUT = cr_contains(ctr_row0col0down, pos);

        bool _REAR_DRIVER_IN = cr_contains(ctr_row1col0up, pos);
        bool _REAR_DRIVER_OUT = cr_contains(ctr_row1col0down, pos);

        // passenger side
        bool _FRONT_PASSENGER_IN = cr_contains(ctr_row0col2up, pos);
        bool _FRONT_PASSENGER_OUT = cr_contains(ctr_row0col2down, pos);

        bool _REAR_PASSENGER_IN = cr_contains(ctr_row1col2up, pos);
        bool _REAR_PASSENGER_OUT = cr_contains(ctr_row1col2down, pos);

        // axles
        bool _FRONT_AXLE_IN = cr_contains(ctr_row0col1up, pos);
        bool _FRONT_AXLE_OUT = cr_contains(ctr_row0col1down, pos);

        bool _REAR_AXLE_IN = cr_contains(ctr_row1col1up, pos);
        bool _REAR_AXLE_OUT = cr_contains(ctr_row1col1down, pos);

        // driver side
        if (_FRONT_DRIVER_IN)
        {
            setValveBit(FRONT_DRIVER_IN);
        }

        if (_FRONT_DRIVER_OUT)
        {
            setValveBit(FRONT_DRIVER_OUT);
        }

        if (_REAR_DRIVER_IN)
        {
            setValveBit(REAR_DRIVER_IN);
        }

        if (_REAR_DRIVER_OUT)
        {
            setValveBit(REAR_DRIVER_OUT);
        }

        // passenger side
        if (_FRONT_PASSENGER_IN)
        {
            setValveBit(FRONT_PASSENGER_IN);
        }

        if (_FRONT_PASSENGER_OUT)
        {
            setValveBit(FRONT_PASSENGER_OUT);
        }

        if (_REAR_PASSENGER_IN)
        {
            setValveBit(REAR_PASSENGER_IN);
        }

        if (_REAR_PASSENGER_OUT)
        {
            setValveBit(REAR_PASSENGER_OUT);
        }

        // axles
        if (_FRONT_AXLE_IN)
        {
            setValveBit(FRONT_DRIVER_IN);
            setValveBit(FRONT_PASSENGER_IN);
        }

        if (_FRONT_AXLE_OUT)
        {
            setValveBit(FRONT_DRIVER_OUT);
            setValveBit(FRONT_PASSENGER_OUT);
        }

        if (_REAR_AXLE_IN)
        {
            setValveBit(REAR_DRIVER_IN);
            setValveBit(REAR_PASSENGER_IN);
        }

        if (_REAR_AXLE_OUT)
        {
            setValveBit(REAR_DRIVER_OUT);
            setValveBit(REAR_PASSENGER_IN);
        }
    }
}

void ScrHome::loop()
{
    Scr::loop();
    this->updatePressureValues();
}

void ScrHome::updatePressureValues()
{
    lv_label_set_text_fmt(this->ui_lblPressureFrontPassenger, "%u", currentPressures[WHEEL_FRONT_PASSENGER]);
    lv_label_set_text_fmt(this->ui_lblPressureRearPassenger, "%u", currentPressures[WHEEL_REAR_PASSENGER]);
    lv_label_set_text_fmt(this->ui_lblPressureFrontDriver, "%u", currentPressures[WHEEL_FRONT_DRIVER]);
    lv_label_set_text_fmt(this->ui_lblPressureRearDriver, "%u", currentPressures[WHEEL_REAR_DRIVER]);
    lv_label_set_text_fmt(this->ui_lblPressureTank, "%u", currentPressures[_TANK_INDEX]);
}