#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

LV_IMG_DECLARE(navbar_home);

ScrHome scrHome(navbar_home, true);

void draw_arrow(lv_obj_t *parent, CenterRect cr, int direction)
{
    lv_point_precise_t *line_points = new lv_point_precise_t[3];
    line_points[0].x = -7 + cr.cx;
    line_points[0].y = -3 * direction + cr.cy;
    line_points[1].x = 0 + cr.cx;
    line_points[1].y = 4 * direction + cr.cy;
    line_points[2].x = 7 + cr.cx;
    line_points[2].y = -3 * direction + cr.cy;

    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_color_hex(THEME_COLOR_LIGHT));
    lv_style_set_line_rounded(&style_line, true);

    /*Create a line and apply the new style*/
    lv_obj_t *line1;
    line1 = lv_line_create(parent);
    lv_line_set_points(line1, line_points, 3); /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
}

void drawCircle(lv_obj_t *parent, CenterRect cr, int direction)
{
    lv_obj_t *my_Cir = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(my_Cir, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(my_Cir, cr.w, cr.h);
    lv_obj_set_pos(my_Cir, cr.cx - cr.w / 2, cr.cy - cr.h / 2);
    lv_obj_set_style_bg_color(my_Cir, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    // lv_obj_set_style_border_color(my_Cir, lv_color_hex(THEME_COLOR_LIGHT), 0);
    lv_obj_set_style_radius(my_Cir, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(my_Cir, 0, 0);

    draw_arrow(parent, cr, direction);
}

void drawRect(lv_obj_t *parent, SimpleRect sr)
{
    lv_obj_t *my_Cir = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(my_Cir, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(my_Cir, sr.w, sr.h);
    lv_obj_set_pos(my_Cir, sr.x, sr.y);
    lv_obj_set_style_bg_color(my_Cir, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_radius(my_Cir, 0, 0);
    lv_obj_set_style_border_width(my_Cir, 0, 0);
}

void drawPill(lv_obj_t *parent, CenterRect up, CenterRect down)
{
    SimpleRect sr = {up.cx - up.w / 2, up.cy, up.w, down.cy - up.cy};
    drawRect(parent, sr);
    drawCircle(parent, up, -1);
    drawCircle(parent, down, 1);

    lv_point_precise_t *line_points = new lv_point_precise_t[3];
    line_points[0].x = up.cx - up.w / 3.5;
    line_points[0].y = (up.cy + down.cy) / 2;
    line_points[1].x = up.cx + up.w / 3.5;
    line_points[1].y = line_points[0].y;

    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_color_hex(THEME_COLOR_DARK));
    lv_style_set_line_rounded(&style_line, true);

    /*Create a line and apply the new style*/
    lv_obj_t *line1;
    line1 = lv_line_create(parent);
    lv_line_set_points(line1, line_points, 2); /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
}

void ScrHome::init(void)
{
    Scr::init();

    drawPill(this->scr, ctr_row0col0up, ctr_row0col0down);
    drawPill(this->scr, ctr_row1col0up, ctr_row1col0down);

    drawPill(this->scr, ctr_row0col1up, ctr_row0col1down);
    drawPill(this->scr, ctr_row1col1up, ctr_row1col1down);

    drawPill(this->scr, ctr_row0col2up, ctr_row0col2down);
    drawPill(this->scr, ctr_row1col2up, ctr_row1col2down);

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
