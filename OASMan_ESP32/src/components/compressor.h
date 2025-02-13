#ifndef compressor_h
#define compressor_h

#include <user_defines.h>
#include "input_type.h"
#include "solenoid.h"
#include "sampleReading.tcc"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "manifoldSaveData.h"

#define PRESSURE_AVERAGE_ARRAY_SIZE 5
#define FREEZE_TIME_CHECK_MS 15 * 1000 // 15 seconds
#define FREEZE_TIME_PAUSE_MS 90 * 1000 // minute and 30 seconds
#define TANK_READING_NOT_READY_YET_VALUE -999

class Compressor
{
private:
    InputType *readPin;
    float currentPressure;
    float pressureArray[PRESSURE_AVERAGE_ARRAY_SIZE];
    int pressureArrayCounter;
    float freezeTimerLastReadValue;
    unsigned long lastFreezeTime;
    unsigned long pauseExecutionUntilTime;
    Solenoid s_trigger; // Not a solenoid, but works the same way
public:
    Compressor();
    Compressor(InputType *triggerPin, InputType *readPin);
    void loop();
    float readPressure();
    float getTankPressure();
    InputType *getReadPin();
    bool isFrozen();
    bool isOn();
};
extern Compressor *getCompressor();           // defined in airSuspensionUtil.h
extern bool isVehicleOn();                    // defined in airSuspensionUtil.h
extern bool isAnyWheelActive();               // defined in airSuspensionUtil.h
extern float readPinPressure(InputType *pin); // defined in Wheel.h
#endif
