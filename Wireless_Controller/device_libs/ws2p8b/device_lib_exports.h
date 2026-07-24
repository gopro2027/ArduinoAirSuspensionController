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

// Physical panel: 2.8" diagonal, 480x640 -> ~286 px/inch
#define DEVICE_DPI 286