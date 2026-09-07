#ifndef wake_on_movement_h
#define wake_on_movement_h

#include <Arduino.h>

#include "imu.h"

// Needs an orientation sensor and nothing else. Deliberately NOT gated on SUPPORTS_ROTATION:
// picking the device up is not a display concern, and the tests this feature runs (vector
// magnitude, angle between two gravity vectors) are rotation-invariant, so it never touches
// the panel geometry or the IMU_SCREEN_* mapping that auto rotate owns.
#if IMU_SUPPORTED == 1
#define WAKE_ON_MOVEMENT_SUPPORTED 1
#else
#define WAKE_ON_MOVEMENT_SUPPORTED 0
#endif

#if WAKE_ON_MOVEMENT_SUPPORTED == 1

/** Wake-on-movement state machine. Call every pass of the LVGL loop; it rate limits itself. */
void wakeOnMovementLoop();

#else

inline void wakeOnMovementLoop() {}

#endif

#endif /* wake_on_movement_h */
