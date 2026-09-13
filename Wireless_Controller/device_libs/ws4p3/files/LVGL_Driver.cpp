/*****************************************************************************
  | File        :   LVGL_Driver.cpp  (ws4p3 / ESP32-S3-Touch-LCD-4.3)
******************************************************************************/
#include "LVGL_Driver.h"

lv_display_t *disp = NULL;

static void *buf1 = NULL;
static void *buf2 = NULL;

/* Serial debugging */
void Lvgl_print(const char *buf)
{
    // Serial.printf(buf);
    // Serial.flush();
}

/*  Display flushing
    Blits one dirty strip into the RGB panel's frame buffer.
*/
void Lvgl_Display_LCD(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, px_map);
  lv_display_flush_ready(display);
}

/*Read the touchpad*/
void Lvgl_Touchpad_Read(lv_indev_t *indev, lv_indev_data_t *data)
{
  uint16_t touchpad_x[GT911_LCD_TOUCH_MAX_POINTS] = {0};
  uint16_t touchpad_y[GT911_LCD_TOUCH_MAX_POINTS] = {0};
  uint16_t strength[GT911_LCD_TOUCH_MAX_POINTS]   = {0};
  uint8_t touchpad_cnt = 0;
  Touch_Read_Data();
  uint8_t touchpad_pressed = Touch_Get_XY(touchpad_x, touchpad_y, strength, &touchpad_cnt, GT911_LCD_TOUCH_MAX_POINTS);
  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
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
  lv_init();

  // Partial draw buffers in internal DMA-capable RAM. The RGB panel owns its own 768 KB frame
  // buffer in PSRAM; these are just the staging strips LVGL renders into before LCD_addWindow()
  // copies them across.
  if (!buf1) buf1 = heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!buf2) buf2 = heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!buf1 || !buf2) {
    // Blank screen with no explanation is the usual symptom; log it so the cause is findable.
    printf("LVGL draw buffer alloc FAILED (%u bytes each) -- lower LVGL_BUF_LINES\r\n",
           (unsigned)LVGL_BUF_BYTES);
  }

  /*Initialize the display*/
  disp = lv_display_create(LVGL_WIDTH, LVGL_HEIGHT);
  lv_display_set_flush_cb(disp, Lvgl_Display_LCD);
  lv_display_set_buffers(disp, buf1, buf2, LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_user_data(disp, panel_handle);

  // 60 FPS target: the RGB panel scans itself out of its own frame buffer, so LVGL only pays for
  // the dirty strips. Same override the other RGB board (ws2p8b) uses.
  // ; was: LV_DEF_REFR_PERIOD default (33ms / 30 FPS)
  lv_timer_t *refr = lv_display_get_refr_timer(disp);
  if (refr)
    lv_timer_set_period(refr, 16);

  /*Initialize the input device driver*/
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, Lvgl_Touchpad_Read);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  return {indev, disp};
}

void Lvgl_Loop(void)
{
  uint32_t time_till_next = lv_timer_handler();
  delay(time_till_next);  // Sleep until next update needed
}
