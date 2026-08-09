#ifndef wheel_h
#define wheel_h

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <atomic>
#include <functional>
#include <user_defines.h>
#include "input_type.h"
#include "solenoid.h"
#include "compressor.h"
#include "manifoldSaveData.h"
#include "../sampleLoggingUtil.h" // getPredictedBagPressure / getPredictedBagHeight

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
    unsigned long slLastInstabilityDetectedTimeMS = 0;    // when current pressure-stable window began (0 = none)
    unsigned long slLastCorrection = 0; // last correction time (for cooldown)
    int8_t slSameDirCount = 0;          // signed run-length of same-direction corrections
    // Baseline capture (see Wheel::sensorlessCaptureBaseline())
    unsigned long slValvesClosedSince = 0; // when all valves last became closed (0 = a valve is open)
    bool slBaselineCaptured = false;       // captured the baseline once for the current valve-close event
    byte directlySetPressure = 0; // This is the 'real' pressure that is read after any valve movement. This is our 'expected' pressure in a sense. This is tracked by the sensorlessCaptureBaseline and is only usable when slBaselineCaptured is true.

    void goalRoutine();
    // Fine precision phase: short valve bursts to the exact pressure. True = landed on goal.
    bool achieveFineGoal();
    // Block until the reading holds within `band` for SETTLE_STABLE_MS, then return it (valves closed).
    double waitForStableReading(int band);
    // Open `valve` for `ms`, then close it. Used by the fine phase.
    void openValveForMs(Solenoid *valve, uint32_t ms);
    // Log an offset sample from a manual valve move (watched by valve state each Wheel::loop tick).
    void captureManualOffsetSample();
    bool manualValveWasOpen = false;
    bool manualUp = false;                // direction of the tracked manual move (picks the settle time)
    uint32_t manualSettleUntil = 0;       // while non-zero: valve closed, waiting to read the settled bag
    uint8_t manualFlowBag = 0, manualFlowTank = 0;
    SOLENOID_AI_INDEX manualAiIndex = AI_MODEL_UNDEFINED;
    void maintainPressure();
    void heightsensorlessLevelling();
    void pressureCaptureBaseline();
    void nullifySensorlessBaseline();
    void markInstability(float current);

    bool onlyAirUp = false;
    std::function<void()> onPressureGoalComplete;
    void trackPressureStability();
    bool isPressureStable();

    float readLevelSensorNormalized();
    bool haveValvesBeenClosedForSomeTime(uint32_t additionalTimeMS = 0);

public:
    Wheel();
    Wheel(int solenoidInPin, int solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum);
    bool initPressureGoal(int newPressure, std::function<void()> onComplete = nullptr);
    bool initPressureGoal(int newPressure, bool onlyAirUp, std::function<void()> onComplete = nullptr);
    void loop();
    void readInputs();
    float readLevelSensorRaw();
    float getSelectedInputValue();
    bool isActive();
    Solenoid *getInSolenoid();
    Solenoid *getOutSolenoid();
    InputType *getPressurePin();
};

float readPinPressure(InputType *pin, bool heightMode);

void setupWheelLockSem();

extern Manifold *getManifold(); // defined in airSuspensionUtil.h
#endif
