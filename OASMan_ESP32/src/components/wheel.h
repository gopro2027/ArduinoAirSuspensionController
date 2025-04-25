#ifndef wheel_h
#define wheel_h

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
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
    bool flagStartPressureGoalRoutine; // flag to tell it to start routine to pressureGoal
    bool quickMode;                    // flag to skip extra percise measurements

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
#endif
