#pragma once
#include <Arduino.h>
#include "esp_lcd_panel_ops.h"

#define LCD_WIDTH   466
#define LCD_HEIGHT  466

#define LCD_BIT_PER_PIXEL  16

/** Column window is 0x06..0x1D7 on this panel, so every draw is offset by 6 px in x. */
#define CO5300_X_GAP 6
#define CO5300_Y_GAP 0

/** Must match LVGL partial strip height (LVGL_Driver.h). SPI max_transfer_sz drives internal DMA priv buffer sizing — do not use full-frame size here. */
#define CO5300_LVGL_PARTIAL_LINES 50
#define CO5300_MAX_TRANSFER_SZ ((LCD_WIDTH) * (CO5300_LVGL_PARTIAL_LINES) * (LCD_BIT_PER_PIXEL) / 8)

#define EXAMPLE_PIN_NUM_LCD_CS     12
#define EXAMPLE_PIN_NUM_LCD_PCLK   38
#define EXAMPLE_PIN_NUM_LCD_DATA0  4
#define EXAMPLE_PIN_NUM_LCD_DATA1  5
#define EXAMPLE_PIN_NUM_LCD_DATA2  6
#define EXAMPLE_PIN_NUM_LCD_DATA3  7
#define EXAMPLE_PIN_NUM_LCD_RST    1
/** Tearing-effect output, wired on this board but unused — partial strips do not tear here. */
#define EXAMPLE_PIN_NUM_LCD_TE     13

#define Backlight_MAX 100

extern esp_lcd_panel_handle_t panel_handle;

void LCD_Init(void);
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);

void Backlight_Init(void);
void Set_Backlight(uint8_t Light);
void LCD_SetRotation(uint8_t rotation);
