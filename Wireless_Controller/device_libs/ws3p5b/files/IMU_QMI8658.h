// IMU_QMI8658.h
#pragma once

#include <Arduino.h>
#include "SensorQMI8658.hpp"

// Waveshare 4.3" board defaults
#ifndef IMU_SDA_PIN
#  define IMU_SDA_PIN 8
#endif

#ifndef IMU_SCL_PIN
#  define IMU_SCL_PIN 7
#endif

// QMI8658 INT1 -> GPIO0 (per Waveshare pinout)
#ifndef IMU_INT_GPIO
#  define IMU_INT_GPIO 0
#endif

// Public API
bool imu_init();                    // call once at startup
void imu_enter_wake_mode();         // configure for wake-on-motion
void imu_exit_wake_mode();          // back to normal mode
bool imu_data_ready();              // check/clear interrupt after wake
void imu_read(IMUdata &acc, IMUdata &gyr);  // optional helper
