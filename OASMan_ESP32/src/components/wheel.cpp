#include "wheel.h"
#include "manifold.h"

#define NUM_WHEEL_THREADS 4
std::atomic<bool> flagStartPressureGoalRoutine[NUM_WHEEL_THREADS];

extern bool isVehicleParked(bool dontTrustEBrakeAlone = false, bool requireBothAccAndEbrake_or_GPS = false); // defined in airSuspensionUtil.cpp
extern bool isAnyWheelActive();            // defined in airSuspensionUtil.cpp

int getMinValveOpenPSI()
{
    return 0;
}

Wheel::Wheel() {}

Wheel::Wheel(int solenoidInPin, int solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum)
{
    this->pressurePin = pressurePin;
    this->levelSensorPin = levelSensorPin;
    this->thisWheelNum = thisWheelNum;
    this->s_AirIn = solenoidInPin;
    this->s_AirOut = solenoidOutPin;
    this->pressureValue = 0;
    this->pressureGoal = 0;
    this->routineStartTime = 0;
    // this->flagStartPressureGoalRoutine = false;
    flagStartPressureGoalRoutine[thisWheelNum] = false;
}

Solenoid *Wheel::getInSolenoid()
{
    return getManifold()->get(this->s_AirIn);
}

Solenoid *Wheel::getOutSolenoid()
{
    return getManifold()->get(this->s_AirOut);
}

InputType *Wheel::getPressurePin()
{
    return this->pressurePin;
}

float analogToRange(int nativeAnalogValue)
{
    float floored = float(nativeAnalogValue) - pressureZeroAnalogValue;  // chop off the 0 value
    float totalRange = pressureMaxAnalogValue - pressureZeroAnalogValue; // get the total analog voltage difference between min and max
    float normalized = floored / totalRange;                             // 0 to 1 where 0 is 0psi and 1 is max psi
    return normalized;
}

float analogToPressure(int nativeAnalogValue)
{
    return analogToRange(nativeAnalogValue) * getpressureSensorMax(); // multiply out 0 to 1 by our max psi
}

float analogToHeightPercentage(int nativeAnalogValue)
{
    return analogToRange(nativeAnalogValue) * getHeightSensorMax(); // multiply out 0 to 1 by 100 to get a percentage
}

// Testing function, convert pressure value back to analog value, exact reverse of analogToPressure
float pressureToAnalog(float psi)
{
    float totalRange = pressureMaxAnalogValue - pressureZeroAnalogValue; // get the total analog voltage difference between min and max
    return (psi / getpressureSensorMax()) * totalRange + pressureZeroAnalogValue;
}

float readPinPressure(InputType *pin, bool heightMode)
{
    if (heightMode == false)
    {
        return analogToPressure(pin->analogRead());
    }
    else
    {
        return analogToHeightPercentage(pin->analogRead());
    }
}

// Raw (pre-normalization, pre-invert) height sensor reading, used to capture calibration points
float Wheel::readLevelSensorRaw()
{
    return readPinPressure(this->levelSensorPin, true);
}

// Normalize a raw 0-100 height reading into 0-100 between the per-wheel calibrated
// min/max points. Defaults (min=0, max=100) make this an identity mapping.
float Wheel::readLevelSensorNormalized()
{
    float calMin = getheightCalMin(this->thisWheelNum);
    float calMax = getheightCalMax(this->thisWheelNum);

    float reading = this->readLevelSensorRaw(); // always 0 to 100

    bool inverted = calMin > calMax;

    if (inverted) {
        float tmp = calMin;
        calMin = calMax;
        calMax = tmp;
    }

    float range = calMax - calMin;
    
    if (fabsf(range) < 0.001f)
    {
        return reading; // degenerate calibration, skip normalization
    }
    float normalized = ((reading - calMin) / range) * getHeightSensorMax();
    if (normalized < 0.0f)
    {
        normalized = 0.0f;
    }
    if (normalized > getHeightSensorMax())
    {
        normalized = getHeightSensorMax();
    }
    if (inverted) {
        normalized = getHeightSensorMax() - normalized;
    }
    return normalized;
}

