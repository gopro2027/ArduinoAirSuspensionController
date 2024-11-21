#ifndef airSuspensionUtil_h
#define airSuspensionUtil_h

#include "user_defines.h"
#include "manifold.h"
#include "compressor.h"
#include "wheel.h"
#include "saveData.h"

extern Manifold *manifold;
extern Compressor *compressor;
extern Wheel *wheel[4];
extern bool skipPerciseSet;

#if USE_ADS == true
extern Adafruit_ADS1115 ADS1115A;
#if USE_2_ADS == true
extern Adafruit_ADS1115 ADS1115B;
#endif
#endif

Manifold *getManifold();
Wheel *getWheel(int i);
void setRideHeightFrontPassenger(byte value);
void setRideHeightRearPassenger(byte value);
void setRideHeightFrontDriver(byte value);
void setRideHeightRearDriver(byte value);
void initializeADS();
void setupManifold();
bool isAnyWheelActive();
void setGoToPressureGoalPercise(byte wheelnum);
void setNotGoToPressureGoalPercise(byte wheelnum);
bool shouldDoPressureGoalOnWheel(byte wheelnum);
void pressureGoalRoutine();
void airUp();
void airOut();
void airUpRelativeToAverage(int value);
int getTankPressure();
void readPressures();
void compressorLogic();

#endif