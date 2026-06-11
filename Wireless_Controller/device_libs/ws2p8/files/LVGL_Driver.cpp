#include "LVGL_Driver.h"
#include <Arduino.h>

// Prefer internal RAM: LVGL software-renders into these buffers, and internal RAM is much
// faster than PSRAM. 2x 19.2KB fits easily. No cache sync needed on the PSRAM fallback —
// the flush path is CPU-driven Arduino SPI (transferBytes), not DMA.
// ; was: two full-frame (153.6KB) buffers in MALLOC_CAP_SPIRAM
static lv_color_t *alloc_draw_buf(void)
{
  void *p = heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!p)
    p = heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_SPIRAM);
  if (!p)
    p = malloc(LVGL_BUF_BYTES);
  return (lv_color_t *)p;
}

static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

/* Serial debugging */
void Lvgl_print(const char *buf)
{
  // Serial.printf(buf);
  // Serial.flush();
}

/*  Display flushing
    Displays LVGL content on the LCD
    Uses hardware rotation via MADCTL - no software rotation needed
    LVGL resolution is set to match the rotated display dimensions
*/
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  // Direct output - hardware MADCTL handles rotation
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
  lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data)
{
  uint16_t touchpad_x[5] = {0};
  uint16_t touchpad_y[5] = {0};
  uint16_t strength[5] = {0};
  uint8_t touchpad_cnt = 0;
  Touch_Read_Data();
  uint8_t touchpad_pressed = Touch_Get_XY(touchpad_x, touchpad_y, strength, &touchpad_cnt, CST328_LCD_TOUCH_MAX_POINTS);
  if (touchpad_pressed && touchpad_cnt > 0)
  {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void example_increase_lvgl_tick(void *arg)
{
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

touch_and_screen Lvgl_Init(void)
{
  if (!buf1)
    buf1 = alloc_draw_buf();
  if (!buf2)
    buf2 = alloc_draw_buf();
  if (!buf1 || !buf2)
    Serial.println("[LVGL] FATAL: display buffer alloc failed");

  lv_init();

  /*Initialize the display*/
  static lv_display_t *disp = lv_display_create(LVGL_WIDTH, LVGL_HEIGHT);
  lv_display_set_flush_cb(disp, Lvgl_Display_LCD);
  // PARTIAL: only dirty areas are rendered and pushed over SPI, instead of re-rendering and
  // re-transmitting the whole 153.6KB frame (>15ms of blocking wire time) on every refresh.
  // ; was: full-frame buffers + lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_FULL)
  lv_display_set_buffers(disp, buf1, buf2, LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // 60 FPS refresh: bench-verified with the perf overlay after the PARTIAL switch — idle CPU
  // 2-4%, page-change peaks ~35% at 30 FPS, so doubling the refresh rate has ample headroom.
  // ; was: LV_DEF_REFR_PERIOD default (33ms / 30 FPS)
  lv_timer_t *refr = lv_display_get_refr_timer(disp);
  if (refr)
    lv_timer_set_period(refr, 16);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, Lvgl_Touchpad_Read);

  /* Create simple label */
  // lv_obj_t *label = lv_label_create(lv_screen_active());
  // lv_label_set_text(label, "Hello Ardino and LVGL!");
  // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

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
  lv_timer_handler(); /* let the GUI do its work */
}