void Wheel::readInputs()
{
    this->pressureValue = readPinPressure(this->pressurePin, false);
    if (getheightSensorMode())
    {
        this->levelValue = readLevelSensorNormalized();
    }
}

float Wheel::getSelectedInputValue()
{
    float value = getheightSensorMode() ? this->levelValue : this->pressureValue;
    if (value < 0)
    {
        return 0;
    }
    else
    {
        return value;
    }
}

bool Wheel::isActive()
{
    return getInSolenoid()->isOpen() || getOutSolenoid()->isOpen() || flagStartPressureGoalRoutine[thisWheelNum].load();
}

bool Wheel::initPressureGoal(int newPressure, bool onlyAirUp, std::function<void()> onComplete)
{

    if (newPressure > (getheightSensorMode() ? getHeightSensorMax() * 1.03f : getbagMaxPressure()))
    {
        // hardcode not to go above set psi
        return false;
    }
    int pressureDif = newPressure - this->getSelectedInputValue(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI())
    {
        // okay we need to set the values, but only if we are airing out or if the tank has more pressure than what is currently in the bags
        bool tankIsLowerThanBag = false;
        if (getheightSensorMode() == false)
        {
            // it's in pressure mode, check if tank is less than whats currently in the bag and if it is then tankIsLowerThanBag is true.
            // we don't care about the goal because we still want to try to reach the goal even if the tank isn't capable. We do care if tank is more than the bag though because if it's less than the bag then it will actually air out.
            if (getCompressor()->getTankPressure() < this->getSelectedInputValue())
            {
                tankIsLowerThanBag = true;
            }
        }
        if (pressureDif < 0 || !tankIsLowerThanBag)
        {
            this->pressureGoal = newPressure;
            this->routineStartTime = millis();
            this->onlyAirUp = onlyAirUp;
            this->onPressureGoalComplete = onComplete;
            flagStartPressureGoalRoutine[thisWheelNum] = true;
            return true;
        }
    }
    return false;
}

bool Wheel::initPressureGoal(int newPressure, std::function<void()> onComplete)
{
    return this->initPressureGoal(newPressure, false, onComplete);
}

// height sensor: AA-ROT-120 https://www.aliexpress.us/item/3256807527882480.html https://www.amazon.com/Height-Sensor-Suspension-Leveling-AA-ROT-120/dp/B08DJ3HX1B https://www.aliexpress.us/item/3256806751644782.html
// Output voltage UA:0.5-4.5

int wheelLoopBittset = 0;

static SemaphoreHandle_t wheelLockSem;
void setupWheelLockSem()
{
    wheelLockSem = xSemaphoreCreateMutex();
}

void wheelThreadLock()
{
    while (xSemaphoreTake(wheelLockSem, 1) != pdTRUE)
    {
        delay(1);
    }
}

void wheelThreadUnlock()
{
    xSemaphoreGive(wheelLockSem);
}

static int goalSyncClubSize = 0;
static int goalSyncWaiting = 0;
static int goalSyncGeneration = 0;

// Barrier wait cap (backstop only). INVARIANT: a wheel only ever waits at the barrier with its valve
// CLOSED, so waiting is always harmless; the cap just exceeds the slowest single fill (~5000ms).
// Full rationale: docs/goal-sync-barrier.md.
#ifndef GOAL_SYNC_BARRIER_TIMEOUT_MS
#define GOAL_SYNC_BARRIER_TIMEOUT_MS 6000 // was: split 6000 (pressure) / 100 (height)
#endif

static void goalSyncJoin()
{
    wheelThreadLock();
    goalSyncClubSize++;
    wheelThreadUnlock();
}

