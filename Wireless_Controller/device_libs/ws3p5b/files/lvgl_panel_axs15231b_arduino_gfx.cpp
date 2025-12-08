// drv_axs15231b_lvgl.cpp
// ESP32-S3 + AXS15231B (QSPI) + LVGL 9 + Arduino_GFX performance driver
// - Full-frame double buffering (RGB565)
// - Full-screen flush (ignores area) to reduce per-rect overhead
// - Panel PCLK step-ups (60 -> 80 MHz fallback chain)
// - DMA-friendly buffer caps, try 1 buffer in internal RAM
// - 1 kHz LVGL tick (esp_timer) + UI task pinned to core with high prio
// - TCA9554 expander pulse-reset for Waveshare Type-B
// - Touch read capped (continue_reading=false) to avoid extra polls

#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "TCA9554.h"
// #include <esp32_smartdisplay.h>   // for smartdisplay_lcd_set_backlight()

extern "C" lv_indev_t *indev;   // owned by your smartdisplay glue (kept for compatibility)

// Your touch helper
#include "esp_lcd_touch_axs15231b.h" // bsp_touch_init/read/get_coordinates, touch_data_t

// ---------- Defaults / Pins ----------
#ifndef DISPLAY_WIDTH
  #define DISPLAY_WIDTH  320
#endif
#ifndef DISPLAY_HEIGHT
  #define DISPLAY_HEIGHT 480
#endif
#ifndef GPIO_BCKL
  #define GPIO_BCKL   6
#endif

#ifndef AXS15231B_SPI_BUS_PCLK
  #define AXS15231B_SPI_BUS_PCLK 5
#endif
#ifndef AXS15231B_SPI_BUS_DATA0
  #define AXS15231B_SPI_BUS_DATA0 1
#endif
#ifndef AXS15231B_SPI_BUS_DATA1
  #define AXS15231B_SPI_BUS_DATA1 2
#endif
#ifndef AXS15231B_SPI_BUS_DATA2
  #define AXS15231B_SPI_BUS_DATA2 3
#endif
#ifndef AXS15231B_SPI_BUS_DATA3
  #define AXS15231B_SPI_BUS_DATA3 4
#endif
#ifndef AXS15231B_SPI_CONFIG_CS
  #define AXS15231B_SPI_CONFIG_CS 12
#endif

// Try a fast panel clock, fall back safely:
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY 60000000UL  // 80 MHz
#endif
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY 50000000UL // 60 MHz
#endif
// #ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_TERTIARY
//   #define AXS15231B_SPI_CONFIG_PCLK_HZ_TERTIARY 40000000UL  // 52 MHz
// #endif
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK 40000000UL  // 40 MHz
#endif

#ifndef I2C_SDA
  #define I2C_SDA 8
#endif
#ifndef I2C_SCL
  #define I2C_SCL 7
#endif
#ifndef TCA_ADDR
  #define TCA_ADDR 0x20
#endif
#ifndef TCA_LCD_RSTBIT
  #define TCA_LCD_RSTBIT 1   // LCD RST on TCA (Waveshare Type-B)
#endif

#ifndef TOUCH_INT
  #define TOUCH_INT 16       // optional: keep line pulled up
#endif

#ifndef TOUCH_SWAP_XY
  #define TOUCH_SWAP_XY false
#endif
#ifndef TOUCH_MIRROR_X
  #define TOUCH_MIRROR_X false
#endif
#ifndef TOUCH_MIRROR_Y
  #define TOUCH_MIRROR_Y false
#endif

// ---- FULL-FRAME BUFFERS (RGB565) ----
#undef  LVGL_BUFFER_PIXELS
#define LVGL_BUFFER_PIXELS (DISPLAY_WIDTH * DISPLAY_HEIGHT)

// Force full-screen blits in flush for speed:
#ifndef FLUSH_ALWAYS_FULL
  #define FLUSH_ALWAYS_FULL 1
#endif

// Task / timing
#ifndef LVGL_TICK_HZ
  #define LVGL_TICK_HZ 1000  // 1kHz tick
#endif
#ifndef LVGL_HANDLER_PERIOD_MS
  #define LVGL_HANDLER_PERIOD_MS 8   // ~125 Hz handler
#endif
#ifndef LVGL_TASK_STACK
  #define LVGL_TASK_STACK 12288
#endif
#ifndef LVGL_TASK_PRIO
  #define LVGL_TASK_PRIO 2  // higher than default Arduino loop task
