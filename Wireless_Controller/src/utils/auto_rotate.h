#ifndef auto_rotate_h
#define auto_rotate_h

#include <Arduino.h>

#include "device_lib_exports.h"
#include "imu.h"

// Auto rotate needs both halves of the feature: an orientation sensor to read (IMU_SUPPORTED,
// owned by imu.h) and a panel this firmware knows how to re-point. Boards missing either
// compile the whole thing out and the settings entry never appears.
#if SUPPORTS_ROTATION == 1 && IMU_SUPPORTED == 1
#define AUTO_ROTATE_SUPPORTED 1
#else
#define AUTO_ROTATE_SUPPORTED 0
#endif

#if AUTO_ROTATE_SUPPORTED == 1

// IMU -> screen axis mapping. Turns the IMU's own axes (imuReadAccel) into the gravity vector
// in *screen* space for the panel's native portrait orientation: +X points right, +Y points
// down. This is a display concern, which is why it lives here and not in the IMU driver.
//
// Boards override these in device_lib_exports.h - both shipping boards do, because their
// QMI8658 sits turned 90 degrees from the panel, so treat the identity defaults below as a
// starting point for a new board rather than a likely answer. Three things can be wrong after
// a bench test, and each has one fix:
//   - portrait and landscape are swapped     -> swap the ax/ay sources
//   - portrait comes up upside down          -> negate IMU_SCREEN_Y
//   - landscape rotates the wrong way        -> negate IMU_SCREEN_X
// Read the values off the `Auto rotate: candidate N (g=..,..)` log line at INFO, holding the
// device in each of the four resting positions; four readings pin the mapping down exactly.
#ifndef IMU_SCREEN_X
#define IMU_SCREEN_X(ax, ay, az) (ax)
#endif
#ifndef IMU_SCREEN_Y
#define IMU_SCREEN_Y(ax, ay, az) (ay)
#endif

/** Auto rotate state machine. Call every pass of the LVGL loop; it rate limits itself. */
void autoRotateLoop();

#else

inline void autoRotateLoop() {}

#endif

#endif /* auto_rotate_h */
