#ifndef airSuspensionUtil_h
#define airSuspensionUtil_h

#include <user_defines.h>
#include "components/manifold.h"
#include "components/compressor.h"
#include "components/wheel.h"
#include "components/AuxillaryOutput.h"
#include "manifoldSaveData.h"
#include "sampleReading.tcc"
#include "components/rf_receiver.h"
#include <FastLED.h>

extern InputType *pressureInputs[5];
extern Manifold *manifold;
extern Compressor *compressor;
extern AuxillaryOutput *auxillaryOutput;
extern Wheel *wheel[4];
extern RfReceiver *rfReceiver;
extern bool forceShutoff;

extern Adafruit_ADS1115 ADS1115A;
extern Adafruit_ADS1115 ADS1115B;
extern Adafruit_ADS1115 ADS1115C;
extern Adafruit_ADS1115 ADS1115D;

Manifold *getManifold();
Compressor *getCompressor();
AuxillaryOutput *getAuxillaryOutput();
Wheel *getWheel(int i);
void initializeADS();
void setupManifold();
bool isAnyWheelActive();
void loadProfileAirUp(int profileIndex);
void accessoryWireSetup();
void accessoryWireLoop();
void ebrakeWireSetup();
void ebrakeWireLoop();
bool isEBrakeOn();
void notifyKeepAlive();
bool isVehicleOn();
bool isVehicleParked(bool dontTrustEBrakeAlone = false, bool requireBothAccAndEbrake_or_GPS = false);
bool isKeepAliveTimerExpired();
namespace PressureSensorCalibration
{
    void learnPressureSensorsRoutine();
}

/** Refit any offset model whose sample count changed. Call at boot and periodically from task_trainAI. */
void trainOffsetModels();
/** True bag pressure recovered from live flowing readings: rawBag + faded (default -> trained) offset. */
double getActualBagPressure(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank, double others_open);
void setupLEDs();
#endif