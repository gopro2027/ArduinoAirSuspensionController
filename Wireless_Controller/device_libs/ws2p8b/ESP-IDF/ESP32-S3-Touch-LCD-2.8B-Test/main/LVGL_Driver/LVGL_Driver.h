#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

#include "ST7701S.h"
#include "GT911.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

extern lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
extern lv_disp_drv_t disp_drv;      // contains callback functions
extern lv_disp_t *disp;
void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void example_increase_lvgl_tick(void *arg);
/*Read the touchpad*/
void example_touchpad_read( lv_indev_drv_t * drv, lv_indev_data_t * data );

void LVGL_Init(void);