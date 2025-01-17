#ifndef util_h
#define util_h

typedef struct
{
    double x, y;
} SimplePoint;

typedef struct
{
    double x, y, w, h;

} SimpleRect;

typedef struct {
    double cx, cy, w, h;
} CenterRect;

int sr_contains(SimpleRect r, SimplePoint p);

int cr_contains(CenterRect cr, SimplePoint p);



#define ARROW_BUTTON_WIDTH 54
#define ARROW_BUTTON_HEIGHT 44

// first column (left)
extern CenterRect ctr_row0col0up;
extern CenterRect ctr_row0col0down;

extern CenterRect ctr_row1col0up;
extern CenterRect ctr_row1col0down;

// second column (center)
extern CenterRect ctr_row0col1up;
extern CenterRect ctr_row0col1down;

extern CenterRect ctr_row1col1up;
extern CenterRect ctr_row1col1down;

// third column (right)
extern CenterRect ctr_row0col2up;
extern CenterRect ctr_row0col2down;

extern CenterRect ctr_row1col2up;
extern CenterRect ctr_row1col2down;

// bottom nav
extern SimpleRect navbarbtn_home;
extern SimpleRect navbarbtn_presets;
extern SimpleRect navbarbtn_settings;

#endif