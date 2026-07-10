#ifndef waveshare_h
#define waveshare_h

void waveshare_init();
void waveshare_loop();
char *getBatteryVoltageString();
bool isBatteryCharging();

// True when the IMU-based motion detector considers the vehicle to be moving.
// Always false on boards without an IMU (HAS_MOTION_IMU == 0) or before init.
bool isVehicleMoving();

#endif