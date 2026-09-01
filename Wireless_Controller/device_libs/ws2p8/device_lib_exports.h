#pragma once

// requirements for screen
#include "files/Display_ST7789.h"
#include "files/Touch_CST328.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"

// power key and battery definitions
#include "files/PWR_Key.h"
#include "files/BAT_Driver.h"

#define SUPPORTS_ROTATION 1

// QMI8658 6-axis IMU on the shared I2C bus. Only its accelerometer is used, and only to pick
// a screen orientation for auto rotate (src/utils/imu.cpp). The driver still probes WHO_AM_I
// at boot, so a board that turns out not to be populated just hides the setting.
#define HAS_IMU 1

// IMU -> screen axis mapping for auto rotate. The QMI8658 is mounted turned 90 degrees from
// the panel, so the raw axes cross over: screen X comes from the IMU's Y, and screen Y is the
// IMU's X negated. Derived on hardware 2026-08-31 from all four resting positions.
// ; was: identity on both axes, which sent every orientation to the neighbouring one
#define IMU_SCREEN_X(ax, ay, az) (ay)
#define IMU_SCREEN_Y(ax, ay, az) (-(ax))

// Physical panel: 2.8" diagonal, 240x320 -> ~143 px/inch (the scaling reference panel)
#define DEVICE_DPI 143