#include "util.h"
int sr_contains(SimpleRect r, SimplePoint p)
{
    return p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}

int cr_contains(CenterRect cr, SimplePoint p)
{
    SimpleRect sr = {cr.cx - (cr.w / 2), cr.cy - (cr.h / 2), cr.w, cr.h};
    return sr_contains(sr, p);
}

// first column (left)
CenterRect ctr_row0col0up = {48, 89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col0down = {48, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col0up = {48, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col0down = {48, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// second column (center)
CenterRect ctr_row0col1up = {118, 78, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col1down = {118, 126, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col1up = {118, 204, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col1down = {118, 252, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// third column (right)
CenterRect ctr_row0col2up = {189, 89, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row0col2down = {189, 137, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

CenterRect ctr_row1col2up = {189, 193, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};
CenterRect ctr_row1col2down = {189, 241, ARROW_BUTTON_WIDTH, ARROW_BUTTON_HEIGHT};

// bottom nav
SimpleRect navbarbtn_home = {0, 291, 80, 29};
SimpleRect navbarbtn_presets = {80, 291, 80, 29};
SimpleRect navbarbtn_settings = {160, 291, 80, 29};

int currentPressures[5];