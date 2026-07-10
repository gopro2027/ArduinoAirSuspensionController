#pragma once

// requirements for screen
#include "files/Display_ST7789.h"
#include "files/Touch_CST328.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"

// power key and battery definitions
#include "files/PWR_Key.h"
#include "files/BAT_Driver.h"

// 6-axis IMU (QMI8658) for moving/parked detection
#include "files/QMI8658.h"

#define SUPPORTS_ROTATION 1

// Enables QMI8658 IMU polling + moving/parked status indicator
#define HAS_MOTION_IMU 1