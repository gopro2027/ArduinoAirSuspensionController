#pragma once
#include <Wire.h>

// ESP32-S3-Touch-LCD-4.3: one shared I2C bus carries the GT911 touch controller and the
// CH422G IO expander (schematic nets D_SDA / D_SCL).
#define I2C_SDA_PIN       8
#define I2C_SCL_PIN       9
// Waveshare's own examples run this bus at 400 kHz. The 2.8b runs 800 kHz, but that board has
// nothing but the touch panel + a TCA9554 on it; here the CH422G is also driving the backlight
// and both LCD/touch resets, so stay at the vendor-validated speed.
#define I2C_Frequency     400000

// Safe to call more than once -- Backlight_Init() needs the bus up before board_drivers_init()
// gets around to calling I2C_Init(), because the backlight enable sits on the expander.
void I2C_Init(void);

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);
bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);
