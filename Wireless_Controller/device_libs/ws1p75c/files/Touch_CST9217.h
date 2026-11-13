#pragma once
#include <Arduino.h>
#include "I2C_Driver.h"

/** Default CST92xx address. Not every panel ships 0x5A, but this board's wiki documents it. */
#define CST9217_ADDR 0x5A

#define CST9217_SDA_PIN I2C_SDA_PIN
#define CST9217_SCL_PIN I2C_SCL_PIN
#define CST9217_INT_PIN 11
#define CST9217_RST_PIN 2

/** Vendor BSP ships both mirrored and no axis swap; flip here if the bench says otherwise. */
#define CST9217_SWAP_XY  false
#define CST9217_MIRROR_X true
#define CST9217_MIRROR_Y true

void Touch_Init(void);
uint8_t getTouch(uint16_t *x, uint16_t *y);
