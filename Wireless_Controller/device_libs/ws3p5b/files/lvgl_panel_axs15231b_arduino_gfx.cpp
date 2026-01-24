// drv_axs15231b_lvgl.cpp - OPTIMIZED FULL FRAME RENDER VERSION
// ESP32-S3 + AXS15231B (QSPI) + LVGL 9 + Arduino_GFX
//
// OPTIMIZATIONS:
// - 50MHz stable SPI clock
// - 32-byte aligned buffers for optimal DMA
// - Smart frame skipping (disabled during motion)
// - Transaction batching with startWrite/endWrite
// - Follows manufacturer pattern (no separate LVGL task)

#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "TCA9554.h"
#include <esp32-hal-ledc.h>

void set_brightness(float level);         // from board_driver_util.cpp
void display_set_backlight(uint8_t pct);  // optional
void Set_Backlight(uint8_t Light);        // from LVGL_Driver.cpp

#include "esp_lcd_touch_axs15231b.h"      // bsp_touch_init/read/get_coordinates, touch_data_t

// ---------- Configuration ----------
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

// OPTIMIZED: Stable high-speed clocks
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY 60000000UL
#endif
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY 40000000UL
#endif
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK
  #define AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK 30000000UL
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
  #define TCA_LCD_RSTBIT 1
#endif

#ifndef TOUCH_INT
  #define TOUCH_INT 16
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

// ========== BUFFER SETTINGS FOR FULL REFRESH ==========
// Bytes per pixel for RGB565
#define BYTES_PER_PIXEL      2

// Allow overriding the effective buffer size from build flags or headers.
// By default we use a full-frame buffer (required by the AXS15231B).
#ifndef LVGL_BUFFER_PIXELS
  #define LVGL_BUFFER_PIXELS   (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#endif

#ifndef LVGL_BUFFER_BYTES
  #define LVGL_BUFFER_BYTES    (LVGL_BUFFER_PIXELS * BYTES_PER_PIXEL)
#endif

// Toggle double vs single buffering for full-frame rendering.
// 1 = double buffer (two full-frame buffers, smoother, more RAM)
// 0 = single buffer  (one full-frame buffer, less RAM, higher tear risk)
#ifndef LVGL_USE_DOUBLE_BUFFER
  #define LVGL_USE_DOUBLE_BUFFER  1
#endif

// Sampling stride for the frame checksum used by frame skipping.
// Larger values = fewer samples (faster, less sensitive).
#ifndef FRAMECHECK_SAMPLE_STRIDE
  #define FRAMECHECK_SAMPLE_STRIDE 128
#endif

// OPTIMIZED: Smart frame skipping
#ifndef ENABLE_FRAME_SKIP
  #define ENABLE_FRAME_SKIP 0
#endif

// Motion detection timeout (ms)
#define MOTION_TIMEOUT_MS 150

// ---------- Module statics ----------
static Arduino_GFX *gfx = nullptr;
extern "C" lv_indev_t *indev;

// OPTIMIZED: Smart frame skip with motion detection
#if ENABLE_FRAME_SKIP
static uint32_t s_last_frame_checksum = 0;
static uint32_t s_last_motion_time = 0;
static volatile bool s_is_animating = false;

// Ultra-fast checksum (samples every FRAMECHECK_SAMPLE_STRIDE pixels)
static inline uint32_t calculate_frame_checksum(const uint16_t *data, size_t len)
{
  uint32_t sum = 0;
  for (size_t i = 0; i < len; i += FRAMECHECK_SAMPLE_STRIDE) {
    sum = (sum << 5) - sum + data[i];  // sum * 31 + data[i]
  }
  return sum;
}
#endif

// ----------------- Backlight helper -----------------
// static inline void driver_set_backlight_pct(uint8_t pct)
// {
//   if (pct > 100) pct = 100;
//   Set_Backlight(pct);
// }

// ------------- TCA9554 reset pulse ------------
static void panel_reset_via_tca()
{
  // I2C_Init() (in LVGL_Driver.cpp) and/or PMU_init() should have
  // configured Wire and set the clock already. Just use the bus.
  TCA9554 tca(TCA_ADDR);
  if (!tca.begin()) {
    Serial.println("[PANEL] TCA9554 init failed, skipping LCD reset");
    return;
  }

  // If you really only care about the reset bit, you can do just that:
  tca.pinMode1(TCA_LCD_RSTBIT, OUTPUT);
  tca.write1(TCA_LCD_RSTBIT, 1);  // default high

  delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(180);
}

// ---------------- LVGL tick callback ----------------
static uint32_t millis_cb(void)
{
  return millis();
}

// ---------------- OPTIMIZED LVGL flush -> GFX ----------------
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  if (!gfx || !px_map) {
    lv_display_flush_ready(disp);
    return;
  }

  uint16_t *pixels = (uint16_t *)px_map;

#if ENABLE_FRAME_SKIP
  uint32_t now = millis();
  
  // Update animation state
  s_is_animating = (now - s_last_motion_time) < MOTION_TIMEOUT_MS;
  
  // Only skip frames when idle (no scrolling/animation)
  if (!s_is_animating) {
    uint32_t checksum = calculate_frame_checksum(pixels, LVGL_BUFFER_PIXELS);
    if (checksum == s_last_frame_checksum) {
      lv_display_flush_ready(disp);
      return;  // Skip identical frame
    }
    s_last_frame_checksum = checksum;
  } else {
    // Update checksum during animation
    s_last_frame_checksum = calculate_frame_checksum(pixels, LVGL_BUFFER_PIXELS);
  }
