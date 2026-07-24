#pragma once

#include "Arduino.h"
#include <Wire.h>

#define CST3530_ADDR 0x58
#define CST3530_SDA_PIN       1
#define CST3530_SCL_PIN       3
#define CST3530_INT_PIN       4
#define CST3530_RST_PIN       2
#define CST3530_I2C_FREQ_HZ   400000

#define CST3530_LCD_TOUCH_MAX_POINTS             (5)

/* CST3530 registers */
#define ESP_LCD_TOUCH_CST3530_END_READ_REG        0xD00002AB
#define ESP_LCD_TOUCH_CST3530_DATA_REG            0xD0070000
#define ESP_LCD_TOUCH_CST3530_COORD_NEXT_REG      0xD0070900

struct CST3530_Touch {
  uint8_t points;
  struct {
    uint16_t x;
    uint16_t y;
    uint16_t strength;
  } coords[CST3530_LCD_TOUCH_MAX_POINTS];
};

uint8_t TOUCH2_Init(void);
uint8_t CST3530_Touch_Reset(void);
uint8_t Touch2_Read_Data(void);
uint8_t Touch2_Get_XY(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
