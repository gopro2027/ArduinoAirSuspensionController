#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

LV_IMG_DECLARE(home_bg);
LV_IMG_DECLARE(navbar_home);

ScrHome scrHome(navbar_home, true);

void ScrHome::init(void)
{
    Scr::init();
    lv_obj_add_flag(this->rect_bg, LV_OBJ_FLAG_HIDDEN); // hide grey background, we have a different one on this page

    // background image
    this->icon_home_bg = lv_image_create(this->scr);
    lv_image_set_src(this->icon_home_bg, &home_bg);
    lv_obj_center(this->icon_home_bg);

    lv_obj_move_foreground(this->icon_navbar);                  // bring navbar to foreground
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger); // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);  // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);    // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);     // pressures to foreground front
    lv_obj_move_foreground(this->ui_lblPressureTank);           // pressures to foreground front
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
            setValveBit(REAR_PASSENGER_OUT);
        }
    }
}

void ScrHome::loop()
{
    Scr::loop();
}
