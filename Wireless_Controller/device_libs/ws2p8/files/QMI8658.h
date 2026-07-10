#pragma once
#include <Arduino.h>

// Minimal QMI8658 6-axis IMU driver for the ws2p8 board.
// Shares the general-purpose Wire bus (SDA=11, SCL=10) already brought up by
// I2C_Init(); reuses the I2C_Read/I2C_Write helpers from I2C_Driver.

// Probe both possible 7-bit addresses (SA0 low/high) at init.
#define QMI8658_ADDR_LOW  0x6A
#define QMI8658_ADDR_HIGH 0x6B

// Initialize the IMU: probe WHO_AM_I, configure accel + gyro, enable both.
// Returns true only if a QMI8658 was found and configured.
bool QMI8658_Init(void);

// True once QMI8658_Init() has succeeded.
bool QMI8658_Present(void);

// Read accelerometer (g) and gyroscope (deg/s). Any out pointer may be null.
// Returns false if the IMU is not present or the I2C read failed.
bool QMI8658_ReadAccelGyro(float *ax, float *ay, float *az,
                           float *gx, float *gy, float *gz);
