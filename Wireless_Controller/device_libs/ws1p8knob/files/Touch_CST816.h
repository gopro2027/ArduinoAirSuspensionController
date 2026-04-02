#pragma once

#include "Arduino.h"

#define CST816_ADDR            0x15
#define CST816_SDA_PIN         11
#define CST816_SCL_PIN         12
#define CST816_I2C_CLK_SPEED   (300 * 1000)

#define CST816_LCD_TOUCH_MAX_POINTS 1

#ifdef __cplusplus
extern "C" {
#endif

void Touch_Init(void);
uint8_t getTouch(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif
