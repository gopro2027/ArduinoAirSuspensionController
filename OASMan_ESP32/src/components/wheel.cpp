#include "wheel.h"
#include "manifold.h"

#define NUM_WHEEL_THREADS 4
std::atomic<bool> flagStartPressureGoalRoutine[NUM_WHEEL_THREADS];

extern bool isVehicleParked(bool dontTrustEBrakeAlone = false, bool requireBothAccAndEbrake_or_GPS = false); // defined in airSuspensionUtil.cpp
extern bool isAnyWheelActive();            // defined in airSuspensionUtil.cpp

// This function can be updated in the future to use some better algorithm to decide how long to open the valves for to reach the desigred pressure
int getValveTimingSimpleFit(int pressureDifferenceAbsolute)
{
    float valveTimingUntilWithin = 9.993f*pressureDifferenceAbsolute + 0.2189f; // very close to y = 10x, but our lookup table was just ever so slightly diverging from it
    if (valveTimingUntilWithin < 10) {
        return 10; // minimum time to open the valves for
    }
    return valveTimingUntilWithin;
}

int getMinValveOpenPSI()
{
    //return getheightSensorMode() ? 0 : getValveTimingSimpleFit(0)->pressureDelta; // old forumla, would always return 0 (yes, always)
    return 0;
}

// TODO: Turn this into a function based on the values above so that it is smooth, and add a multiplier to change in the settings for people with larger volume bags where it's too slow by default
int calculateValveOpenTimeMS(int pressureDifferenceAbsolute)
{
    return getheightSensorMode() ? 0 : (getValveTimingSimpleFit(pressureDifferenceAbsolute) * ((float)getbagVolumePercentage() / 100.0f)); // Note: Added 0 for height sensor mode but it is unused
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

    float reading = this->readLevelSensorRaw();

    if (calMin > calMax) {
        // we are flipped, so we need to invert the reading
        reading = getHeightSensorMax() - reading;
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

bool Wheel::initPressureGoal(int newPressure, bool onlyAirUp)
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
            flagStartPressureGoalRoutine[thisWheelNum] = true;
            return true;
        }
    }
    return false;
}

