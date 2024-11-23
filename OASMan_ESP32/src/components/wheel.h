#ifndef wheel_h
#define wheel_h

#include <Arduino.h>
#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include "Manifold.h"
#include "tasks/taskUtil.h"

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
};

float readPinPressure(InputType *pin);

// janky fix because i can't import airSuspensionUtil.h due to circular import I think
extern Manifold *getManifold();
extern int getTankPressure();

#endif
