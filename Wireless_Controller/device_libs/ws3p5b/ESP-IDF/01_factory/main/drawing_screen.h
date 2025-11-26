#ifndef __DRAWING_SCREEN_H__
#define __DRAWING_SCREEN_H__
#include "lvgl.h"

extern lv_obj_t *canvas;
extern bool canvas_exit;

#ifdef __cplusplus
extern "C" {
#endif

void drawing_screen_init(void);


#ifdef __cplusplus
}
#endif


#endif