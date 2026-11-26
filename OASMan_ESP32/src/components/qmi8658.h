#ifndef qmi8658_h
#define qmi8658_h

#include <Arduino.h>
#include <user_defines.h>

#ifdef HAS_MOTION_IMU

// QMI8658 7-bit addresses by SA0 level (V5 ties SA0 to GND, so 0x6B)
#define QMI8658_ADDR_SA0_LOW 0x6B
#define QMI8658_ADDR_SA0_HIGH 0x6A

bool QMI8658_Init();    // probe WHO_AM_I, configure accel + gyro, true if found
bool QMI8658_Present(); // true once QMI8658_Init() has succeeded
bool QMI8658_ReadAccelGyro(float *ax, float *ay, float *az, float *gx, float *gy, float *gz); // g and deg/s, any pointer may be null

#endif

#endif
