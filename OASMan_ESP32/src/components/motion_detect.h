#ifndef motion_detect_h
#define motion_detect_h

#include <Arduino.h>

void motionDetectInit();    // probe the IMU once at boot, like the ADS1115s
void motionDetectLoop();    // call at ~30Hz from task_motion
bool motionImuPresent();    // true while an IMU is found and responding
bool motionImuSaysStationary(); // true after the IMU has been quiet for MOTION_PARKED_CONFIRM_MS, check motionImuPresent() first

#endif
