#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_CO5300.h"
#include "Touch_CST9217.h"

#define LVGL_WIDTH  LCD_WIDTH
#define LVGL_HEIGHT LCD_HEIGHT
// Must equal CO5300_LVGL_PARTIAL_LINES in Display_CO5300.h (SPI max_transfer_sz).
#define LVGL_PARTIAL_LINES CO5300_LVGL_PARTIAL_LINES
// Sized in BYTES (RGB565 = 2 bytes/px), not sizeof(lv_color_t) -- that is 3 bytes in LVGL 9, so
// it would declare a 75-line strip to LVGL while the SPI bus is set up for 50.
#define LVGL_BUF_BYTES ((uint32_t)LVGL_WIDTH * LVGL_PARTIAL_LINES * 2)

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

void Lvgl_print(const char *buf);
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void Lvgl_Rounder(lv_event_t *e);
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data);
void example_increase_lvgl_tick(void *arg);

struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

touch_and_screen Lvgl_Init(void);
void Lvgl_Loop(void);
