#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_SH8601.h"
#include "Touch_CST816.h"

#define LVGL_WIDTH  LCD_WIDTH
#define LVGL_HEIGHT LCD_HEIGHT
// Must equal SH8601_LVGL_PARTIAL_LINES in Display_SH8601.h (SPI max_transfer_sz).
#define LVGL_PARTIAL_LINES SH8601_LVGL_PARTIAL_LINES
// RGB565 = 2 bytes/px, matching SH8601_MAX_TRANSFER_SZ exactly.
// ; was: * sizeof(lv_color_t) (3 bytes in LVGL 9), which let LVGL fit 36 rows in the buffer —
// a single flush could then exceed the SPI max_transfer_sz sized for 24 rows.
#define LVGL_BUF_BYTES ((uint32_t)((size_t)LVGL_WIDTH * (size_t)LVGL_PARTIAL_LINES * 2))

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

void Lvgl_print(const char *buf);
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data);
void example_increase_lvgl_tick(void *arg);

struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

touch_and_screen Lvgl_Init(void);
void Lvgl_Loop(void);