static void goalSyncLeave()
{
    wheelThreadLock();
    if (goalSyncClubSize > 0)
    {
        goalSyncClubSize--;
    }
    if (goalSyncWaiting > 0 && (goalSyncClubSize == 0 || goalSyncWaiting >= goalSyncClubSize))
    {
        goalSyncWaiting = 0;
        goalSyncGeneration++;
    }
    wheelThreadUnlock();
}

static void goalSyncBarrier(unsigned long timeoutMs)
{
    wheelThreadLock();
    const int gen = goalSyncGeneration;
    goalSyncWaiting++;
    if (goalSyncClubSize > 0 && goalSyncWaiting >= goalSyncClubSize)
    {
        goalSyncWaiting = 0;
        goalSyncGeneration++;
        wheelThreadUnlock();
        return;
    }
    wheelThreadUnlock();

    const unsigned long startMs = millis();
    for (;;)
    {
        wheelThreadLock();
        bool done = (goalSyncGeneration != gen);
        if (!done && goalSyncClubSize > 0 && goalSyncWaiting >= goalSyncClubSize)
        {
            goalSyncWaiting = 0;
            goalSyncGeneration++;
            done = true;
        }
        if (!done && goalSyncClubSize == 0)
        {
            goalSyncWaiting = 0;
            goalSyncGeneration++;
            done = true;
        }
        if (!done && (millis() - startMs) >= timeoutMs)
        {
            goalSyncWaiting = 0;
            goalSyncGeneration++;
            done = true;
        }
        wheelThreadUnlock();
        if (done)
        {
            return;
        }
        delay(1);
    }
}

static const int8_t FLOW_NONE = 0;
static const int8_t FLOW_UP = 1;
static const int8_t FLOW_DOWN = -1;

// Block until the selected input reading holds within `band` for SETTLE_STABLE_MS (SETTLE_MAX_WAIT_MS
// backstop); returns the final reading. Valve(s) must already be closed -- this IS the true settled value.
double Wheel::waitForStableReading(int band)
{
    const uint32_t start = millis();
    this->readInputs();
    double reference = this->getSelectedInputValue();
    uint32_t stableSince = millis();
    for (;;)
    {
        delay(1);
        this->readInputs();
        double v = this->getSelectedInputValue();
        if (fabs(v - reference) > (double)band)
        {
            reference = v; // still moving -> restart the stability window
            stableSince = millis();
        }
        if (millis() - stableSince >= SETTLE_STABLE_MS)
        {
            return v; // held steady long enough -> settled
        }
        if (millis() - start >= SETTLE_MAX_WAIT_MS)
        {
            return v; // backstop: never block on a settle forever
        }
    }
}

// Open `valve` for `ms`, then close it. Used by the fine phase.
void Wheel::openValveForMs(Solenoid *valve, uint32_t ms)
{
    valve->open();
    delay(ms);
    valve->close();
}

