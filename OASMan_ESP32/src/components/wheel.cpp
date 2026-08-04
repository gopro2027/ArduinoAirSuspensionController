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

// barrier code (for syncing wheel threads) generated from chat-gpt because I don't quite understand it but it seems to work fine lololol
std::atomic<int> waiting_threads(0);
std::atomic<int> generation(0);

int count_participants()
{
    // Count how many threads are currently running
    int currently_active = 0;
    for (int i = 0; i < NUM_WHEEL_THREADS; ++i)
    {
        if (flagStartPressureGoalRoutine[i].load())
        {
            currently_active++;
        }
    }
    return currently_active;
}

void custom_barrier_wait(int num_participants)
{
    int gen = generation.load();
    waiting_threads.fetch_add(1);

    if (waiting_threads.load() == num_participants)
    {
        // Last thread arrives, reset and let others go
        wheelThreadLock();
        waiting_threads.store(0);
        generation.fetch_add(1);
        wheelThreadUnlock();
    }
    else
    {
        // Poll until generation changes
        while (generation.load() == gen)
        {
            delay(1);
        }
    }
}

static const int8_t FLOW_NONE = 0;
static const int8_t FLOW_UP = 1;
static const int8_t FLOW_DOWN = -1;

// Closed-loop control shared by pressure and height modes: commit a fill/dump direction from the true
// (valve-closed) reading, hold the valve open while the live reading is run through a predictor to recover
// the true bag value, and stop when it reaches goal. On close the value settles and is re-verified,
// correcting under/overshoot in either direction. Pressure mode's predictor applies the learned sensor
// offset and logs a flowing->settled sample each close; the level sensor reads true even during flow, so
// height mode's predictor is an identity stub and nothing is logged. See AI_TRAINING.md.
void Wheel::goalRoutine() {
    if (flagStartPressureGoalRoutine[thisWheelNum].load())
    {
        delay(100); // wait for all threads to sync on first call

        int8_t dir = FLOW_NONE;                // committed move direction
        Solenoid *valve = nullptr;
        SOLENOID_AI_INDEX aiIndex = AI_MODEL_UNDEFINED;
        bool valveWasOpened = false;
        uint8_t sampleRawBag = 0, sampleRawTank = 0; // flowing readings captured at close
        uint32_t settleUntil = 0; // while non-zero: valve closed, waiting to verify the settled pressure

        for (;;)
        {
            const bool heightMode = getheightSensorMode();

            // 10 second timeout in case the tank doesn't have enough air, a sensor is stuck, etc.
            if (millis() > this->routineStartTime + ROUTINE_TIMEOUT_MS)
            {
                break;
            }

            this->readInputs();

            // Settle/verify: after the predicted value reaches goal the valve is closed while the bag
            // settles; once settled, drop the committed direction so the control block below re-checks
            // against the TRUE (valve-closed) reading and finishes, continues, or reverses. Bidirectional.
            if (settleUntil != 0)
            {
                if (millis() >= settleUntil)
                {
                    double settled = this->getSelectedInputValue();
                    if (!heightMode && valveWasOpened)
                    {
                        // pressure mode only: log the flowing->settled offset sample for the model
                        recordLearnSample(aiIndex, sampleRawBag, (uint8_t)lround(settled), sampleRawTank);
                    }
                    valveWasOpened = false;
                    settleUntil = 0;
                    dir = FLOW_NONE; // re-evaluate from the settled reading (top-of-loop check breaks out or corrects)
                }
            }

            if (settleUntil == 0)
            {
                int deadband = heightMode ? LEVEL_DEADBAND_PERCENTAGE : PRESSURE_DEADBAND_PSI;
                double raw = this->getSelectedInputValue();
                // tank reading is only a pressure-model feature; skip the ADC read in height mode
                double rawTank = heightMode ? 0.0 : getCompressor()->readTankPressureNow();

                if (dir == FLOW_NONE)
                {
                    // Valve still closed here, so raw is the true settled value. Commit a direction from it
                    // (the pressure predictor is direction-specific, so it can't run before a direction is
                    // chosen); every later tick uses the predicted value below.
                    int rawDif = this->pressureGoal - (int)lround(raw);
                    if (abs(rawDif) <= deadband)
                    {
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

                // Recover the true bag value from the live (flowing) reading. Pressure mode applies the
                // learned/default sensor offset; the level sensor reads true even during flow, so height
                // mode's predictor is an identity stub (getPredictedBagHeight).
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
                    // air-out settles slower than air-up, so give it longer before the verify read
                    uint32_t settleMs = (dir == FLOW_DOWN) ? OFFSET_SAMPLE_SETTLE_DOWN_MS : OFFSET_SAMPLE_SETTLE_MS;
                    settleUntil = millis() + settleMs; // verify the settled value next
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
            }

            delay(1);
            custom_barrier_wait(count_participants());
        }

        // goalRoutine blocks this thread, so reset sensorless baseline / instability manually.
        nullifySensorlessBaseline();
        markInstability(this->getSelectedInputValue());

        custom_barrier_wait(count_participants()); // one final barrier so all exiting threads rendezvous

        flagStartPressureGoalRoutine[thisWheelNum] = false;
        getInSolenoid()->close();
        getOutSolenoid()->close();

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
// Log an offset sample from a MANUAL valve move (BLE valveControlBittset / gamepad). goalRoutine
// self-collects its own samples, so only track when no goal routine is running (and never in
// height-sensor mode). While a manual valve is open we cache the flowing readings; when it closes we
// read the settled bag and log {flowing bag/tank, settled bag}. Runs on the wheel task, so
// the ADC reads / SPIFFS write are safe. See AI_TRAINING.md.
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