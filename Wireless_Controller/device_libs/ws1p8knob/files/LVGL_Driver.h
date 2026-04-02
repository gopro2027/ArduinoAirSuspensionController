#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_SH8601.h"
#include "Touch_CST816.h"

#define LVGL_WIDTH  LCD_WIDTH
#define LVGL_HEIGHT LCD_HEIGHT
// Partial strip height: each flush is width*lines*2 bytes. Large strips + internal LVGL buffers exhaust
// spi_master setup_dma_priv_buffer (internal DMA heap); keep this small (16–24 typical).
#define LVGL_PARTIAL_LINES 16
#define LVGL_BUF_BYTES ((uint32_t)((size_t)LVGL_WIDTH * (size_t)LVGL_PARTIAL_LINES * sizeof(lv_color_t)))

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
