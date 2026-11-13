#pragma once

#include "files/Display_CO5300.h"
#include "files/Touch_CST9217.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"
// Must be visible here: src/utils/imu.cpp calls i2c_lock() when IMU_I2C_GUARDED is 1.
#include "files/i2c_guard.h"

#include "files/BAT_Driver.h"

#include "files/PWR_Key.h"

#define SUPPORTS_ROTATION 0

/** Round 466x466 display: use swipe + circle UI under src/ui_circle/ instead of tab navbar. */
#define SCREEN_MODE_CIRCLE

// No rotary encoder on this board, so HAS_ROTARY_ENCODER stays undefined -- it is independent
// of SCREEN_MODE_CIRCLE, which only picks the UI tree.

// Physical panel: 1.75" round, 466x466 -> ~266 px/inch
#define DEVICE_DPI 266

// QMI8658 on the same Wire bus as touch and the PMIC, which src/utils/imu.cpp already drives.
#define HAS_IMU 1

// pmic_cache polls the AXP2101 from a task on the other core, so IMU reads need the bus mutex.
#define IMU_I2C_GUARDED 1

// IMU_SCREEN_X/Y are deliberately absent: only auto rotate consumes them and SUPPORTS_ROTATION is 0.

// No light-sleep path on this board, same as the 3.5b. The side PWR button is not on an MCU pin,
// so a sleeping device could only be revived with BOOT or the timeout -- it would look dead.
#ifndef DEVICE_SUPPORTS_LIGHT_SLEEP
#define DEVICE_SUPPORTS_LIGHT_SLEEP 0
#endif

// The AXP2101 cuts the rails for a real power-off, and a later PWR press powers it back up.
#ifndef DEVICE_HAS_HARD_SHUTDOWN
#define DEVICE_HAS_HARD_SHUTDOWN 1
#endif

void power_hard_shutdown(void);
