#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "TCA9554.h"

extern "C" lv_indev_t *indev;   // declared/owned by esp32_smartdisplay.c

// Your touch helper in src/
#include "esp_lcd_touch_axs15231b.h"  // bsp_touch_init/read/get_coordinates, touch_data_t

// ---------- Defaults ----------
#ifndef DISPLAY_WIDTH
  #define DISPLAY_WIDTH  320
#endif
#ifndef DISPLAY_HEIGHT
  #define DISPLAY_HEIGHT 480
#endif
#ifndef DISPLAY_BCKL
  #define DISPLAY_BCKL   6
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
#ifndef AXS15231B_SPI_CONFIG_PCLK_HZ
  #define AXS15231B_SPI_CONFIG_PCLK_HZ 40000000
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

// Optional: if your board routes TP_INT to a pin (typ. GPIO16) we can enable pull-up.
// The touch helper will still poll; this just keeps the line sane.
#ifndef TOUCH_INT
  #define TOUCH_INT 16
#endif

// Optional touch orientation tweaks
#ifndef TOUCH_SWAP_XY
  #define TOUCH_SWAP_XY false
#endif
#ifndef TOUCH_MIRROR_X
  #define TOUCH_MIRROR_X false
#endif
#ifndef TOUCH_MIRROR_Y
  #define TOUCH_MIRROR_Y false
#endif

// ---- FORCE FULL-FRAME BUFFERS (RGB565) ----
#undef  LVGL_BUFFER_PIXELS
#define LVGL_BUFFER_PIXELS (DISPLAY_WIDTH * DISPLAY_HEIGHT)
// -------------------------------------------

static Arduino_GFX *gfx = nullptr;

/* ----------------- Backlight ----------------- */
static void backlight_init()
{
  pinMode(DISPLAY_BCKL, OUTPUT);
  ledcAttachPin(DISPLAY_BCKL, 1);
  ledcSetup(1, 5000 /*Hz*/, 10 /*bits*/);
  ledcWrite(1, (1 << 10) - 1); // 100%
}

/* ------------- TCA9554 reset pulse ------------ */
static void panel_reset_via_tca()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  TCA9554 tca(TCA_ADDR);
  tca.begin();

  // Release all expander pins HIGH so nothing else (e.g. TP_RST) is held low.
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

/* ---------------- LVGL flush -> GFX ---------------- */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  const int16_t x = area->x1;
  const int16_t y = area->y1;
  const int16_t w = area->x2 - area->x1 + 1;
  const int16_t h = area->y2 - area->y1 + 1;

#if LV_COLOR_16_SWAP
  gfx->draw16bitBeRGBBitmap(x, y, (uint16_t *)px_map, w, h);
#else
  gfx->draw16bitRGBBitmap(x, y, (uint16_t *)px_map, w, h);
#endif
  lv_display_flush_ready(disp);
}

/* ---------------- LVGL touch read ---------------- */
static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  LV_UNUSED(indev);

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

    // // uncomment for quick logs
    // static uint32_t last;
    // if (millis() - last > 100) { Serial.printf("[TOUCH] %d,%d\n", x, y); last = millis(); }
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* --------------- SmartDisplay hook --------------- */
extern "C" lv_display_t *lvgl_lcd_init(void)
{
  Serial.println("[PANEL] Using Arduino_GFX AXS15231B panel + touch");

  // Hard reset panel via TCA expander
  panel_reset_via_tca();

  // BL on
  backlight_init();

  // Keep TP_INT sane (not required, but helps if it floats)
  pinMode(TOUCH_INT, INPUT_PULLUP);

  // QSPI bus -> panel
  Arduino_DataBus *bus = new Arduino_ESP32QSPI(
      AXS15231B_SPI_CONFIG_CS,
      AXS15231B_SPI_BUS_PCLK,
      AXS15231B_SPI_BUS_DATA0,
      AXS15231B_SPI_BUS_DATA1,
      AXS15231B_SPI_BUS_DATA2,
      AXS15231B_SPI_BUS_DATA3);

  gfx = new Arduino_AXS15231B(bus, -1 /*RST*/, 0 /*rot*/, false /*ips*/, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  if (!gfx->begin(AXS15231B_SPI_CONFIG_PCLK_HZ)) {
    gfx->begin(26000000);
  }
  gfx->fillScreen(0x0000);

  // LVGL display + full-frame double buffers in PSRAM
  lv_display_t *lvdisp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_color_format(lvdisp, LV_COLOR_FORMAT_RGB565);

  const size_t buf_bytes = LVGL_BUFFER_PIXELS * sizeof(lv_color_t);
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!buf1) {
    const size_t tiny_bytes = DISPLAY_WIDTH * 20 * sizeof(lv_color_t);
    lv_color_t *tiny = (lv_color_t *)heap_caps_malloc(tiny_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lv_display_set_buffers(lvdisp, tiny, nullptr, tiny_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  } else if (!buf2) {
    lv_display_set_buffers(lvdisp, buf1, nullptr, buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
  } else {
    lv_display_set_buffers(lvdisp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
  }

  lv_display_set_flush_cb(lvdisp, lvgl_flush_cb);
  lv_display_set_default(lvdisp);

  // --------- TOUCH INIT (IMPORTANT: correct signature!) ----------
  // DO NOT pass TOUCH_INT here — second arg is **reset GPIO**, not INT.
  // Your working test used: bsp_touch_init(&Wire, -1, 0, 320, 480);
  bsp_touch_init(&Wire, -1 /* rst_gpio none */, 0 /* addr/unused */, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  // Register a single LVGL pointer indev
  lv_indev_t *my_indev = lv_indev_create();
  lv_indev_set_type(my_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(my_indev, lvgl_touch_read_cb);
  lv_indev_set_disp(my_indev, lvdisp);

indev = my_indev;

  return lvdisp;
}
