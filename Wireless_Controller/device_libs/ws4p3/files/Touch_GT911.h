#pragma once

#include "Arduino.h"
#include "I2C_Driver.h"
#include "CH422G.h"
#include "Display_ST7262.h"

/*
 * GT911 capacitive touch -- ESP32-S3-Touch-LCD-4.3.
 *
 * The GT911 picks its own I2C address out of reset from the level on its INT pin:
 * INT low -> 0x5D, INT high -> 0x14. GT911_Touch_Reset() drives INT low across the reset pulse,
 * so 0x5D is the address here. If a scan ever shows 0x14 instead, the reset sequence ran with
 * INT floating.
 *
 * Reset is not on a GPIO: it is CH422G EXIO1 (schematic net CTP_RST).
 */

#define GT911_ADDR          0x5D
#define GT911_INT_PIN       4     // schematic CTP_IRQ

#define Mirror_X       0
#define Mirror_Y       0

#define Touch_WIDTH     ESP_PANEL_LCD_WIDTH
#define Touch_HEIGHT    ESP_PANEL_LCD_HEIGHT

#define GT911_LCD_TOUCH_MAX_POINTS             (5)

/* GT911 registers */
#define ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG    (0x8140)
#define ESP_LCD_TOUCH_GT911_Resolution_REG    (0x8146)
#define ESP_LCD_TOUCH_GT911_READ_DATA_REG     (0x814E)

extern uint8_t Touch_interrupts;

struct GT911_Touch {
  uint8_t points;  // Number of touch points
  struct {
    uint16_t x;        /*!< X coordinate */
    uint16_t y;        /*!< Y coordinate */
    uint16_t strength; /*!< Strength */
  } coords[GT911_LCD_TOUCH_MAX_POINTS];
};

uint8_t Touch_Init();
void Touch_Loop(void);
uint8_t GT911_Touch_Reset(void);
void GT911_Read_cfg(void);
uint8_t Touch_Read_Data(void);
uint8_t Touch_Get_XY(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
void example_touchpad_read(void);
void IRAM_ATTR Touch_GT911_ISR(void);