// Precision (fine) phase, pressure only: hone in on the EXACT goal with short valve bursts read off the
// accurate valve-closed pressure. Bursts start sized to the error and shrink by FINE_PULSE_OVERSHOOT_SHRINK
// each time the reading crosses the goal (anti-oscillation). No thread sync (bursts are quick and local).
// Exit conditions + full design: AI_TRAINING.md.
bool Wheel::achieveFineGoal()
{
    int8_t prevDir = FLOW_NONE;
    double burstMs = 0.0;
    bool sized = false;
    int stallCount = 0;
    int lastErr = 0;

    for (;;)
    {
        if (millis() > this->routineStartTime + ROUTINE_TIMEOUT_MS)
        {
            return false;
        }

        double trueVal = this->waitForStableReading(SETTLE_STABLE_BAND_PSI);
        int err = this->pressureGoal - (int)lround(trueVal);
        if (abs(err) <= PRESSURE_DEADBAND_PSI)
        {
            return true; // landed on goal (deadband 0 -> exact psi)
        }

        int8_t dir = (err > 0) ? FLOW_UP : FLOW_DOWN;
        if (dir == FLOW_DOWN && this->onlyAirUp)
        {
            return true; // maintain-pressure never vents; accept where we are
        }

        // Stuck detector: the true reading isn't moving between bursts (tank/bag exhausted) -> give up rather
        // than pulse to the routine timeout. Reset whenever it does move (normal convergence / crossings).
        if (sized && err == lastErr)
        {
            if (++stallCount >= FINE_PULSE_MAX_TRIES)
            {
                return false;
            }
        }
        else
        {
            stallCount = 0;
        }
        lastErr = err;

        if (!sized)
        {
            burstMs = (double)abs(err) * FINE_PULSE_MS_PER_PSI; // start proportional to the remaining error
            if (burstMs < FINE_PULSE_MIN_MS) burstMs = FINE_PULSE_MIN_MS;
            if (burstMs > FINE_PULSE_MAX_MS) burstMs = FINE_PULSE_MAX_MS;
            sized = true;
        }
        else if (dir != prevDir)
        {
            burstMs *= FINE_PULSE_OVERSHOOT_SHRINK; // crossed the goal -> damp the oscillation
            if (burstMs < 1.0)
            {
                return false; // can't resolve any finer than this
            }
        }
        prevDir = dir;

        Solenoid *valve = (dir == FLOW_UP) ? getInSolenoid() : getOutSolenoid();
        (dir == FLOW_UP ? getOutSolenoid() : getInSolenoid())->close();
        bool canOpen = true;
#if SIX_VALVE_MANIFOLD == true
        canOpen = getManifold()->canOpenDirectionSixValveThreadSafe(valve);
#endif
        if (canOpen)
        {
            this->openValveForMs(valve, (uint32_t)burstMs);
        }
        else
        {
            delay(1); // couldn't open (six-valve contention) -> yield and retry
        }
    }
}

