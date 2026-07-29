#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_ST7701.h"
#include "Touch_GT911.h"

#define LVGL_WIDTH     ESP_PANEL_LCD_WIDTH
#define LVGL_HEIGHT    ESP_PANEL_LCD_HEIGHT
// Partial draw buffers: 90 lines each, sized in BYTES (RGB565 = 2 bytes/px).
// Bumped 30 -> 90 to cut per-strip overhead when scrolling the (now larger, DPI-scaled) settings
// list: fewer flush strips per full-screen redraw (640/90 ~= 8 vs 22) and tall glyphs get split
// across fewer strips, so they're re-blended fewer times. Cost: 2 buffers now use
// 480*90*2*2 = ~172.8KB of internal DMA RAM (was ~57.6KB). If that allocation ever fails, dial
// this back toward 60.
// ; was: #define LVGL_BUF_LINES 30 (28.8KB/buffer)
// ; was: #define LVGL_BUF_LEN (LVGL_WIDTH * 20) in pixels, multiplied by sizeof(lv_color_t)
#define LVGL_BUF_LINES 90
#define LVGL_BUF_BYTES ((uint32_t)LVGL_WIDTH * LVGL_BUF_LINES * 2)

#define EXAMPLE_LVGL_TICK_PERIOD_MS  2


extern lv_display_t *disp;

void Lvgl_print(const char * buf);
void Lvgl_Display_LCD(lv_display_t *display, const lv_area_t *area, uint8_t *px_map); // Displays LVGL content on the LCD.    This function implements associating LVGL data to the LCD screen
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data);                // Read the touchpad
void example_increase_lvgl_tick(void *arg);

struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

touch_and_screen Lvgl_Init(void);
void Lvgl_Loop(void);
