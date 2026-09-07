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

// This board carries a QMI8658 IMU too. HAS_IMU alone now builds the driver (src/utils/imu.cpp)
// and is worth defining as soon as something uses it - wake on movement, say. Auto rotate is a
// separate matter: it additionally needs SUPPORTS_ROTATION, which no driver here implements
// yet, plus IMU_SCREEN_X/Y, plus IMU_I2C_GUARDED if this bus turns out to need the mutex.
// Left undefined for now because the bus setup here has not been checked against the driver.

// Physical panel: 2.8" diagonal, 480x640 -> ~286 px/inch
#define DEVICE_DPI 286