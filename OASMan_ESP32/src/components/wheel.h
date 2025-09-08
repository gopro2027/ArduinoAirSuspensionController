#ifndef wheel_h
#define wheel_h

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <atomic>
#include <user_defines.h>
#include "input_type.h"
#include "solenoid.h"
#include "compressor.h"
#include "manifoldSaveData.h"

class Wheel
{
private:
    InputType *pressurePin;
    InputType *levelSensorPin;
    byte thisWheelNum;

    byte pressureGoal;
    unsigned long routineStartTime;
    bool quickMode; // flag to skip extra percise measurements

    float pressureValue;
    float levelValue;

    Solenoid *s_AirIn;
    Solenoid *s_AirOut;

public:
    Wheel();
    Wheel(Solenoid *solenoidInPin, Solenoid *solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum);
    void initPressureGoal(int newPressure, bool quick = false);
    void loop();
    void readInputs();
    float getSelectedInputValue();
    bool isActive();
    Solenoid *getInSolenoid();
    Solenoid *getOutSolenoid();
    InputType *getPressurePin();
};

float readPinPressure(InputType *pin, bool heightMode);

void setupWheelLockSem();

extern bool canUseAiPrediction(SOLENOID_AI_INDEX aiIndex);
extern double getAiPredictionTime(SOLENOID_AI_INDEX aiIndex, double start_pressure, double end_pressure, double tank_pressure);
#endif