bool Wheel::initPressureGoal(int newPressure)
{
    return this->initPressureGoal(newPressure, false);
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

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::goalRoutine() {
    if (flagStartPressureGoalRoutine[thisWheelNum].load())
    {
        delay(100); // wait for all threads to sync on first call. 6-19-2025: I actually have no clue if this is required. The 100 is the same as the delay in the task.

        // const double oscillation = 1.359142965358979; //e/2 seems like a decent value tbh
        // const double oscillation = 1.75;
        const double oscillation = 1.2;
        int oscillationPow = 0;
        const int startIteration = -1;
        int iteration = startIteration; // - values make it skip the first generation. It won't start dividing until iteration = 1
        const int fullAirOutTime = 5000;
        bool previousDirection = false;
        for (;;)
        {
            // 10 second timeout in case tank doesn't have a whole lot of air or something
            if (millis() > this->routineStartTime + ROUTINE_TIMEOUT_MS)
            {
                break;
            }

            // Main routine
            this->readInputs();
            int pressureDif = this->pressureGoal - this->getSelectedInputValue();
            int pressureDifABS = abs(pressureDif);

            if (pressureDifABS > getMinValveOpenPSI())
            {
                // Decide which valve to use
                Solenoid *valve;
                bool up = pressureDif >= 0;
                if (up)
                {
                    valve = getInSolenoid();
                    getOutSolenoid()->close();
                }
                else
                {
                    if (this->onlyAirUp) {
                        // we are done, we don't want to air out
                        break;
                    }
                    valve = getOutSolenoid();
                    getInSolenoid()->close();
                }

                if (!getheightSensorMode())
                {
                    // Pressure sensor logic. You can't read the pressure accurately with the valve open. Essentially must open the valve for a guesstimate amount of time, then close it, then you are able to read the pressure.

                    // Choose time to open for
                    int valveTime = calculateValveOpenTimeMS(pressureDifABS);

                    // right now not going to use this because it doesn't seem to work super well up air up. Results in super low values. Need to do more testing
                    // int valveTime = this->calculatePressureTimingReal(valve);

                    double start_pressure = this->getSelectedInputValue();
                    double end_pressure = this->pressureGoal;
                    double tank_pressure = getCompressor()->getTankPressure();

                    if (canUseAiPrediction(valve->getAIIndex()))
                    {

                        // conversion float to int eliminates inf and nan
                        int aiPredict = getAiPredictionTime(valve->getAIIndex(), start_pressure, end_pressure, tank_pressure);

                        // There are some valid scenarios where we can get inf or nan if say the tank pressure is lower than the end pressure
                        if (aiPredict < 5000 && aiPredict > 0)
                        {
                            valveTime = aiPredict;
                        }
                    }

                    // To help prevent ocellations, check if previous direction is different than new direction. Ex: was going up, but suddently now is going down. It must have jumped over goal. Go ahead and start dividing valve time by (oscillation ^ oscillationPow)
                    if (iteration > startIteration)
                    {
                        if (previousDirection != up)
                        {
                            oscillationPow++;
                        }
                    }
                    valveTime = valveTime / std::pow(oscillation, oscillationPow);

                    // save previous direction.
                    previousDirection = up;

                    // If the goal pressure is 0 or 1psi, go ahead and just open the valve for a long time to ensure a smooth air out
                    bool specialSmoothAirOut = false;
                    if (!up && pressureGoal < 2 && valveTime < fullAirOutTime)
                    {
                        valveTime = fullAirOutTime;
                        specialSmoothAirOut = true;
                    }

                    if (valveTime > 0)
                    {
                        const int valveSettleTime = 250; // ms to wait after closing valve to allow pressure to stabilize a bit before reading again
                        bool canOpen = true;
                        #if SIX_VALVE_MANIFOLD == true
                        if (!getManifold()->canOpenDirectionSixValveThreadSafe(valve)) {
                            canOpen = false;
                            iteration--;// don't count this as an iteration since we decided at last moment to skip it due to the other chamber being in use
                            delay(valveSettleTime); // delay 250 to at least get some resemblence of matching the other valves that are opening
                        }
                        #endif

                        if (canOpen) {
                            // Open valve for calculated time
                            valve->open();
                            delay(valveTime);
                            valve->close();

                            // Sleep 150ms to allow time for valve to fully close and pressure to equalize a bit
                            delay(valveSettleTime); // Changed to 250. 150 was... confusing

                            // only bother saving data for first 2 iterations AND when the valve was opened for more than 10ms AND it wasn't just set to do a special low value full smooth air out AND if the pressure change is greater than 3psi
                            if (iteration < startIteration + 2 && valveTime > 10 && !specialSmoothAirOut)
                            {
                                this->readInputs();
                                end_pressure = this->getSelectedInputValue(); // gonna be slightly different than the pressureGoal
                                if (abs(start_pressure - end_pressure) > 3)
                                {
                                    appendPressureDataToFile(valve->getAIIndex(), start_pressure, end_pressure, tank_pressure, valveTime);
                                }
                            }
                        }
                    }
                    else
                    {
                        // calculated valve time is 0 so just break out of loop
                        break;
                    }
                }
                else
                {
                    // for level sensors, just open the valve and read level sensors while valves are open since it doesn't affect the reading
                    valve->open();
                    delay(1);
                }
            }
            else
            {
                // Completed
                break;
            }
            iteration++;

            custom_barrier_wait(count_participants());
        }

        // since this function (goalRoutine) is blocking the same thread, we must manually reset sensorless baseline and mark instability. If we had trackPressureStability() and pressureCaptureBaseline() in a different thread, we wouldn't need to do this.
        nullifySensorlessBaseline();
        markInstability(this->getSelectedInputValue());

        custom_barrier_wait(count_participants()); // needed here because when it breaks out of the loop it needs to hit it one last time.

        flagStartPressureGoalRoutine[thisWheelNum] = false;
        // close both after (only applies for level sensor logic)
        getInSolenoid()->close();
        getOutSolenoid()->close();
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
        if (!this->slBaselineCaptured && isPressureStable() && ((millis() - this->slValvesClosedSince) >= SENSORLESS_LEVEL_BASELINE_SETTLE_MS))
        {
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
}