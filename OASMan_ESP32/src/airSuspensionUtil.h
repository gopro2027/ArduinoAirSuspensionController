#ifndef airSuspensionUtil_h
#define airSuspensionUtil_h

#include <user_defines.h>
#include "components/manifold.h"
#include "components/compressor.h"
#include "components/wheel.h"
#include "saveData.h"
#include "sampleReading.tcc"

extern Manifold *manifold;
extern Compressor *compressor;
extern Wheel *wheel[4];

#if USE_ADS == true
extern Adafruit_ADS1115 ADS1115A;
#if USE_2_ADS == true
extern Adafruit_ADS1115 ADS1115B;
#endif
#endif

Manifold *getManifold();
Compressor *getCompressor();
Wheel *getWheel(int i);
void setRideHeightFrontPassenger(byte value);
void setRideHeightRearPassenger(byte value);
void setRideHeightFrontDriver(byte value);
void setRideHeightRearDriver(byte value);
void initializeADS();
void setupManifold();
bool isAnyWheelActive();
void airUp(bool quick = false);
void airOut();
void airUpRelativeToAverage(int value);
void accessoryWireSetup();
void accessoryWireLoop();
void notifyKeepAlive();
bool isVehicleOn();
bool isKeepAliveTimerExpired();

#endif