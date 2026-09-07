#pragma once
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"

#include "CH422G.h"

/*
 * ESP32-S3-Touch-LCD-4.3 -- 800x480 IPS on an ST7262 RGB driver.
 *
 * Unlike the ST7701 on the 2.8b, the ST7262 has NO command interface: there is no 3-wire SPI
 * init sequence and no MADCTL, so there is nothing to configure beyond the RGB timing and a
 * reset pulse. That also means the panel cannot rotate itself -- SUPPORTS_ROTATION is 0 and the
 * UI runs in the panel's native landscape (see device_lib_exports.h).
 *
 * Timings and pin map are taken from Waveshare's own board file for this product
 * (esp_panel_board_custom_conf.h, ESP32-S3-Touch-LCD-4.3 demo) and cross-checked against
 * ESP32-S3-Touch-LCD-4.3-Sch.pdf.
 */

#define ESP_PANEL_LCD_WIDTH                       (800)
#define ESP_PANEL_LCD_HEIGHT                      (480)

#define ESP_PANEL_LCD_RGB_TIMING_FREQ_HZ          (16 * 1000 * 1000)
#define ESP_PANEL_LCD_RGB_TIMING_HPW              (4)
#define ESP_PANEL_LCD_RGB_TIMING_HBP              (8)
#define ESP_PANEL_LCD_RGB_TIMING_HFP              (8)
#define ESP_PANEL_LCD_RGB_TIMING_VPW              (4)
#define ESP_PANEL_LCD_RGB_TIMING_VBP              (8)
#define ESP_PANEL_LCD_RGB_TIMING_VFP              (8)
// This panel latches on the FALLING edge -- opposite of the 2.8b. Getting it wrong shows up as a
// washed-out / smeared image rather than a blank screen, so it is easy to miss.
#define ESP_PANEL_LCD_RGB_PCLK_ACTIVE_NEG         (1)
#define ESP_PANEL_LCD_RGB_DATA_WIDTH              (16)
#define ESP_PANEL_LCD_RGB_PIXEL_BITS              (16)
// Single frame buffer (Waveshare's own config for this board). With LVGL rendering PARTIAL
// strips, a second frame buffer would mean each flush lands in whichever buffer is currently
// off-screen, so half the dirty rectangles would show up a frame late (visible flicker on
// scroll). One FB + a bounce buffer is the artifact-free combination here.
// ; was, on the 2.8b: num_fbs 2 / double_fb true
#define ESP_PANEL_LCD_RGB_FRAME_BUF_NUM           (1)
// Bounce buffer in internal SRAM, in PIXELS. The panel DMAs from PSRAM through this, which is
// what stops the "screen drift" tearing when PSRAM is momentarily busy. Costs
// 800*10*2 bytes * 2 buffers = 32 KB of internal DMA RAM.
#define ESP_PANEL_LCD_RGB_BOUNCE_BUF_SIZE         (10 * ESP_PANEL_LCD_WIDTH)

// for oasman
#define LCD_WIDTH ESP_PANEL_LCD_WIDTH
#define LCD_HEIGHT ESP_PANEL_LCD_HEIGHT

////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// RGB pin map -- ESP32-S3-Touch-LCD-4.3 only /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

#define ESP_PANEL_LCD_PIN_NUM_RGB_HSYNC           (46)
#define ESP_PANEL_LCD_PIN_NUM_RGB_VSYNC           (3)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DE              (5)
#define ESP_PANEL_LCD_PIN_NUM_RGB_PCLK            (7)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DISP            (-1)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA0           (14)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA1           (38)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA2           (18)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA3           (17)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA4           (10)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA5           (39)
// GPIO0 is an RGB data line on this board, NOT a usable button. See USE_BOOT_BUTTON_FUNCTIONALITY in
// device_lib_exports.h -- reading it at runtime both breaks the green channel and returns
// garbage that would look like button presses.
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA6           (0)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA7           (45)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA8           (48)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA9           (47)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA10          (21)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA11          (1)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA12          (2)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA13          (42)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA14          (41)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA15          (40)

extern esp_lcd_panel_handle_t panel_handle;
extern uint8_t LCD_Backlight;

void ST7262_Init();

void LCD_Init();
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint8_t *color);

// Backlight. NOTE: on/off only -- see Set_Backlight() in the .cpp.
void Backlight_Init();
void Set_Backlight(uint8_t Light);