// Coarse closed-loop control shared by pressure and height modes: commit a direction from the true
// (valve-closed) reading, hold the valve open while a predictor recovers the true bag value from the flowing
// reading, close at goal, settle, and re-decide (bidirectional). Near goal (pressure only) it hands off to
// achieveFineGoal for the exact landing. Full design: AI_TRAINING.md.
void Wheel::goalRoutine() {
    if (flagStartPressureGoalRoutine[thisWheelNum].load())
    {
        delay(100); // wait for all threads to sync on first call

        int8_t dir = FLOW_NONE;                // committed coarse move direction
        Solenoid *valve = nullptr;
        SOLENOID_AI_INDEX aiIndex = AI_MODEL_UNDEFINED;
        bool valveWasOpened = false;
        uint8_t sampleRawBag = 0, sampleRawTank = 0; // flowing readings captured at close

        goalSyncJoin();

        for (;;)
        {
            const bool heightMode = getheightSensorMode();

            // 10 second timeout in case the tank doesn't have enough air, a sensor is stuck, etc.
            if (millis() > this->routineStartTime + ROUTINE_TIMEOUT_MS)
            {
                break;
            }

            this->readInputs();
            const int deadband = heightMode ? LEVEL_DEADBAND_PERCENTAGE : PRESSURE_DEADBAND_PSI;

            if (dir == FLOW_NONE)
            {
                // Valve is closed here, so the reading is the TRUE value. Decide: finish, hand off to the
                // fine precision phase, or commit a coarse fill/dump direction.
                int rawDif = this->pressureGoal - (int)lround(this->getSelectedInputValue());
                if (abs(rawDif) <= deadband)
                {
                    break; // at goal
                }
                // Pressure only: near goal the flowing prediction is too blind to land precisely, so hand
                // off to the fine burst routine. (Height reads true even while flowing -- no fine phase.)
                if (!heightMode && abs(rawDif) <= FINE_PULSE_THRESHOLD_PSI)
                {
                    this->achieveFineGoal();
                    break;
                }
                if (rawDif > 0)
                {
                    dir = FLOW_UP;
                }
                else if (!this->onlyAirUp)
                {
                    dir = FLOW_DOWN;
                }
                else
                {
                    break;
                }
                valve = (dir == FLOW_UP) ? getInSolenoid() : getOutSolenoid();
                (dir == FLOW_UP ? getOutSolenoid() : getInSolenoid())->close();
                aiIndex = valve->getAIIndex();
            }

            // Coarse drive: hold the valve open and recover the true pressure from the flowing reading; stop
            // when it reaches goal.
            double raw = this->getSelectedInputValue();
            double rawTank = heightMode ? 0.0 : getCompressor()->readTankPressureNow(); // pressure-model feature only
            double actual = heightMode ? getPredictedBagHeight(raw)
                                       : getPredictedBagPressure(aiIndex, raw, rawTank);

            bool reached = (dir == FLOW_UP) ? (actual >= this->pressureGoal - deadband)
                                            : (actual <= this->pressureGoal + deadband);
            // In-loop safety ceiling (pressure mode only): never fill past the bag max.
            // TODO: Have this bag pressure check for height mode too. Future improvement.
            bool overCeiling = !heightMode && (dir == FLOW_UP) && (actual >= (double)getbagMaxPressure());

            if (reached || overCeiling)
            {
                valve->close();
                // remember the flowing readings for the offset sample logged after the settle (pressure mode)
                sampleRawBag = (uint8_t)lround(raw);
                sampleRawTank = (uint8_t)lround(rawTank);
                if (overCeiling)
                {
                    break; // safety: stop now, no settle/verify
                }
                // Rendezvous with the valve GUARANTEED closed (INVARIANT: never wait at the barrier with a
                // valve open). A corner that reaches goal early blocks here -- valve closed, harmless -- until
                // every other active corner has also closed; corners still flowing haven't reached this line.
                goalSyncBarrier(GOAL_SYNC_BARRIER_TIMEOUT_MS);

                // Settle to a stable true reading, log the offset sample, then re-decide (bidirectional:
                // continue, reverse, or -- next iteration -- hand off to the fine phase).
                double settled = this->waitForStableReading(heightMode ? SETTLE_STABLE_BAND_LEVEL : SETTLE_STABLE_BAND_PSI);
                if (!heightMode && valveWasOpened)
                {
                    recordLearnSample(aiIndex, sampleRawBag, (uint8_t)lround(settled), sampleRawTank);
                }
                valveWasOpened = false;
                dir = FLOW_NONE;
            }
            else
            {
                bool canOpen = true;
#if SIX_VALVE_MANIFOLD == true
                canOpen = getManifold()->canOpenDirectionSixValveThreadSafe(valve);
#endif
                if (canOpen)
                {
                    valve->open();
                    valveWasOpened = true;
                }
            }

            // Always yield the CPU each tick (the flow-based loop has no other built-in delay).
            delay(1);
        }

        // Close both valves before the exit rendezvous so this thread never waits at the
        // barrier with a valve open (the routine-timeout break above can fire mid-fill).
        getInSolenoid()->close();
        getOutSolenoid()->close();

        // Rendezvous: every active corner has finished the main routine and closed its valves -> all idle.
        goalSyncBarrier(GOAL_SYNC_BARRIER_TIMEOUT_MS);

        // Final cross-corner re-check (pressure mode): a sibling finishing later can nudge an already-done
        // corner through the shared manifold, so with everyone idle, re-read and re-correct for
        // FINAL_RECHECK_ROUNDS synchronized rounds (a barrier per round keeps the reads clean). See AI_TRAINING.md.
        if (!getheightSensorMode())
        {
            for (int rc = 0; rc < FINAL_RECHECK_ROUNDS; rc++)
            {
                double trueVal = this->waitForStableReading(SETTLE_STABLE_BAND_PSI);
                if (abs(this->pressureGoal - (int)lround(trueVal)) > PRESSURE_DEADBAND_PSI)
                {
                    this->achieveFineGoal();
                }
                getInSolenoid()->close();
                getOutSolenoid()->close();
                goalSyncBarrier(GOAL_SYNC_BARRIER_TIMEOUT_MS); // re-sync (all idle) before the next round's read
            }
        }

        // goalRoutine blocks this thread, so reset sensorless baseline / instability manually.
        nullifySensorlessBaseline();
        markInstability(this->getSelectedInputValue());

        goalSyncLeave();

        flagStartPressureGoalRoutine[thisWheelNum] = false;

        if (this->onPressureGoalComplete)
        {
            auto cb = std::move(this->onPressureGoalComplete);
            this->onPressureGoalComplete = nullptr;
            cb();
        }
    }
}

