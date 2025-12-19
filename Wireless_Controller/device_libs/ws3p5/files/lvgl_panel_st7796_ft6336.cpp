// lvgl_panel_st7796_ft6336.cpp
// ESP32-S3 + ST7796 (SPI) + TCA9554 reset + Arduino_GFX + LVGL 9 + FT6336
// Performance-oriented PARTIAL mode (bigger buffer) + clean init
// Added: Waveshare vendor-specific ST7796 init + tunable software color correction.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>
#include <math.h>

#include "TCA9554.h"
#include "esp_lcd_touch_ft6336.h"

#ifndef GPIO_BCKL
  #define GPIO_BCKL 6
#endif

#define SPI_MISO    2
#define SPI_MOSI    1
#define SPI_SCLK    5

#define LCD_CS     -1
#define LCD_DC      3
#define LCD_RST    -1

#define LCD_WIDTH   320
#define LCD_HEIGHT  480

#define I2C_SDA     8
#define I2C_SCL     7

#define TCA_ADDR        0x20
#define TCA_LCD_RSTBIT  1

// Partial buffer size tuned for internal RAM on Arduino_GFX 1.5.5
#define LVGL_BUFFER_LINES   480
#define LVGL_BUFFER_PIXELS (LCD_WIDTH * LVGL_BUFFER_LINES)

// ---------- Tunable ST7796 color correction ----------
// Set to 0 to disable all software correction quickly.

#define ST_ENABLE_COLOR_CORRECTION 1

// ----------------------------------------------------

// Created once
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX     *gfx = nullptr;
static TCA9554          tca(TCA_ADDR);

// indev is defined elsewhere (board_driver_util.cpp)
extern "C" lv_indev_t *indev;

#if ST_ENABLE_COLOR_CORRECTION
// 8-bit per-channel LUTs, built once at runtime.
static bool    s_cc_lut_built = false;
static uint8_t s_r_lut[256];
static uint8_t s_g_lut[256];
static uint8_t s_b_lut[256];

// Overall brightness scaling (applied before gamma). 1.0 = no change.
#define ST_GLOBAL_BRIGHT  1.00f

// Global gamma for all channels so greys stay neutral
#define ST_GLOBAL_GAMMA  1.0f

// Small per-channel gains to fine-tune tint (1.0 = no change)
#define ST_R_GAIN  1.00f
#define ST_G_GAIN  1.00f
#define ST_B_GAIN  1.00f

static void st_build_cc_luts()
{
  if (s_cc_lut_built) return;

  for (int i = 0; i < 256; ++i) {
    float x = (float)i / 255.0f;

    // Apply global brightness first
    float v = x * ST_GLOBAL_BRIGHT;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    // Global gamma (same for R/G/B keeps greys neutral)
    v = powf(v, ST_GLOBAL_GAMMA);
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    // Per-channel gains (small, linear tweaks)
    float r = v * ST_R_GAIN;
    float g = v * ST_G_GAIN;
    float b = v * ST_B_GAIN;

    if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f; if (b > 1.0f) b = 1.0f;

    s_r_lut[i] = (uint8_t)(r * 255.0f + 0.5f);
    s_g_lut[i] = (uint8_t)(g * 255.0f + 0.5f);
    s_b_lut[i] = (uint8_t)(b * 255.0f + 0.5f);
  }

  s_cc_lut_built = true;
}

// Apply correction to a single RGB565 pixel
static inline uint16_t st_apply_cc_565(uint16_t c)
{
  uint8_t r5 = (c >> 11) & 0x1F;
  uint8_t g6 = (c >> 5)  & 0x3F;
  uint8_t b5 =  c        & 0x1F;

  // Expand to 8-bit
  uint8_t r = (r5 * 527 + 23) >> 6;
  uint8_t g = (g6 * 259 + 33) >> 6;
  uint8_t b = (b5 * 527 + 23) >> 6;

  // LUT corrected
  r = s_r_lut[r];
  g = s_g_lut[g];
  b = s_b_lut[b];

  // Back to 5/6/5
  r5 = (r * 31 + 127) / 255;
  g6 = (g * 63 + 127) / 255;
  b5 = (b * 31 + 127) / 255;

  return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}
