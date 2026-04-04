#include "LVGL_Driver.h"
#include <Arduino.h>
#include "esp_attr.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"

// Partial strips only: full-frame SPI TX needs huge spi_master internal DMA "priv" buffers and fails on this chip (setup_dma_priv_buffer).
// Draw buffers live in PSRAM; flush path calls esp_cache_msync before SPI reads them.
static lv_color_t *alloc_draw_buf(void)
{
    const size_t n = (size_t)LVGL_BUF_BYTES;
    void *p = heap_caps_malloc(n, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    if (!p)
        p = heap_caps_malloc(n, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!p)
        p = malloc(n);
    return (lv_color_t *)p;
}

static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

void Lvgl_print(const char *buf)
{
}

void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int32_t w = lv_area_get_width(area);
    const int32_t h = lv_area_get_height(area);
    const size_t len = (size_t)w * (size_t)h * sizeof(uint16_t);
    if (esp_ptr_external_ram(px_map)) {
        esp_cache_msync(px_map, len, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }
    LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t tp_x, tp_y;
    uint8_t touched = getTouch(&tp_x, &tp_y);
    if (touched) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

touch_and_screen Lvgl_Init(void)
{
    if (!buf1)
        buf1 = alloc_draw_buf();
    if (!buf2)
        buf2 = alloc_draw_buf();
    if (!buf1 || !buf2) {
        Serial.println("[LVGL] FATAL: display buffer alloc failed");
    }

    lv_init();

    static lv_display_t *disp = lv_display_create(LVGL_WIDTH, LVGL_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(disp, Lvgl_Display_LCD);
    lv_display_set_buffers(disp, buf1, buf2, LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(disp);
    lv_timer_t *refr = lv_display_get_refr_timer(disp);
    if (refr)
        lv_timer_set_period(refr, 16);

    static lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, Lvgl_Touchpad_Read);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
    return {indev, disp};
}

void Lvgl_Loop(void)
{
    lv_timer_handler();
}
