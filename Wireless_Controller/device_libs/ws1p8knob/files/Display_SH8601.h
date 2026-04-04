#pragma once
#include <Arduino.h>
#include "esp_lcd_panel_ops.h"

#define LCD_WIDTH   360
#define LCD_HEIGHT  360

#define LCD_BIT_PER_PIXEL  16

/** Must match LVGL partial strip height (LVGL_Driver.h). SPI max_transfer_sz drives internal DMA priv buffer sizing — do not use full-frame size here. */
#define SH8601_LVGL_PARTIAL_LINES 24
#define SH8601_MAX_TRANSFER_SZ ((LCD_WIDTH) * (SH8601_LVGL_PARTIAL_LINES) * (LCD_BIT_PER_PIXEL) / 8)

#define EXAMPLE_PIN_NUM_LCD_CS     14
#define EXAMPLE_PIN_NUM_LCD_PCLK   13
#define EXAMPLE_PIN_NUM_LCD_DATA0  15
#define EXAMPLE_PIN_NUM_LCD_DATA1  16
#define EXAMPLE_PIN_NUM_LCD_DATA2  17
#define EXAMPLE_PIN_NUM_LCD_DATA3  18
#define EXAMPLE_PIN_NUM_LCD_RST    21
#define EXAMPLE_PIN_NUM_BK_LIGHT   47

#define Backlight_MAX 100

extern esp_lcd_panel_handle_t panel_handle;

void LCD_Init(void);
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);

void Backlight_Init(void);
void Set_Backlight(uint8_t Light);
void LCD_SetRotation(uint8_t rotation);
