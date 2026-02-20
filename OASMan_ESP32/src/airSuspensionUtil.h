#ifndef airSuspensionUtil_h
#define airSuspensionUtil_h

#include <user_defines.h>
#include "components/manifold.h"
#include "components/compressor.h"
#include "components/wheel.h"
#include "manifoldSaveData.h"
#include "sampleReading.tcc"
#include "components/rf_receiver.h"
#include <FastLED.h>

extern InputType *pressureInputs[5];
extern Manifold *manifold;
extern Compressor *compressor;
extern Wheel *wheel[4];
extern RfReceiver *rfReceiver;
extern bool forceShutoff;

extern Adafruit_ADS1115 ADS1115A;
extern Adafruit_ADS1115 ADS1115B;
extern Adafruit_ADS1115 ADS1115C;
extern Adafruit_ADS1115 ADS1115D;

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
void loadProfileAirUpQuick(int profileIndex);
void airOut();
void airUpRelativeToAverage(int value);
void accessoryWireSetup();
void accessoryWireLoop();
void ebrakeWireSetup();
void ebrakeWireLoop();
bool isEBrakeOn();
void notifyKeepAlive();
bool isVehicleOn();
bool isKeepAliveTimerExpired();
namespace PressureSensorCalibration
{
    void learnPressureSensorsRoutine();
}

void trainAIModels();
double getAiPredictionTime(SOLENOID_AI_INDEX aiIndex, double start_pressure, double end_pressure, double tank_pressure);
bool canUseAiPrediction(SOLENOID_AI_INDEX aiIndex);
void setupLEDs();
#endif