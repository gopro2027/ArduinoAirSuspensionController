#ifndef imu_h
#define imu_h

#include <Arduino.h>

#include "device_lib_exports.h"

// Auto rotate needs both halves of the feature: an orientation sensor to read, and a panel
// this firmware knows how to re-point. Boards missing either compile the whole thing out and
// the settings entry never appears.
#if SUPPORTS_ROTATION == 1 && defined(HAS_IMU) && HAS_IMU == 1
#define AUTO_ROTATE_SUPPORTED 1
#else
#define AUTO_ROTATE_SUPPORTED 0
#endif

#if AUTO_ROTATE_SUPPORTED == 1

// Board -> screen axis mapping. The result is the gravity vector in *screen* space for the
// panel's native portrait orientation: +X points right, +Y points down. A board whose IMU is
// mounted differently overrides these in its device_lib_exports.h - both shipping boards do,
// because their QMI8658 sits turned 90 degrees from the panel, so treat the identity defaults
// below as a starting point for a new board rather than a likely answer. Three things can be
// wrong after a bench test, and each has one fix here:
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

/** Probe and configure the IMU. Call once after I2C is up and before the UI is built. */
void imuInit();

/** True only if an IMU actually answered at boot. Gates the auto rotate settings entry. */
bool imuAvailable();

/** Gravity in screen space (g units), already through IMU_SCREEN_X/Y. False if the read failed. */
bool imuReadGravity(float *gx, float *gy, float *gz);

/** Auto rotate state machine. Call every pass of the LVGL loop; it rate limits itself. */
void autoRotateLoop();

#else

inline void imuInit() {}
inline bool imuAvailable() { return false; }
inline void autoRotateLoop() {}

#endif

#endif /* imu_h */
