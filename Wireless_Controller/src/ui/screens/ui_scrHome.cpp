#include "ui_scrHome.h"
#include "ui/ui.h" // sketchy backwards import may break in the future

LV_IMAGE_DECLARE(navbar_home);

LV_IMAGE_DECLARE(navbar_home);
ScrHome scrHome(navbar_home, true);

void draw_arrow(lv_obj_t *parent, CenterRect cr, int direction)
{
    CenterRect c = scaleCR(cr);

    const int dx  = LV_MAX(1, (int)(c.w * (7.0  / ARROW_BUTTON_WIDTH)));
    const int dy3 = LV_MAX(1, (int)(c.h * (3.0  / ARROW_BUTTON_HEIGHT)));
    const int dy4 = LV_MAX(1, (int)(c.h * (4.0  / ARROW_BUTTON_HEIGHT)));

    lv_point_precise_t *pts = new lv_point_precise_t[3];
    pts[0].x = (lv_coord_t)(c.cx - dx);
    pts[0].y = (lv_coord_t)(c.cy - direction * dy3);
    pts[1].x = (lv_coord_t)(c.cx);
    pts[1].y = (lv_coord_t)(c.cy + direction * dy4);
    pts[2].x = (lv_coord_t)(c.cx + dx);
    pts[2].y = (lv_coord_t)(c.cy - direction * dy3);

    static lv_style_t style;
    static bool inited = false;
    if (!inited) {
        lv_style_init(&style);
        int lw = LV_MAX(1, (int)(c.w * (2.0 / ARROW_BUTTON_WIDTH))); // 2px @ author space
        lv_style_set_line_width(&style, lw);
        lv_style_set_line_color(&style, lv_color_hex(THEME_COLOR_LIGHT));
        lv_style_set_line_rounded(&style, true);
        inited = true;
    }

    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts, 3);
    lv_obj_add_style(line, &style, 0);
}

// Scaled circle “button” at cr, then the arrow on top.
void drawCircle(lv_obj_t *parent, CenterRect cr, int direction)
{
    CenterRect c = scaleCR(cr);

    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(o, (lv_coord_t)c.w, (lv_coord_t)c.h);
    lv_obj_set_pos(o, (lv_coord_t)(c.cx - c.w/2), (lv_coord_t)(c.cy - c.h/2));
    lv_obj_set_style_bg_color(o, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(o, 0, 0);

    draw_arrow(parent, cr, direction); // arrow uses the same author-space cr internally
}

void drawRect(lv_obj_t *parent, SimpleRect sr)
{
    SimpleRect r = scaleSR(sr);

    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(o, (lv_coord_t)r.w, (lv_coord_t)r.h);
    lv_obj_set_pos(o, (lv_coord_t)r.x, (lv_coord_t)r.y);
    lv_obj_set_style_bg_color(o, lv_color_hex(GENERIC_GREY_VERY_DARK), 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
}

void drawPill(lv_obj_t *parent, CenterRect up, CenterRect down)
{
    // middle rect between the two circles (author space → display)
    SimpleRect mid = { up.cx - up.w/2, up.cy, up.w, (down.cy - up.cy) };
    drawRect(parent, mid);

    // end caps
    drawCircle(parent, up,   -1);
    drawCircle(parent, down,  1);

    // center divider line (scale endpoints)
    CenterRect upS   = scaleCR(up);
    CenterRect downS = scaleCR(down);
    const int ymid   = (int)((upS.cy + downS.cy) / 2);

    const int xL = (int)(upS.cx - upS.w / 3.5f);
    const int xR = (int)(upS.cx + upS.w / 3.5f);

    lv_point_precise_t *pts = new lv_point_precise_t[2];
    pts[0].x = (lv_coord_t)xL; pts[0].y = (lv_coord_t)ymid;
    pts[1].x = (lv_coord_t)xR; pts[1].y = (lv_coord_t)ymid;

    static lv_style_t style;
    static bool inited = false;
    if (!inited) {
        lv_style_init(&style);
        // 1px @ author space, scaled to current width
        int lw = LV_MAX(1, (int)(upS.w * (1.0 / ARROW_BUTTON_WIDTH)));
        lv_style_set_line_width(&style, lw);
        lv_style_set_line_color(&style, lv_color_hex(THEME_COLOR_DARK));
        lv_style_set_line_rounded(&style, true);
        inited = true;
    }

    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts, 2);
    lv_obj_add_style(line, &style, 0);
}

void ScrHome::init(void)
{
    Scr::init();

    // Navbar image at bottom, stretched
    lv_obj_t *nav = lv_image_create(this->scr);
    lv_image_set_src(nav, &navbar_home);
    lv_obj_set_size(nav, 320, 49);   // scale to full width
    lv_image_set_align(nav, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    

    // keep reference if you need it
    this->icon_navbar = nav;

    // Your existing pills
    drawPill(this->scr, ctr_row0col0up, ctr_row0col0down);
    drawPill(this->scr, ctr_row1col0up, ctr_row1col0down);
    drawPill(this->scr, ctr_row0col1up, ctr_row0col1down);
    drawPill(this->scr, ctr_row1col1up, ctr_row1col1down);
    drawPill(this->scr, ctr_row0col2up, ctr_row0col2down);
    drawPill(this->scr, ctr_row1col2up, ctr_row1col2down);

    // Bring UI forward
    lv_obj_move_foreground(this->icon_navbar);
    lv_obj_move_foreground(this->ui_lblPressureFrontPassenger);
    lv_obj_move_foreground(this->ui_lblPressureRearPassenger);
    lv_obj_move_foreground(this->ui_lblPressureFrontDriver);
    lv_obj_move_foreground(this->ui_lblPressureRearDriver);
    lv_obj_move_foreground(this->ui_lblPressureTank);
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