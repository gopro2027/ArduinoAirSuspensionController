// lvgl_panel_st7796_ft6336.cpp
// ESP32-S3 + ST7796 (SPI) + TCA9554 reset + Arduino_GFX + LVGL 9 + FT6336
// Performance-oriented PARTIAL mode (bigger buffer) + clean init

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>

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

// Created once
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX     *gfx = nullptr;
static TCA9554          tca(TCA_ADDR);

// indev is defined elsewhere (board_driver_util.cpp)
extern "C" lv_indev_t *indev;

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

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

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
  if (!gfx) gfx = new Arduino_ST7796(bus, LCD_RST, 0, false, LCD_WIDTH, LCD_HEIGHT);

  Serial.println("[PANEL] gfx->begin()");
  if (!gfx->begin(100000000)) {
    Serial.println("[PANEL] WARNING: gfx->begin() failed");
  }

  gfx->displayOn();
  delay(10);

  // If your colors are correct now, keep your working invert state.
  // Only change this if you *know* you need it.
  gfx->invertDisplay(true);

  pinMode(GPIO_BCKL, OUTPUT);
  digitalWrite(GPIO_BCKL, HIGH);

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
//   vTaskDelay(pdMS_TO_TICKS(0));
}
