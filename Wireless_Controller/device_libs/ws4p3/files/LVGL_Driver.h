#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_ST7262.h"
#include "Touch_GT911.h"

#define LVGL_WIDTH     ESP_PANEL_LCD_WIDTH
#define LVGL_HEIGHT    ESP_PANEL_LCD_HEIGHT

// Partial draw buffers, sized in BYTES (RGB565 = 2 bytes/px).
// Do NOT write this as `LINES * sizeof(lv_color_t)`: in LVGL 9 lv_color_t is 3 bytes even at
// LV_COLOR_DEPTH 16, which silently over-allocates by 50%.
//
// 40 lines: 800*40*2 = 64 KB per buffer, 128 KB of internal DMA RAM for the pair. This panel is
// 800 px wide vs the 2.8b's 480, so the 2.8b's 90-line buffers would cost 288 KB here -- more
// internal RAM than can be spared alongside the WiFi driver (the SSID scan needs internal RAM
// even though NimBLE is pushed into PSRAM). A full-screen redraw is 480/40 = 12 flush strips,
// close to the 2.8b's 8. Raise toward 60 lines if scrolling feels choppy and the allocation
// still succeeds; drop toward 24 if heap_caps_malloc returns NULL at boot.
#define LVGL_BUF_LINES 40
#define LVGL_BUF_BYTES ((uint32_t)LVGL_WIDTH * LVGL_BUF_LINES * 2)

#define EXAMPLE_LVGL_TICK_PERIOD_MS  2

extern lv_display_t *disp;

void Lvgl_print(const char *buf);
void Lvgl_Display_LCD(lv_display_t *display, const lv_area_t *area, uint8_t *px_map); // Displays LVGL content on the LCD
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data);                    // Read the touchpad
void example_increase_lvgl_tick(void *arg);

struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

touch_and_screen Lvgl_Init(void);
void Lvgl_Loop(void);