#endif

  // OPTIMIZED: Match manufacturer pattern but with transaction batching
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->startWrite();
  
#if LV_COLOR_16_SWAP
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, pixels, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, pixels, w, h);
#endif
  
  gfx->endWrite();

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
    
#if ENABLE_FRAME_SKIP
    // Mark motion detected - disables frame skipping during scrolling
    s_last_motion_time = millis();
#endif
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

  data->continue_reading = false;
}

// --------------- Panel init helper ---------------
static bool panel_begin_with_fallbacks(Arduino_GFX* g)
{
  if (g->begin(AXS15231B_SPI_CONFIG_PCLK_HZ_PRIMARY))   return true;
  Serial.println("[PANEL] Primary speed failed, trying secondary...");
  if (g->begin(AXS15231B_SPI_CONFIG_PCLK_HZ_SECONDARY)) return true;
  Serial.println("[PANEL] Secondary speed failed, trying fallback...");
  return g->begin(AXS15231B_SPI_CONFIG_PCLK_HZ_FALLBACK);
}

// --------------- OPTIMIZED Main LVGL init ---------------
// Note: No separate LVGL task - call lvgl_loop() from your main loop
extern "C" lv_display_t *lvgl_lcd_init_perf(void)
{
  // Hardware init
  panel_reset_via_tca();
  set_brightness(1.0f);
  pinMode(TOUCH_INT, INPUT_PULLUP);

  // QSPI bus - match manufacturer's initialization
  Arduino_DataBus *bus = new Arduino_ESP32QSPI(
      AXS15231B_SPI_CONFIG_CS,
      AXS15231B_SPI_BUS_PCLK,
      AXS15231B_SPI_BUS_DATA0,
      AXS15231B_SPI_BUS_DATA1,
      AXS15231B_SPI_BUS_DATA2,
      AXS15231B_SPI_BUS_DATA3);

  gfx = new Arduino_AXS15231B(bus, -1, 0, false, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  if (!panel_begin_with_fallbacks(gfx)) {
    Serial.println("[PANEL] FATAL: Display initialization failed!");
    return nullptr;
  }

  Serial.println("[PANEL] Display initialized successfully");
  gfx->fillScreen(0x0000);  // Clear to black

  // Touch init
  bsp_touch_init(&Wire, -1, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  // LVGL core init (should be called before this function in your main code)
  // lv_init() should already be done

  // LVGL tick source
  lv_tick_set_cb(millis_cb);

  // LVGL display object
  lv_display_t *lvdisp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_physical_resolution(lvdisp, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_color_format(lvdisp, LV_COLOR_FORMAT_RGB565);

  // OPTIMIZED: 32-byte aligned buffers in SPIRAM (matching manufacturer but optimized)
  lv_color_t *buf1 = (lv_color_t *)heap_caps_aligned_alloc(
      32,  // 32-byte alignment for DMA
      LVGL_BUFFER_BYTES,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  lv_color_t *buf2 = nullptr;

#if LVGL_USE_DOUBLE_BUFFER
  if (buf1) {
    buf2 = (lv_color_t *)heap_caps_aligned_alloc(
        32,
        LVGL_BUFFER_BYTES,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
#endif

  if (!buf1) {
    Serial.println("[LVGL] CRITICAL: Primary buffer allocation failed!");
    return nullptr;
  }

  if (buf2) {
    lv_display_set_buffers(
        lvdisp,
        buf1,
        buf2,
        LVGL_BUFFER_BYTES,
        LV_DISPLAY_RENDER_MODE_FULL);
    Serial.printf("[LVGL] Double-buffered: %u bytes each (FULL mode, 32-byte aligned)\n",
                  (unsigned)LVGL_BUFFER_BYTES);
  } else {
    lv_display_set_buffers(
        lvdisp,
        buf1,
        nullptr,
        LVGL_BUFFER_BYTES,
        LV_DISPLAY_RENDER_MODE_FULL);
    Serial.printf("[LVGL] Single-buffered: %u bytes (FULL mode, 32-byte aligned)\n",
                  (unsigned)LVGL_BUFFER_BYTES);
  }

  // Flush callback
  lv_display_set_flush_cb(lvdisp, lvgl_flush_cb);
  lv_display_set_default(lvdisp);

  // Screen background
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  // Touch input device
  lv_indev_t *s_indev = lv_indev_create();
  lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(s_indev, lvgl_touch_read_cb);
  lv_indev_set_disp(s_indev, lvdisp);
  indev = s_indev;

  // Force initial refresh
  lv_obj_invalidate(scr);
  lv_refr_now(lvdisp);

  Serial.println("[LVGL] Initialization complete (optimized, no separate task)");
  Serial.printf("[LVGL] Smart frame skip: %s (auto-disabled during scrolling)\n", 
                ENABLE_FRAME_SKIP ? "ENABLED" : "DISABLED");
  Serial.println("[LVGL] Call lv_timer_handler() from your main loop");
  
  return lvdisp;
}

// --------------- LVGL loop function ---------------
// Call this from your main loop() or from a task
// Manufacturer pattern: call every 5ms with adaptive timing when animating
extern "C" void lvgl_loop(void)
{
  lv_timer_handler();
  
#if ENABLE_FRAME_SKIP
  // Adaptive delay: faster during scrolling/animation, slower when idle
  if (s_is_animating) {
    vTaskDelay(pdMS_TO_TICKS(0));  // 2ms when animating (~500Hz)
  } else {
    vTaskDelay(pdMS_TO_TICKS(2));  // 5ms when idle (~200Hz)
  }
#else
  vTaskDelay(pdMS_TO_TICKS(0));    // 5ms constant like manufacturer
#endif
}
