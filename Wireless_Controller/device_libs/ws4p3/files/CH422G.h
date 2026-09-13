#pragma once

#include <stdio.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2C_Driver.h"

/*
 * CH422G IO expander (U11) -- ESP32-S3-Touch-LCD-4.3.
 *
 * This is NOT a TCA9554 (what the 2.8b uses) and the two differ in two ways that will silently
 * mis-drive pins if you copy code between them:
 *
 *  1. Register addressing. The CH422G has no register-pointer byte. Each register is its own
 *     I2C *device address*, and a write is a single raw byte to that address:
 *         WR_SET 0x24   mode/config      (bit0 IO_OE: 1 = IO0..IO7 are outputs)
 *         WR_OC  0x23   OC0..OC3 output  (open-drain pins, unused on this board)
 *         WR_IO  0x38   IO0..IO7 output
 *         RD_IO  0x26   IO0..IO7 input   (1-byte read, no address phase)
 *     (These are the 7-bit forms of the 0x48/0x46/0x70/0x4D values in Espressif's
 *     esp_io_expander_ch422g driver.)
 *
 *  2. Pin numbering. Schematic net EXIO<n> == CH422G IO<n> == bit <n>, starting at 0.
 *     The TCA9554 driver on the 2.8b indexes from 1 and subtracts one; do not do that here.
 *
 * Board wiring (ESP32-S3-Touch-LCD-4.3-Sch.pdf, confirmed against Waveshare's
 * esp_panel_board_custom_conf.h for this board):
 *     EXIO1 = CTP_RST   GT911 touch reset          (active low)
 *     EXIO2 = DISP      MP3302 LED-driver enable   (== the backlight, on/off only)
 *     EXIO3 = LCD_RST   RGB panel reset            (active low)
 *     EXIO4 = SD_CS     microSD chip select        (active low)
 *     EXIO5 = USB_SEL   FSUSB42 mux: USB-C <-> USB host port
 * IO0, IO6 and IO7 are not connected on this board.
 */

#define CH422G_REG_WR_SET   0x24  // mode register
#define CH422G_REG_WR_OC    0x23  // OC0..OC3 output
#define CH422G_REG_WR_IO    0x38  // IO0..IO7 output
#define CH422G_REG_RD_IO    0x26  // IO0..IO7 input

#define CH422G_WR_SET_IO_OE (1 << 0)  // 1 = IO0..IO7 driven as outputs
#define CH422G_WR_SET_OD_EN (1 << 2)  // 1 = OC pins open-drain

#ifndef Low
#define Low   0
#endif
#ifndef High
#define High  1
#endif

// Bit index == schematic EXIO number. Named pins for the four the firmware touches.
#define EXIO_TP_RST    1
#define EXIO_LCD_BL    2
#define EXIO_LCD_RST   3
#define EXIO_SD_CS     4
#define EXIO_USB_SEL   5

// Put IO0..IO7 in output mode and drive a known starting pattern.
// Backlight starts OFF so the panel does not flash garbage before the splash is rendered.
void CH422G_Init(void);

// Drive one IO0..IO7 pin without disturbing the others (cached shadow register -- the CH422G
// output register is write-only, so reading back is not an option).
void CH422G_Set_EXIO(uint8_t pin, uint8_t state);

// Current level of IO0..IO7 as inputs. Only meaningful after CH422G_Set_All_Input().
uint8_t CH422G_Read_EXIO(uint8_t pin);
uint8_t CH422G_Read_EXIOS(void);

void CH422G_Set_All_Input(void);
void CH422G_Set_All_Output(void);