#endif
#ifndef LVGL_TASK_CORE
  #define LVGL_TASK_CORE 1  // pin to APP CPU on S3
#endif

// ---------- Module statics ----------
static Arduino_GFX *gfx = nullptr;
static lv_indev_t  *s_indev = nullptr;

static esp_timer_handle_t s_tick_timer = nullptr;
static TaskHandle_t       s_lvgl_task  = nullptr;

// ----------------- Backlight -----------------
// static void backlight_init()
// {
//   pinMode(DISPLAY_BCKL, OUTPUT);
//   // PWM for future dimming (full now)
//   ledcAttachPin(DISPLAY_BCKL, 1);
//   ledcSetup(1, 5000 /*Hz*/, 10 /*bits*/);
//   ledcWrite(1, (1 << 10) - 1); // 100%
// }

static inline void driver_set_backlight_pct(uint8_t pct /*0..100*/)
{
  if (pct > 100) pct = 100;
  smartdisplay_lcd_set_backlight(pct / 100.0f);
}

// ------------- TCA9554 reset pulse ------------
static void panel_reset_via_tca()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);  // fast I2C

  TCA9554 tca(TCA_ADDR);
  tca.begin();

  // Release expander pins HIGH
  for (uint8_t b = 0; b < 8; ++b) {
    tca.pinMode1(b, OUTPUT);
    tca.write1(b, 1);
  }
  delay(10);

  // Clean LCD RST pulse
  tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(180);
}

// ---------------- LVGL flush -> GFX ----------------
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
#if FLUSH_ALWAYS_FULL
  const int16_t x = 0, y = 0, w = DISPLAY_WIDTH, h = DISPLAY_HEIGHT;
#else
  const int16_t x = area->x1, y = area->y1;
  const int16_t w = area->x2 - area->x1 + 1;
  const int16_t h = area->y2 - area->y1 + 1;
#endif

#if LV_COLOR_16_SWAP
  gfx->draw16bitBeRGBBitmap(x, y, (uint16_t *)px_map, w, h);
#else
  gfx->draw16bitRGBBitmap(x, y, (uint16_t *)px_map, w, h);
#endif

  lv_display_flush_ready(disp);
}

// ---------------- LVGL touch read ----------------
static void lvgl_touch_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
  LV_UNUSED(indev_drv);

  touch_data_t td;
  bsp_touch_read();

  if (bsp_touch_get_coordinates(&td)) {
    int16_t x = td.coords[0].x;
    int16_t y = td.coords[0].y;

#if TOUCH_SWAP_XY
    int16_t tmp = x; x = y; y = tmp;
#endif
#if TOUCH_MIRROR_X
    x = DISPLAY_WIDTH - 1 - x;
#endif
#if TOUCH_MIRROR_Y
    y = DISPLAY_HEIGHT - 1 - y;
#endif

    data->state   = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

  // Prevent LVGL from polling multiple samples this cycle
  data->continue_reading = false;
}

// ----------- LVGL tick (1 kHz via esp_timer) -----------
static void tick_cb(void* /*arg*/)
{
  lv_tick_inc(1);
}

// ----------- LVGL handler task -----------
static void lvgl_task(void*) {
  const TickType_t period = pdMS_TO_TICKS(LVGL_HANDLER_PERIOD_MS);
  for (;;) {
    TickType_t t0 = xTaskGetTickCount();
    lv_timer_handler();
    TickType_t took = xTaskGetTickCount() - t0;
    vTaskDelay((took < period) ? (period - took) : 1);   // <-- ensures idle runs
  }
}

// --------------- Panel init helper ---------------
static bool panel_begin_with_fallbacks(Arduino_GFX* g)
{
  // Try fast to slow
  uint32_t tries[] = {
    AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY,
    AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY,
    // AXS15231B_SPI_CONFIG_PCLK_HZ_TERTIARY,
    AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK
  };

  for (uint32_t hz : tries) {
    if (g->begin(hz)) {
      Serial.printf("[PANEL] Began at %lu Hz\n", (unsigned long)hz);
      return true;
    }
    Serial.printf("[PANEL] Failed at %lu Hz, trying next...\n", (unsigned long)hz);
  }
  return false;
}