void Wheel::maintainPressure() {
    // Maintain Pressure code
    if (getmaintainPressure())
    {
        if (this->slBaselineCaptured) 
        {
            // if we are in pressure mode, we don't care about stable values. we only want to go up to prevent a leaky bag. If we are in height sensor mode though, we care about stability as this is more meant as a weight levelling function not a leak detector.
            if (!getheightSensorMode() || isPressureStable()) {
                if (this->directlySetPressure > (getheightSensorMode() ? MAINTAIN_PRESSURE_MIN_ACTIVATION_LEVEL : MAINTAIN_PRESSURE_MIN_ACTIVATION_PSI))
                {
                    float dif = this->directlySetPressure - this->getSelectedInputValue();
                    if (getheightSensorMode()) {
                        dif = fabs(dif);
                    }
                    if (dif >= (getheightSensorMode() ? MAINTAIN_PRESSURE_THRESHOLD_LEVEL : MAINTAIN_PRESSURE_THRESHOLD_PSI))
                    {
                        bool success = this->initPressureGoal(this->directlySetPressure, !getheightSensorMode()); // try to go back to the desired pressure
                        if (!success) {
                            Serial.println("Maintain pressure auto-disabled: failed to init pressure goal");
                            setmaintainPressure(false);
                            requestSendConfigBT(); // because we setsensorlessLeveling, ask BLE task to re-broadcast config so UIs reflect OFF
                        }
                    }
                }
            }
        } else {
            // TODO: This whole else statement could be removed if we removed isPressureStable() from pressureCaptureBaseline() but i am not sure if that would be ok to do
            // bag potentially too leaky to settle and grab a baselinem need specifial logic to sync to last preset instead of the baseline since we can't grab one
            if (!getheightSensorMode() && haveValvesBeenClosedForSomeTime(10000)) {
                // We are in pressure mode and we haven't captured a baseline and valves have been closed for 10 seconds longer than the time required to capture a baseline, lets do a more aggressive check for leaks solely based off the last loaded preset (ie the old way to do this)
                int pressureDif = this->pressureGoal - this->getSelectedInputValue();
                if (pressureDif >= MAINTAIN_PRESSURE_THRESHOLD_PSI) {
                    bool success = initPressureGoal(this->pressureGoal, true);
                    if (!success) {
                        Serial.println("Maintain pressure auto-disabled (no baseline): failed to init pressure goal");
                        setmaintainPressure(false);
                        requestSendConfigBT(); // because we setsensorlessLeveling, ask BLE task to re-broadcast config so UIs reflect OFF
                    }
                }
            }
        }
    }
}

void Wheel::markInstability(float current) {
    this->slLastInstabilityDetectedTimeMS = millis();
    this->slLastSample = current;
}

