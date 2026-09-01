#pragma once

// requirements for screen
#include "files/Display_ST7701.h"
#include "files/Touch_GT911.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"

// power key and battery definitions
#include "files/BAT_Driver.h"

// include blank power key implementation
#include "files/PWR_Key_Dummy.h"

#define SUPPORTS_ROTATION 0

// This board carries a QMI8658 IMU too, but auto rotate also needs SUPPORTS_ROTATION, which
// no driver here implements yet. Defining HAS_IMU now would do nothing; add it together with
// rotation support (plus IMU_SCREEN_X/Y, and IMU_I2C_GUARDED if this bus gets a mutex).

// Physical panel: 2.8" diagonal, 480x640 -> ~286 px/inch
#define DEVICE_DPI 286