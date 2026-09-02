#ifndef imu_h
#define imu_h

#include <Arduino.h>

#include "device_lib_exports.h"

// Presence of an orientation sensor on this board, and nothing about what it is used for.
// Consumers (auto rotate today, wake-on-movement later) add their own gates on top.
#if defined(HAS_IMU) && HAS_IMU == 1
#define IMU_SUPPORTED 1
#else
#define IMU_SUPPORTED 0
#endif

#if IMU_SUPPORTED == 1

/** Probe and configure the IMU. Call once after I2C is up and before anything reads it. */
void imuInit();

/** True only if an IMU actually answered at boot. Gate any IMU-backed UI on this. */
bool imuAvailable();

/**
 * Latest acceleration in g, in the IMU's own axes exactly as the part reports them - no
 * board or screen orientation is applied here, because different consumers want different
 * frames (auto rotate wants screen space, motion detection wants magnitude). False if the
 * read failed; the caller keeps its previous sample.
 */
bool imuReadAccel(float *ax, float *ay, float *az);

#else

inline void imuInit() {}
inline bool imuAvailable() { return false; }

#endif

#endif /* imu_h */