#endif // ST_ENABLE_COLOR_CORRECTION


// ---- Waveshare ST7796 vendor init cloned from ESP-IDF driver ----
// Uses the *bus* object for low-level writes.

static void st7796_ws_send_cmd(uint8_t cmd, const uint8_t *data, uint8_t len, uint16_t delay_ms)
{
  bus->beginWrite();
  bus->writeCommand(cmd);

  if (data && len) {
    for (uint8_t i = 0; i < len; ++i) {
      bus->write(data[i]);  // single byte
    }
  }

  bus->endWrite();

  if (delay_ms) {
    delay(delay_ms);
  }
}

static void st7796_ws_apply_vendor_init()
{
  // Interface pixel format (16-bit)
  {
    const uint8_t d3A[] = { 0x05 };
    st7796_ws_send_cmd(0x3A, d3A, sizeof(d3A), 0);
  }

  // Command access enable sequence
  {
    const uint8_t dF0_1[] = { 0xC3 };
    const uint8_t dF0_2[] = { 0x96 };
    st7796_ws_send_cmd(0xF0, dF0_1, sizeof(dF0_1), 0);
    st7796_ws_send_cmd(0xF0, dF0_2, sizeof(dF0_2), 0);
  }

  // Power / frame / voltage settings
  {
    const uint8_t dB4[] = { 0x01 };
    const uint8_t dB7[] = { 0xC6 };
    const uint8_t dC0[] = { 0x80, 0x45 };
    const uint8_t dC1[] = { 0x13 };
    const uint8_t dC2[] = { 0xA7 };
    const uint8_t dC5[] = { 0x0A };
    st7796_ws_send_cmd(0xB4, dB4, sizeof(dB4), 0);
    st7796_ws_send_cmd(0xB7, dB7, sizeof(dB7), 0);
    st7796_ws_send_cmd(0xC0, dC0, sizeof(dC0), 0);
    st7796_ws_send_cmd(0xC1, dC1, sizeof(dC1), 0);
    st7796_ws_send_cmd(0xC2, dC2, sizeof(dC2), 0);
    st7796_ws_send_cmd(0xC5, dC5, sizeof(dC5), 0);
  }

  // Display-related tweaks
  {
    const uint8_t dE8[] = { 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33 };
    st7796_ws_send_cmd(0xE8, dE8, sizeof(dE8), 0);
  }

  // Positive gamma correction
  {
    const uint8_t dE0[] = {
      0xD0, 0x08, 0x0F, 0x06, 0x06, 0x33, 0x30, 0x33,
      0x47, 0x17, 0x13, 0x13, 0x2B, 0x31
    };
    st7796_ws_send_cmd(0xE0, dE0, sizeof(dE0), 0);
  }

  // Negative gamma correction
  {
    const uint8_t dE1[] = {
      0xD0, 0x0A, 0x11, 0x0B, 0x09, 0x07, 0x2F, 0x33,
      0x47, 0x38, 0x15, 0x16, 0x2C, 0x32
    };
    st7796_ws_send_cmd(0xE1, dE1, sizeof(dE1), 0);
  }

  // Finalize access sequence
  {
    const uint8_t dF0_3[] = { 0x3C };
    const uint8_t dF0_4[] = { 0x69 };
    st7796_ws_send_cmd(0xF0, dF0_3, sizeof(dF0_3), 0);
    st7796_ws_send_cmd(0xF0, dF0_4, sizeof(dF0_4), 0);
  }

  // Waveshare enables display inversion (INVON)
  st7796_ws_send_cmd(0x21, nullptr, 0, 0);
}

// ----------------------------------------------------

static bool lcd_reset_via_tca(void)
{
  Serial.println("[PANEL] Reset via TCA9554");

  if (!tca.begin()) {
    Serial.println("[PANEL] TCA begin failed");
    return false;
  }

  tca.pinMode1(TCA_LCD_RSTBIT, OUTPUT);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 0); delay(10);
  tca.write1(TCA_LCD_RSTBIT, 1); delay(200);

  Serial.println("[PANEL] Reset complete");
  return true;
}