// --------------- SmartDisplay hook ---------------
extern "C" lv_display_t *lvgl_lcd_init(void)
{
  Serial.println("[PANEL] AXS15231B QSPI + LVGL FULL (perf-tuned)");

  // Hard reset via TCA expander
  panel_reset_via_tca();

  // Backlight on
  // backlight_init();
  // Hand backlight to SmartDisplay (no direct LEDC here)
  smartdisplay_lcd_set_backlight(1.0f);   // full on after panel is ready

  // Keep TP_INT sane
  pinMode(TOUCH_INT, INPUT_PULLUP);

  // QSPI -> panel
  Arduino_DataBus *bus = new Arduino_ESP32QSPI(
      AXS15231B_SPI_CONFIG_CS,
      AXS15231B_SPI_BUS_PCLK,
      AXS15231B_SPI_BUS_DATA0,
      AXS15231B_SPI_BUS_DATA1,
      AXS15231B_SPI_BUS_DATA2,
      AXS15231B_SPI_BUS_DATA3);

  gfx = new Arduino_AXS15231B(bus, -1 /*RST*/, 0 /*rot*/, false /*ips*/, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  if (!panel_begin_with_fallbacks(gfx)) {
    // Last-chance sanity
    gfx->begin(AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK);
  }
  gfx->fillScreen(0x0000);

  // ---- LVGL display setup ----
  lv_display_t *lvdisp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_color_format(lvdisp, LV_COLOR_FORMAT_RGB565);

  // Buffers: try one in INTERNAL (faster DMA/cache), the other in PSRAM
  const size_t buf_bytes = LVGL_BUFFER_PIXELS * sizeof(lv_color_t);

  lv_color_t *buf1 = (lv_color_t*) heap_caps_malloc(
      buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);
  if (!buf1) {
    buf1 = (lv_color_t*) heap_caps_malloc(
      buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);
  }

  lv_color_t *buf2 = (lv_color_t*) heap_caps_malloc(
      buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);

  if (buf1 && buf2) {
    lv_display_set_buffers(lvdisp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
    Serial.println("[LVGL] Double buffer (FULL) set");
  } else if (buf1) {
    lv_display_set_buffers(lvdisp, buf1, nullptr, buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
    Serial.println("[LVGL] Single buffer (FULL) set — consider freeing RAM to enable double buffering");
  } else {
    // Emergency tiny partial buffer to avoid crash (not ideal for your panel, but fail-safe)
    const size_t tiny_bytes = DISPLAY_WIDTH * 20 * sizeof(lv_color_t);
    lv_color_t *tiny = (lv_color_t*) heap_caps_malloc(
        tiny_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);
    lv_display_set_buffers(lvdisp, tiny, nullptr, tiny_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("[LVGL] Fallback to small PARTIAL buffer due to memory pressure");
  }

  // Flush callback
  lv_display_set_flush_cb(lvdisp, lvgl_flush_cb);
  lv_display_set_default(lvdisp);

  // Root screen opaque to avoid extra blending work
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  // --------- TOUCH INIT ----------
  // IMPORTANT: second arg is reset GPIO, NOT INT pin
  bsp_touch_init(&Wire, -1 /* rst_gpio none */, 0 /* addr/unused */, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  // Register pointer input
  s_indev = lv_indev_create();
  lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(s_indev, lvgl_touch_read_cb);
  lv_indev_set_disp(s_indev, lvdisp);
  indev = s_indev; // keep external reference compatibility

  // --------- LVGL tick + handler task ----------
  if (!s_tick_timer) {
    const esp_timer_create_args_t targs = {
      .callback = &tick_cb,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lv_tick_1k"
    };
    esp_timer_create(&targs, &s_tick_timer);
    esp_timer_start_periodic(s_tick_timer, 1000000UL / LVGL_TICK_HZ);
  }

  if (!s_lvgl_task) {
    xTaskCreatePinnedToCore(
      lvgl_task, "lvgl_task",
      LVGL_TASK_STACK, nullptr,
      LVGL_TASK_PRIO, &s_lvgl_task,
      LVGL_TASK_CORE);
  }

  Serial.println("[LVGL] Driver init complete");
  return lvdisp;
}

// Optional: expose a dimmer if you want later
// void display_set_backlight(uint8_t pct /*0..100*/)
// {
//   pct = (pct > 100) ? 100 : pct;
//   const uint32_t max = (1 << 10) - 1;
//   ledcWrite(1, (max * pct) / 100);
// }
void display_set_backlight(uint8_t pct /*0..100*/)
{
  if (pct > 100) pct = 100;
  smartdisplay_lcd_set_backlight(pct / 100.0f);
}