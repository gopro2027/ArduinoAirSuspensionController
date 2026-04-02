#pragma once

#include "files/Display_SH8601.h"
#include "files/Touch_CST816.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"

#include "files/BAT_Driver.h"

#include "files/PWR_Key_Dummy.h"

#define SUPPORTS_ROTATION 0

/** Round 360x360 display: use swipe + circle UI under src/ui_circle/ instead of tab navbar. */
#define SCREEN_MODE_CIRCLE
