// NOTE: This is the classic bluetooth version and will be depricated eventually
#ifndef bt_h
#define bt_h
#include "user_defines.h"
#include "Wheel.h"
#include "BluetoothSerial.h"

//values
extern BluetoothSerial bt;
extern byte currentProfile[4];
extern bool sendProfileBT;
extern bool pause_exe;
extern bool skipPerciseSet;

//functions
extern int getTankPressure();
extern Wheel *getWheel(int i);
extern void airUp();
extern void airOut();
extern void airUpRelativeToAverage(int value);
extern void writeProfile(byte profileIndex);
extern void setBaseProfile(byte value);
extern void readProfile(byte profileIndex);
extern void setRideHeightFrontPassenger(byte value);
extern void setRideHeightRearPassenger(byte value);
extern void setRideHeightFrontDriver(byte value);
extern void setRideHeightRearDriver(byte value);
extern void setRiseOnStart(bool value);
extern void setRaiseOnPressureSet(bool value);

#endif