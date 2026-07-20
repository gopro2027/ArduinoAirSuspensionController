#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_ST7789.h"
#include "Touch_CST328.h"

#define LVGL_WIDTH LCD_WIDTH
#define LVGL_HEIGHT LCD_HEIGHT
// Partial render buffers: 40 lines each, sized in BYTES (RGB565 = 2 bytes/px).
// The old full-frame config mixed units: heap_caps_malloc() was given LVGL_BUF_LEN bytes but
// lv_display_set_buffers() was told LVGL_BUF_LEN * sizeof(lv_color_t) (3 bytes in LVGL 9),
// declaring 3x more buffer than was actually allocated. That overrun is why the earlier
// partial-buffer attempts (/20, /4) "crashed"; FULL render mode only survived because LVGL
// caps its usage at the real frame size in that mode.
// ; was: #define LVGL_BUF_LEN (LVGL_WIDTH * LVGL_HEIGHT * 2)  (full frame, FULL render mode)
#define LVGL_BUF_LINES 40
#define LVGL_BUF_BYTES ((uint32_t)LVGL_WIDTH * LVGL_BUF_LINES * 2)

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

void Lvgl_print(const char *buf);
// void Lvgl_Display_LCD( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p ); // Displays LVGL content on the LCD.    This function implements associating LVGL data to the LCD screen
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
// void Lvgl_Touchpad_Read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data); // Read the touchpad
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data);
void example_increase_lvgl_tick(void *arg);

struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

touch_and_screen Lvgl_Init(void);
void Lvgl_Loop(void);
