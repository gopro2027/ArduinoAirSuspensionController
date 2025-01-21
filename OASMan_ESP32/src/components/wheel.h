#ifndef wheel_h
#define wheel_h

#include <Arduino.h>
#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include "compressor.h"

class Wheel
{
private:
    InputType *pressurePin;
    byte thisWheelNum;

    byte pressureGoal;
    unsigned long routineStartTime;
    bool flagStartPressureGoalRoutine; // flag to tell it to start routine to pressureGoal
    bool quickMode;                    // flag to skip extra percise measurements

    float pressureValue;

    Solenoid s_AirIn;
    Solenoid s_AirOut;

public:
    Wheel();
    Wheel(InputType *solenoidInPin, InputType *solenoidOutPin, InputType *pressurePin, byte thisWheelNum);
    void initPressureGoal(int newPressure, bool quick = false);
    void loop();
    void readPressure();
    float getPressure();
    bool isActive();
    Solenoid *getInSolenoid();
    Solenoid *getOutSolenoid();
    InputType *getPressurePin();
};

float readPinPressure(InputType *pin);
#endif
