#ifndef motion_detect_h
#define motion_detect_h

#include <Arduino.h>
#include <user_defines.h>

#ifdef HAS_MOTION_IMU

void motionDetectLoop();        // call at ~30Hz from task_motion
bool motionImuPresent();        // true once the IMU answered and was configured
bool motionImuSaysStationary(); // true only after the IMU has been quiet for MOTION_PARKED_CONFIRM_MS, false if missing

#endif

#endif