void Wheel::heightsensorlessLevelling() {
    // Sensorless levelling code
    // Holds ride HEIGHT without height sensors by inferring weight change from a sustained, stable
    // per-corner pressure change while parked. When a corner's settled pressure deviates from its
    // user-commanded baseline (directlySetPressure) by more than a threshold, command a correction
    // newTarget = 2*current - start (adding air also raises pressure, hence the 2x). Only height
    // is restored; we then re-baseline to the new commanded target. Tunables in user_defines.h.
    // NOTE: height-sensor mode already levels directly, so this only applies in pressure mode.
    if (getsensorlessLeveling() && !getheightSensorMode())
    {
        float current = this->getSelectedInputValue();

        // Two independent gates must BOTH be satisfied before acting:
        //  1. Non-moving: parked (e-brake/GPS/Accessory wire -
        //     continuously for SENSORLESS_LEVEL_PARKED_DWELL_MS.
        //  2. Pressure stable: the reading has stayed within the band for SENSORLESS_LEVEL_PRESSURE_STABLE_MS.
        // Pressure is only readable with valves closed and no fill routine running.
        if (!isVehicleParked())
        {
            this->slParkedSince = millis(); // not parked -> set time since last park to current time 
            return;
        }

        bool parkedLongEnough = ((millis() - this->slParkedSince) >= SENSORLESS_LEVEL_PARKED_DWELL_MS); // 1. make sure we are parked for 10 seconds

        // Non-moving long enough AND pressure settled long enough AND past cooldown -> evaluate.
        if (parkedLongEnough && 
            isPressureStable() && // we want to check stability here too, because we only want to execute height levelling if the car is stable.
            this->slBaselineCaptured) // 4. make sure we have captured a baseline that we like
        {
            int start = this->directlySetPressure;
            int delta = (int)current - start;
            // don't run if out pressure is below 10psi. 
            if (start > 10 && abs(delta) >= SENSORLESS_LEVEL_THRESHOLD_PSI)
            {
                // newTarget = 2*current - start, with the step clamped to bound 2x noise amplification
                int step = constrain(2 * delta, -SENSORLESS_LEVEL_MAX_STEP_PSI, SENSORLESS_LEVEL_MAX_STEP_PSI);
                int hardMax = min((int)MAX_PRESSURE_SAFETY, (int)getbagMaxPressure());
                int newTarget = constrain(start + step, 0, hardMax);

                // Fault-latch: repeated same-direction corrections look like a slow leak or thermal
                // drift (not real weight changes). Auto-disable to prevent ratcheting to the ceiling.
                int dir = (delta > 0) ? 1 : -1;
                if (this->slSameDirCount != 0 && ((this->slSameDirCount > 0) == (dir > 0)))
                    this->slSameDirCount += dir;
                else
                    this->slSameDirCount = dir;

                if (abs(this->slSameDirCount) >= SENSORLESS_LEVEL_FAULT_LIMIT)
                {
                    setsensorlessLeveling(false);
                    requestSendConfigBT(); // because we setsensorlessLeveling, ask BLE task to re-broadcast config so UIs reflect OFF
                    this->slSameDirCount = 0; // reset this corner; feature is now globally off (other corners keep their own run-length)
                    Serial.println("Sensorless levelling auto-disabled: repeated same-direction corrections (leak/drift suspected)");
                }
                else
                {
                    this->slLastCorrection = millis();
                    bool success = this->initPressureGoal(newTarget); // raises OR lowers (air-out) per sign of delta.
                    if (!success) {
                        setsensorlessLeveling(false);
                        requestSendConfigBT(); // because we setsensorlessLeveling, ask BLE task to re-broadcast config so UIs reflect OFF
                        Serial.println("Sensorless levelling auto-disabled: failed to init pressure goal");
                    }
                }
            }
        }
    }
}

bool Wheel::isPressureStable() {
    return ((millis() - this->slLastInstabilityDetectedTimeMS) >= SENSORLESS_LEVEL_PRESSURE_STABLE_MS);
}

void Wheel::nullifySensorlessBaseline() {
    this->slValvesClosedSince = millis();
    this->slBaselineCaptured = false;
}

void Wheel::trackPressureStability() {
    float current = this->getSelectedInputValue();
    if (fabs(current - this->slLastSample) > (getheightSensorMode() ? SENSORLESS_LEVEL_STABILITY_BAND_LEVEL : SENSORLESS_LEVEL_STABILITY_BAND_PSI))
    {
        markInstability(current);
    }
}

