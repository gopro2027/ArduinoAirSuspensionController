#include "wheel.h"
#include "manifold.h"

#define NUM_WHEEL_THREADS 4
std::atomic<bool> flagStartPressureGoalRoutine[NUM_WHEEL_THREADS];

extern bool isVehicleParked(bool strict); // defined in airSuspensionUtil.cpp
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

void Wheel::readInputs()
{
    this->pressureValue = readPinPressure(this->pressurePin, false);
    if (getheightSensorMode())
    {
        this->levelValue = readPinPressure(this->levelSensorPin, true);
        if (getheightSensorInvertBits() & (1 << this->thisWheelNum))
        {
            this->levelValue = 100.0f - this->levelValue;
        }
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
    return getInSolenoid()->isOpen() || getOutSolenoid()->isOpen();
}

void Wheel::initPressureGoal(int newPressure)
{

    if (newPressure > (getheightSensorMode() ? getHeightSensorMax() * 1.03f : getbagMaxPressure()))
    {
        // hardcode not to go above set psi
        return;
    }
    int pressureDif = newPressure - this->getSelectedInputValue(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI())
    {
        // okay we need to set the values, but only if we are airing out or if the tank has more pressure than what is currently in the bags
        bool tankIsCapable = true;
        if (getheightSensorMode() == false)
        {
            // it's in pressure mode, check if tank is less than whats currently in the bag and if it is then tankIsCapable is false.
            // we don't care about the goal because we still want to try to reach the goal even if the tank isn't capable. We do care if tank is more than the bag though because if it's less than the bag then it will actually air out.
            if (getCompressor()->getTankPressure() < this->getSelectedInputValue())
            {
                tankIsCapable = false;
            }
        }
        if (pressureDif < 0 || tankIsCapable)
        {
            this->pressureGoal = newPressure;
            this->routineStartTime = millis();
            flagStartPressureGoalRoutine[thisWheelNum] = true;
        }
    }
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
        // only run if pressure is higher than 10psi... also prevents it when a preset is not yet loaded (0)
        if (this->pressureGoal > 10)
        {
            int pressureDif = this->pressureGoal - this->getSelectedInputValue();
            if (pressureDif >= 10)
            {                                         // 10 psi difference
                initPressureGoal(this->pressureGoal); // try to go back to the desired pressure
            }
        }
    }
}

void Wheel::heightsensorlessLevelling() {
    // Sensorless levelling code
    // Holds ride HEIGHT without height sensors by inferring weight change from a sustained, stable
    // per-corner pressure change while parked. When a corner's settled pressure deviates from its
    // user-commanded baseline (startWeightPressure) by more than a threshold, command a correction
    // newTarget = 2*current - start (adding air also raises pressure, hence the 2x). Only height
    // is restored; we then re-baseline to the new commanded target. Tunables in user_defines.h.
    // NOTE: height-sensor mode already levels directly, so this only applies in pressure mode.
    if (getsensorlessLeveling() && !getheightSensorMode())
    {
        float current = this->getSelectedInputValue();

        // Two independent gates must BOTH be satisfied before acting:
        //  1. Non-moving: strictly parked (e-brake/GPS - false on accessory-only boards so it won't
        //     engage) continuously for SENSORLESS_LEVEL_PARKED_DWELL_MS.
        //  2. Pressure stable: the reading has stayed within the band for SENSORLESS_LEVEL_PRESSURE_STABLE_MS.
        // Pressure is only readable with valves closed and no fill routine running.
        bool parked = isVehicleParked(true);
        if (!parked)
        {
            this->slParkedSince = 0; // not parked -> reset both dwell timers
            this->slStableSince = 0;
        }
        else
        {
            if (this->slParkedSince == 0)
                this->slParkedSince = millis(); // parked just began

            bool readable = !flagStartPressureGoalRoutine[thisWheelNum].load() && !this->isActive();
            if (!readable)
            {
                this->slStableSince = 0; // can't read pressure while a valve is open / routine runs
            }
            // (Re)start the pressure-stability window whenever the reading wanders outside the band.
            else if (this->slStableSince == 0 || fabs(current - this->slLastSample) > SENSORLESS_LEVEL_STABILITY_BAND_PSI)
            {
                this->slStableSince = millis();
                this->slLastSample = current;
            }
        }

        bool parkedLongEnough = this->slParkedSince != 0 && (millis() - this->slParkedSince >= SENSORLESS_LEVEL_PARKED_DWELL_MS);
        bool pressureStableLongEnough = this->slStableSince != 0 && (millis() - this->slStableSince >= SENSORLESS_LEVEL_PRESSURE_STABLE_MS);
        bool cooldownPassed = (millis() - this->slLastCorrection >= SENSORLESS_LEVEL_COOLDOWN_MS);

        // Non-moving long enough AND pressure settled long enough AND past cooldown -> evaluate.
        if (parkedLongEnough && pressureStableLongEnough && cooldownPassed)
        {
            int start = startWeightPressure[this->thisWheelNum]; // global variable
            int delta = (int)current - start;
            // start > 10 mirrors the maintainPressure guard and blocks action before a baseline exists (0)
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
                    startWeightPressure[this->thisWheelNum] = (byte)newTarget; // feature re-baseline (NOT via capture hooks)
                    this->slLastCorrection = millis();
                    this->slStableSince = 0;           // force a fresh stability window after actuating
                    this->initPressureGoal(newTarget); // raises OR lowers (air-out) per sign of delta
                }
            }
        }
    }
}

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::sensorlessCaptureBaseline()
{
    // Whenever all valves close and stay closed long enough for pressure to settle, snapshot the
    // actual settled pressure as this corner's sensorless baseline ("pressure at start weight").
    // This is the canonical baseline - it reflects what the vehicle truly settled at after ANY valve
    // activity (preset, manual, or a sensorless correction), not just a commanded target. A passive
    // weight change opens no valves, so it never re-baselines here, which is exactly what lets
    // heightsensorlessLevelling() detect the resulting pressure delta. Pressure mode only.
    if (getheightSensorMode())
    {
        return;
    }

    if (isAnyWheelActive())
    {
        // a valve is open somewhere -> pressure is in flux, arm a fresh settle window
        this->slValvesClosedSince = 0;
        this->slBaselineCaptured = false;
    }
    else
    {
        if (this->slValvesClosedSince == 0)
        {
            this->slValvesClosedSince = millis(); // all valves just became closed
        }
        else if (!this->slBaselineCaptured && (millis() - this->slValvesClosedSince >= SENSORLESS_LEVEL_BASELINE_SETTLE_MS))
        {
            startWeightPressure[this->thisWheelNum] = (byte)this->getSelectedInputValue();
            this->slBaselineCaptured = true; // capture once per valve-close event
        }
    }
}

void Wheel::loop()
{
    this->readInputs();
    this->goalRoutine();
    this->maintainPressure();
    this->sensorlessCaptureBaseline();
    this->heightsensorlessLevelling();
}