static uint32_t lvgl_tick_cb(void) { return millis(); }

// LVGL -> panel flush
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
  const uint32_t count = w * h;

  uint16_t *src = (uint16_t *)px_map;

#if ST_ENABLE_COLOR_CORRECTION
  st_build_cc_luts();

  // In-place color correction
  for (uint32_t i = 0; i < count; ++i) {
    src[i] = st_apply_cc_565(src[i]);
  }
#endif

  gfx->draw16bitRGBBitmap(area->x1, area->y1, src, w, h);
  lv_display_flush_ready(disp);
}

static void lvgl_touch_read_cb(lv_indev_t *, lv_indev_data_t *data)
{
  touch_data_t touch;

  if (bsp_touch_get_coordinates(&touch) && touch.touch_num > 0) {
    data->state   = LV_INDEV_STATE_PRESSED;
    data->point.x = touch.coords[0].x;
    data->point.y = touch.coords[0].y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

  data->continue_reading = false;
}

extern "C" lv_display_t *lvgl_lcd_init_perf(void)
{
  Serial.println("[LVGL_INIT] ST7796 init (PARTIAL perf)");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  if (!lcd_reset_via_tca()) {
    Serial.println("[PANEL] Reset failed, aborting init");
    return nullptr;
  }

  if (!bus) {
    bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, SPI_SCLK, SPI_MOSI, SPI_MISO);
  }

  if (!gfx) {
    // IPS = true (per Waveshare docs)
    gfx = new Arduino_ST7796(bus, LCD_RST, 0 /* rotation */, true /* IPS */, LCD_WIDTH, LCD_HEIGHT);
  }

  Serial.println("[PANEL] gfx->begin()");
  if (!gfx->begin(80000000)) {
    Serial.println("[PANEL] WARNING: gfx->begin() failed");
  }

  gfx->displayOn();
  delay(10);

  // Apply Waveshare's vendor-specific tuning (gamma, power, inversion).
  st7796_ws_apply_vendor_init();

  // No gfx->invertDisplay() here; inversion is set by 0x21 above.

  lv_tick_set_cb(lvgl_tick_cb);

  const size_t buf_size = LVGL_BUFFER_PIXELS * sizeof(lv_color_t);

  // 1) Best: internal DMA
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  if (!buf2) {
    Serial.println("[LVGL] buf2 internal alloc failed, using single buffer");
  }

  // 2) Fallback to PSRAM only if buf1 is missing
  if (!buf1) {
    buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf2) buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }

  // 3) Last fallback
  if (!buf1) buf1 = (lv_color_t *)malloc(buf_size);
  if (!buf2 && buf1) buf2 = (lv_color_t *)malloc(buf_size);

  if (!buf1) {
    Serial.println("[LVGL] CRITICAL: buffer alloc failed");
    return nullptr;
  }

  lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, lvgl_flush_cb);

  if (buf2) {
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.printf("[LVGL] Double-buffer PARTIAL, lines=%u\n", (unsigned)LVGL_BUFFER_LINES);
  } else {
    lv_display_set_buffers(disp, buf1, nullptr, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.printf("[LVGL] Single-buffer PARTIAL, lines=%u\n", (unsigned)LVGL_BUFFER_LINES);
  }

  lv_display_set_default(disp);

  Serial.println("[TOUCH] Initializing FT6336...");
  bsp_touch_init(&Wire, -1, 0, LCD_WIDTH, LCD_HEIGHT);

  lv_indev_t *touch_indev = lv_indev_create();
  lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touch_indev, lvgl_touch_read_cb);
  lv_indev_set_disp(touch_indev, disp);

  indev = touch_indev;

  Serial.println("[LVGL] FT6336 touch registered");
  Serial.println("[LVGL_INIT] Display ready");
  return disp;
}

extern "C" void lvgl_loop(void)
{
  lv_timer_handler();
  // vTaskDelay(pdMS_TO_TICKS(0));
}