bool Wheel::haveValvesBeenClosedForSomeTime(uint32_t additionalTimeMS) {
    return ((millis() - this->slValvesClosedSince) >= (SENSORLESS_LEVEL_BASELINE_SETTLE_MS + additionalTimeMS));
}

void Wheel::pressureCaptureBaseline()
{
    // grab the baseline value 2 seconds after all valves have closed. We gate on stability because we don't want to accidentally store a baseline from an unstable reading.
    // we specifically use isAnyWheelActive() because we want to be extra strict about pressures changing in any wheel.
    if (isAnyWheelActive())
    {
        nullifySensorlessBaseline(); // a valve is open somewhere -> pressure is in flux, arm a fresh settle window
        markInstability(this->getSelectedInputValue()); // immediately update instability too to nullify it... this should technically be in trackPressureStability BUUUT it saves us 1 call to isAnyWheelActive so i will keep it in here
    }
    else
    {
        if (!this->slBaselineCaptured && isPressureStable() && haveValvesBeenClosedForSomeTime())
        {
            if (getheightSensorMode() && getheightCalMinRide(this->thisWheelNum) > this->getSelectedInputValue()) {
                // we are in height sensor mode and the current height is less than the minimum ride height. We don't want to capture a baseline in this case because it will cause the car to try to stabalize below a ride height
                return;
            }
            this->directlySetPressure = (byte)this->getSelectedInputValue();
            this->slBaselineCaptured = true;
        }
    }
}

void Wheel::loop()
{
    this->readInputs();
    this->goalRoutine();
    this->trackPressureStability();
    this->pressureCaptureBaseline();
    this->maintainPressure();
    this->heightsensorlessLevelling();
    this->captureManualOffsetSample();
}

#if LOG_MANUAL_OFFSET_SAMPLES
// Log an offset sample from a MANUAL valve move (BLE valveControlBittset / gamepad): cache the flowing
// readings while the valve is open, then log the settled bag after it closes. Only runs when no goal
// routine is active and never in height mode. See AI_TRAINING.md.
void Wheel::captureManualOffsetSample()
{
    if (getheightSensorMode() || flagStartPressureGoalRoutine[thisWheelNum].load())
    {
        manualValveWasOpen = false;
        manualSettleUntil = 0;
        return;
    }

    bool upOpen = getInSolenoid()->isOpen();
    bool downOpen = getOutSolenoid()->isOpen();

    if (upOpen || downOpen)
    {
        bool up = upOpen; // if somehow both are open, treat as fill
        this->readInputs();
        manualFlowBag = (uint8_t)lround(this->getSelectedInputValue());
        manualFlowTank = (uint8_t)lround(getCompressor()->readTankPressureNow());
        manualAiIndex = up ? getInSolenoid()->getAIIndex() : getOutSolenoid()->getAIIndex();
        manualUp = up;
        manualValveWasOpen = true;
        manualSettleUntil = 0; // a (re)opened valve cancels any pending settle read
    }
    else if (manualValveWasOpen)
    {
        // Valve just closed: wait for the bag to settle (air-out settles slower) before the settled read.
        manualValveWasOpen = false;
        manualSettleUntil = millis() + (manualUp ? OFFSET_SAMPLE_SETTLE_MS : OFFSET_SAMPLE_SETTLE_DOWN_MS);
    }
    else if (manualSettleUntil != 0 && millis() >= manualSettleUntil)
    {
        // Settled: the flow-induced offset is gone, so the current reading is the settled truth.
        this->readInputs();
        uint8_t settledBag = (uint8_t)lround(this->getSelectedInputValue());
        recordLearnSample(manualAiIndex, manualFlowBag, settledBag, manualFlowTank);
        manualSettleUntil = 0;
    }
}
#else
void Wheel::captureManualOffsetSample() {}
#endif