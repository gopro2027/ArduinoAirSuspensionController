#pragma once
#include <Wire.h>

#define I2C_SCL_PIN       14
#define I2C_SDA_PIN       15

// axp2101_pmic.h defaults these to the 3.5b's pins, so define them before it is included.
#ifndef I2C_SDA
#define I2C_SDA I2C_SDA_PIN
#endif
#ifndef I2C_SCL
#define I2C_SCL I2C_SCL_PIN
#endif

#define I2C_CLK_SPEED 400000

void I2C_Init(void);

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);
bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);
