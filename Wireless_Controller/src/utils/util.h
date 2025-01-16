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

#endif