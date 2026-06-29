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

class Manifold; // from manifold.h, forward reference

class Wheel
{
private:
    InputType *pressurePin;
    InputType *levelSensorPin;
    byte thisWheelNum;

    byte pressureGoal;
    unsigned long routineStartTime;

    float pressureValue;
    float levelValue;

    int s_AirIn;
    int s_AirOut;

    // Sensorless levelling per-corner runtime state (see Wheel::heightsensorlessLevelling())
    float slLastSample = 0;             // pressure-stability window reference reading
    unsigned long slParkedSince = 0;    // when continuous parked state began (0 = not parked)
    unsigned long slStableSince = 0;    // when current pressure-stable window began (0 = none)
    unsigned long slLastCorrection = 0; // last correction time (for cooldown)
    int8_t slSameDirCount = 0;          // signed run-length of same-direction corrections
    // Baseline capture (see Wheel::sensorlessCaptureBaseline())
    unsigned long slValvesClosedSince = 0; // when all valves last became closed (0 = a valve is open)
    bool slBaselineCaptured = false;       // captured the baseline once for the current valve-close event

    void goalRoutine();
    void maintainPressure();
    void heightsensorlessLevelling();
    void sensorlessCaptureBaseline();

public:
    Wheel();
    Wheel(int solenoidInPin, int solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum);
    void initPressureGoal(int newPressure);
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
extern Manifold *getManifold(); // defined in airSuspensionUtil.h
#endif
