#include "airSuspensionUtil.h"
#include "manifoldSaveData.h"

#pragma region variables

InputType *pressureInputs[5];
Manifold *manifold;
Compressor *compressor;
AuxillaryOutput *auxillaryOutput;
RfReceiver *rfReceiver;
Wheel *wheel[4];

Adafruit_ADS1115 ADS1115A;
Adafruit_ADS1115 ADS1115B;
Adafruit_ADS1115 ADS1115C;
Adafruit_ADS1115 ADS1115D;
bool ADS1115C_exists;
bool ADS1115D_exists;

Manifold *getManifold()
{
    return manifold;
}
Compressor *getCompressor()
{
    return compressor;
}
AuxillaryOutput *getAuxillaryOutput()
{
    return auxillaryOutput;
}
RfReceiver *getRfReceiver()
{
    return rfReceiver;
}
// InputType *manifoldSafetyWire;

Wheel *getWheel(int i)
{
    return wheel[i];
}

#pragma endregion

#pragma region initialization

void initializeADS()
{
    if (!ADS1115A.begin(ADS_A_ADDRESS))
    {
        Serial.println(F("Failed to initialize ADS A (pressure sensors)"));
#if ADS_MOCK_BYPASS == false
        ESP.restart();
#endif
    }

    if (!ADS1115B.begin(ADS_B_ADDRESS))
    {
        Serial.println(F("Failed to initialize ADS B (height sensors)"));
#if ADS_MOCK_BYPASS == false
        ESP.restart();
#endif
    }

    if (!ADS1115C.begin(ADS_C_ADDRESS))
    {
        ADS1115C_exists = false;
        Serial.println(F("Failed to initialize ADS C (rf receiver)"));
    } else {
        ADS1115C_exists = true;
    }

    if (!ADS1115D.begin(ADS_D_ADDRESS))
    {
        ADS1115D_exists = false;
        Serial.println(F("Failed to initialize ADS D (tank & other)"));
    } else {
        ADS1115D_exists = true;
    }

}

void setupManifold()
{
    initializeADS();

#if SIX_VALVE_MANIFOLD == false

    manifold = new Manifold(
        solenoidFrontPassengerInPin,
        solenoidFrontPassengerOutPin,
        solenoidRearPassengerInPin,
        solenoidRearPassengerOutPin,
        solenoidFrontDriverInPin,
        solenoidFrontDriverOutPin,
        solenoidRearDriverInPin,
        solenoidRearDriverOutPin);

#else

    manifold = new Manifold(
        m6_solenoidFrontPassengerPin,
        m6_solenoidRearPassengerPin,
        m6_solenoidFrontDriverPin,
        m6_solenoidRearDriverPin,
        m6_solenoidChamberTankPin,
        m6_solenoidChamberExhaustPin);

#endif
}

#pragma endregion

#pragma region ebrake

#if EBRAKE_WIRE_FUNCTIONALITY

const int ebrakeSampleSize = AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE == true ? 2 : 20; // when normal ebrake on, use long sample of 20 samples (2 seconds). When doing double lock mode, only take 2 samples (100ms per sample) to make it a bit more snappy on time
bool ebrakeHistory[ebrakeSampleSize];
int ebrakeCounter = 0;
bool ebrakeOn = false;

InputType *ebrakeWire;
void ebrakeWireSetup()
{
    ebrakeWire = ebrakeInput;
}

void ebrakeWireLoop()
{
    // when e brake is not engaged, the 3.3v through the 10k resistor goes to the esp32. That means we get a high reading when ebrake is off. When ebrake is on, we get a low reading
    bool previousReading = ebrakeOn;
    sampleReading(ebrakeOn, ebrakeWire->digitalRead() == LOW, ebrakeHistory, ebrakeCounter, ebrakeSampleSize);

#if ENABLE_AIR_OUT_ON_SHUTOFF
#if AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE

    if (!isVehicleOn())
    {
        static long lastOnTime = millis();
        if (ebrakeOn) // important to use ebrakeOn instead of isEBrakeOn() because isEBrakeOn() will always return false when AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE is enabled
        {
            long currentReadTime = millis();
            if (previousReading == false)
            {
                if (currentReadTime - lastOnTime < AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE_TIME)
                {
                    // pressed twice within timeframe
                    // check air out on shutoff enabled
                    if (getairOutOnShutoff())
                    {
                        loadProfileAirUp(0); // packet 0 should be the lowest setting!
                    }
                }
            }
            lastOnTime = currentReadTime;
        }
    }
#endif
#endif
}

bool isEBrakeOn()
{
    #if AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE
    return false;
    #endif
    return ebrakeOn;
}

#else

void ebrakeWireSetup()
{
}
void ebrakeWireLoop()
{
}
bool isEBrakeOn()
{
    return false;
}

#endif

#pragma endregion

#pragma region speed_detection

// TODO: Implement GPS functionality
bool gps_exists = false;
int getVehicleSpeed() {
    return 0;
}
bool isGPSCurrentlyAccurate() { // this will error on the side of caution, so we can always be sure our gps readings are accurate/safe
    return true;
}

// This function should not be used as a trigger on it's own, it should only be used as a check upon user interaction.
// 'dontTrustEBrakeAlone' will make it so it skips the ebrake check by ittself, because in a stricter moder we may not want to trust only the ebrake
// 'requireBothAccAndEbrake' will make it so it requires both the accessory wire and the ebrake to be on to return true. GPS will still return true on it's own tho if it is working. 
bool isVehicleParked(bool dontTrustEBrakeAlone, bool requireBothAccAndEbrake_or_GPS) {

    if (requireBothAccAndEbrake_or_GPS) {
        dontTrustEBrakeAlone = true; // we are pairing both the acc and ebrake together, so skip the single ebrake check.
    }
        
    // pretty simple, if gps is working and we are moving less than 3mph, we will assume we are parked. This is highly reliable since we have essentially 100% accuracy knowing if the gps is working or not.
    if (gps_exists && isGPSCurrentlyAccurate()) {
        if (getVehicleSpeed() < 4) {
            return true; // vehicle is parked if less than 5mph
        } else {
            return false; // vehicle is moving more than 5mph, so it is not parked.
        }
    }

    // don't trust e-brake if we are in strict mode
    if (dontTrustEBrakeAlone == false) {
        // ebrake is slightly less of a reliable source of whether or not the vehicle is parked because it is highly likely a user may try to bypass it, so we put it after the gps check.
        if (isEBrakeOn()) { //I consider this safe because for isEBrakeOn to return true, it must have been on for at least 2 seconds. 
            return true; // if the ebrake is on, we can assume the vehicle is parked
        }
        if (!isEBrakeOn()) {
            // ebrake being disengaged doesn't necessarily mean the vehicle is driving or parked, it could be either, so don't return anything here.
        }
    }

    #if ACCESSORY_WIRE_FUNCTIONALITY
    // We can assume that if the vehicle is off, it is parked, and if it is on, it is not parked.
    return isVehicleOn() == false && (requireBothAccAndEbrake_or_GPS == true ? isEBrakeOn() : true);
    #endif

    // We don't know basically anything, so return false. This is for v2 firmwares because they don't have acc.
    return false;

}

#pragma endregion

#pragma region wheel_functions

bool isAnyWheelActive()
{
    for (int i = 0; i < 4; i++)
    {
        if (getWheel(i)->isActive())
        {
            return true;
        }
    }
    return false;
}

// function renamed to 'bag stretch'
static void initPressureGoalWithStretch(byte wheelNum, int goal)
{
    Wheel *w = getWheel(wheelNum);
    int belowP = getAirUpBagStretchTriggerBelowPressure();
    int unroll = getAirUpBagStretchPressure();
    // the functionality of this feature was changed a bit from the original idea. I've kind of deemed the original idea of 'going past the pressure' kind of useless on it's own, as it's just going to make almost every time you go to a preset be inaccurate.
    // instead what this function will do is check if the current pressure is below the AirUpBagStretchTriggerBelowPressure, which by default is 40psi, and if it is, it will go to the AirUpBagStretchPressure pressure first in order to stretch the bag out.
    if (!getheightSensorMode() && // don't do this feature if height sensor mode is enabled
        unroll > 0 && // unroll being 0 is functionally disabled
        goal < unroll && // only run if the goal pressure is below stretch pressure, otherwise no need to stretch if it's already going past it
        goal > w->getSelectedInputValue() && // only run if we are airing up
        w->getSelectedInputValue() < belowP) // only run if the current pressure is below the AirUpBagStretchTriggerBelowPressure
    {
        w->initPressureGoal(unroll, true, [w, goal]()
                            { w->initPressureGoal(goal); }); //this could technically be initPressureGoalWithStretch (recursive) but with a chance of an infinite loop, so we will go straight to initPressureGoal for now unless we want to add more functionality and failsafe checks to initPressureGoalWithStretch in the future
    }
    else
    {
        w->initPressureGoal(goal);
    }
}

void loadProfileAirUp(int profileIndex)
{
    if (profileIndex > MAX_PROFILE_COUNT)
    {
        return;
    }
    // load profile then air up
    ProfileRaw p = readProfile(profileIndex);
    initPressureGoalWithStretch(WHEEL_FRONT_PASSENGER, p.pressure[WHEEL_FRONT_PASSENGER]);
    initPressureGoalWithStretch(WHEEL_REAR_PASSENGER, p.pressure[WHEEL_REAR_PASSENGER]);
    initPressureGoalWithStretch(WHEEL_FRONT_DRIVER, p.pressure[WHEEL_FRONT_DRIVER]);
    initPressureGoalWithStretch(WHEEL_REAR_DRIVER, p.pressure[WHEEL_REAR_DRIVER]);
}

void airOutWithSafetyCheck()
{
#if ENABLE_AIR_OUT_ON_SHUTOFF
#if !AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE
    // regarding the second parameter, true means we require both the accessory wire and the ebrake to be on to return true. This function already executes within the acc being off, but we need that double check of the ebrake also being enabled. If we didn't have the hard ebrake check then this would always return true because the acc is off, but we want to be extremely strict here and verify that both the car is off and the ebrake is pulled. (GPS will still return true on it's own tho if it is enabled)
    if (isVehicleParked(true, true))
    {
        if (getairOutOnShutoff())
        {
            loadProfileAirUp(0); // packet 0 should be the lowest setting!
        }
    }
#endif
#endif
}

#pragma endregion

#pragma region accessory_wire

bool vehicleOn = true; // want this to be true to begin so it sets the current time first thing so it knows it was on and doesn't run the off routine first.
unsigned long lastTimeLive = 0;
bool isVehicleOn()
{
    return vehicleOn;
}

bool isKeepAliveTimerExpired()
{
    return millis() > (lastTimeLive + getsystemShutoffTimeM() * 60 * 1000);
}

bool isKeepAliveTimeReadyForShutdownRoutine()
{
    return millis() > (lastTimeLive + AIR_OUT_AFTER_SHUTDOWN_MS); // 5 seconds before choosing it is actually shut down... sure why not
}

// in the future we can call this from bluetooth functions to keep the device alive longer if actively using bluetooth while the vehicle is off
void notifyKeepAlive()
{
    lastTimeLive = millis();
}

bool forceShutoff = false;

#ifdef ACCESSORY_WIRE_FUNCTIONALITY

InputType *accessoryWire;
InputType *outputKeepESPAlive;
void accessoryWireSetup()
{
    accessoryWire = accessoryInput;
    outputKeepESPAlive = outputKeepAlivePin;
    outputKeepESPAlive->digitalWrite(LOW);
    lastTimeLive = millis();
}
const int accessoryWireSampleSize = 5;
bool vehicleOnHistory[accessoryWireSampleSize];
int vehicleOnCounter = 0;
bool hasJustShutoff = true;
void accessoryWireLoop()
{
    bool previousVehicleOn = vehicleOn;
    sampleReading(vehicleOn, accessoryWire->digitalRead() == HIGH, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
    if (isVehicleOn())
    {
        if (previousVehicleOn == false) {
            getAuxillaryOutput()->setDoStartupEvent(true);
        }
        // accessory wire is supplying 12v (car on)
        notifyKeepAlive();
        hasJustShutoff = false;
    }
    else
    {
        // vehicle is off... first check if 5 seconds have passed
        if (isKeepAliveTimeReadyForShutdownRoutine())
        {
            // one time check that it just happened for the first time
            if (hasJustShutoff == false)
            {
                hasJustShutoff = true;
                // actually check if air out code is enabled and do as asked
                airOutWithSafetyCheck();
                getAuxillaryOutput()->setDoShutdownEvent(true);
            }
        }
    }
    if (isKeepAliveTimerExpired() || forceShutoff)
    {
        Serial.println("Shutting down");
        outputKeepESPAlive->digitalWrite(LOW); // acc wire has been off for some time, shut down system
        delay(500);
    }
    else
    {
        outputKeepESPAlive->digitalWrite(HIGH);
    }
    forceShutoff = false;
}

#else
void accessoryWireSetup()
{
    vehicleOn = true;
}
void accessoryWireLoop()
{
    vehicleOn = true;
}
#endif

#pragma endregion

void downAllBags(int time)
{
    manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_OUT)->open();
    manifold->get(SOLENOID_INDEX::REAR_PASSENGER_OUT)->open();
    manifold->get(SOLENOID_INDEX::FRONT_DRIVER_OUT)->open();
    manifold->get(SOLENOID_INDEX::REAR_DRIVER_OUT)->open();
    delay(time);
    manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_OUT)->close();
    manifold->get(SOLENOID_INDEX::REAR_PASSENGER_OUT)->close();
    manifold->get(SOLENOID_INDEX::FRONT_DRIVER_OUT)->close();
    manifold->get(SOLENOID_INDEX::REAR_DRIVER_OUT)->close();
}

void upAllBags(int time)
{
    manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_IN)->open();
    manifold->get(SOLENOID_INDEX::REAR_PASSENGER_IN)->open();
    manifold->get(SOLENOID_INDEX::FRONT_DRIVER_IN)->open();
    manifold->get(SOLENOID_INDEX::REAR_DRIVER_IN)->open();
    delay(time);
    manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_IN)->close();
    manifold->get(SOLENOID_INDEX::REAR_PASSENGER_IN)->close();
    manifold->get(SOLENOID_INDEX::FRONT_DRIVER_IN)->close();
    manifold->get(SOLENOID_INDEX::REAR_DRIVER_IN)->close();
}

double averageBags()
{
    getWheel(WHEEL_FRONT_PASSENGER)->readInputs();
    getWheel(WHEEL_REAR_PASSENGER)->readInputs();
    getWheel(WHEEL_FRONT_DRIVER)->readInputs();
    getWheel(WHEEL_REAR_DRIVER)->readInputs();
    return (double)(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue() + getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue() + getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue() + getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue()) / 4.0;
}

#pragma region pressure sensor learn

namespace PressureSensorCalibration
{

    void getPinPressures(float pressures[5])
    {
        for (int i = 0; i < 5; i++)
        {
            pressures[i] = readPinPressure(pressureInputs[i], false);
        }
    }

    bool isAnyBagMaxxed(float pressures[5])
    {
        for (int i = 0; i < 5; i++)
        {
            if (pressures[i] > 100)
                return true;
        }
        return false;
    }

    int getHighestPressureIndex(float pressures[5])
    {
        int highestIndex = 0;
        float highestNum = pressures[0];
        for (int i = 1; i < 5; i++)
        {
            if (pressures[i] > highestNum)
            {
                highestIndex = i;
                highestNum = pressures[i];
            }
        }
        return highestIndex;
    }

    int getHighestChangedPressureIndex(float previousPressures[5])
    {
        float changedPressures[5];
        getPinPressures(changedPressures);
        for (int i = 0; i < 5; i++)
        {
            changedPressures[i] = changedPressures[i] - previousPressures[i];
        }
        return getHighestPressureIndex(changedPressures);
    }

    void doBagPressureCheck(int &saveTo, Solenoid *solenoid)
    {
        float startPressures[5];
        getPinPressures(startPressures);
        solenoid->open();
        delay(2 * 1000); // 2 seconds probably fine
        solenoid->open();
        delay(500); // wait for pressures to stabilize
        saveTo = getHighestChangedPressureIndex(startPressures);
    }

    void learnPressureSensorsRoutine()
    {
        // Assume pressure sensor value is correct. ie default 232.
        int IDX_FRONT_PASSENGER;
        int IDX_REAR_PASSENGER;
        int IDX_FRONT_DRIVER;
        int IDX_REAR_DRIVER;
        int IDX_TANK;

        // STEP 1: Air out all bags
        downAllBags(20 * 1000);

        delay(500); // wait for pressures to stabilize

        float pressures[5];
        getPinPressures(pressures);

        // STEP 2: Fill up tank with compressor until we get 100 as our reading or for 30 seconds
        // first check if we already have one pressure that is full (aka tank is already full)
        if (!isAnyBagMaxxed(pressures))
        {
            compressor->getOverrideSolenoid()->open();
            unsigned long startTime = millis();
            while (millis() < startTime + 30 * 1000)
            {
                getPinPressures(pressures);
                // check tank pressure just in case it's already full
                if (isAnyBagMaxxed(pressures))
                {
                    // turn off compressor if any are over 100
                    compressor->getOverrideSolenoid()->close();
                }
                delay(5);
            }
            compressor->getOverrideSolenoid()->close();
        }

        // STEP 3: Tank sensor is the sensor with the highest pressure
        IDX_TANK = getHighestPressureIndex(pressures);

        // STEP 4: Do routine for each bag
        // TODO: Finish this and then implement the controller side/bluetooth
        delay(500);
        doBagPressureCheck(IDX_FRONT_PASSENGER, manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_IN));
        doBagPressureCheck(IDX_FRONT_DRIVER, manifold->get(SOLENOID_INDEX::FRONT_DRIVER_IN)); // this used to be the 3rd one but i changed it to the second because generally we want to air up the front first to avoid cracking a splitter
        doBagPressureCheck(IDX_REAR_PASSENGER, manifold->get(SOLENOID_INDEX::REAR_PASSENGER_IN));
        doBagPressureCheck(IDX_REAR_DRIVER, manifold->get(SOLENOID_INDEX::REAR_DRIVER_IN));

        setpressureInputTank(IDX_TANK);
        setpressureInputFrontPassenger(IDX_FRONT_PASSENGER);
        setpressureInputRearPassenger(IDX_REAR_PASSENGER);
        setpressureInputFrontDriver(IDX_FRONT_DRIVER);
        setpressureInputRearDriver(IDX_REAR_DRIVER);

        delay(500);
        ESP.restart(); // reboot this bih
    }
}

#pragma endregion

#pragma region training

uint8_t AIReadyBittset = 0;
uint8_t AIPercentage = 0;

void trainSingleAIModel(SOLENOID_AI_INDEX index)
{
    AIModelPreference *pref = getAIModel(index);

    // Fit into a temp model so a failed/rejected fit can't leave half-written weights in the live one.
    // up flag mirrors the live model (set in loadAILearnedDataPreferences).
    AIModel aiModelsTemp;
    aiModelsTemp.up = pref->model.up;

    Serial.print(F("Training AI "));
    Serial.println((int)index);
    unsigned long t = millis();

    // The model is linear in its weights, so exact least squares (normal equations) replaces the
    // old 10k-epoch gradient descent: true optimum, no learning rate, milliseconds instead of ~45s
    AIFitter fitter;
    PressureLearnSaveStruct *pls = getLearnData(index);
    int len = getLearnDataLength(index);
    for (int j = 0; j < len; j++)
    {
        fitter.add(aiModelsTemp, pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure, pls[j].timeMS);
    }

    if (!fitter.solveInto(aiModelsTemp))
    {
        Serial.print("AI fit failed (valid samples: ");
        Serial.print(fitter.sampleCount());
        Serial.println("), clearing this model's data to re-collect");
        clearPressureDataSingle(index);
        return;
    }

    // Quality gate: don't hand valve control to a model that can't even fit its own training data
    double sumSqMs = 0;
    int n = 0;
    for (int j = 0; j < len; j++)
    {
        if (!aiModelsTemp.isSampleValid(pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure))
        {
            continue;
        }
        double d = aiModelsTemp.predictDeNormalized(pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure) - (double)pls[j].timeMS;
        sumSqMs += d * d;
        n++;
    }
    double rmseMs = n > 0 ? sqrt(sumSqMs / n) : 0;
    if (n == 0 || rmseMs > ML_BATCH_RMSE_GATE_MS)
    {
        Serial.print("AI fit rejected, RMSE ms: ");
        Serial.println(rmseMs);
        Serial.println("Clearing this model's data to re-collect");
        clearPressureDataSingle(index);
        return;
    }

    unsigned long total = millis() - t;

    Serial.print("Ready ai model: ");
    Serial.println(index);
    Serial.print("Fit RMSE ms: ");
    Serial.println(rmseMs);
    Serial.print("Time for training: ");
    Serial.println(total);

    pref->model.loadWeights(aiModelsTemp.w1, aiModelsTemp.w2, aiModelsTemp.b);
    // Hand the batch fit's covariance + noise floor to the online learner so RLS updates start
    // appropriately small and the outlier gate is armed from sample one
    fitter.initOnline(pref->model);
    pref->model.onlineSeedGate((sumSqMs / n) / (ML_TIME_NORM_MS * ML_TIME_NORM_MS));
    pref->saveWeights();
    pref->setReady(true); // set to not train again and let it know it's ready to use
    AIReadyBittset = AIReadyBittset | (1 << index);
}

// Rebuild a ready model's RLS covariance and outlier-gate noise floor from the retained bootstrap
// samples (these are RAM-only and lost on reboot; the weights themselves persist in NVS)
static void initOnlineStateFromLearnData(SOLENOID_AI_INDEX index)
{
    AIModelPreference *pref = getAIModel(index);
    PressureLearnSaveStruct *pls = getLearnData(index);
    int len = getLearnDataLength(index);

    AIFitter fitter;
    for (int j = 0; j < len; j++)
    {
        fitter.add(pref->model, pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure, pls[j].timeMS);
    }
    if (!fitter.initOnline(pref->model))
    {
        // no usable bootstrap data (initOnline already fell back to the default covariance)
        return;
    }

    double sumSqNorm = 0;
    int n = 0;
    for (int j = 0; j < len; j++)
    {
        if (!pref->model.isSampleValid(pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure))
        {
            continue;
        }
        double d = (pref->model.predictDeNormalized(pls[j].start_pressure, pls[j].goal_pressure, pls[j].tank_pressure) - (double)pls[j].timeMS) / ML_TIME_NORM_MS;
        sumSqNorm += d * d;
        n++;
    }
    if (n > 0)
    {
        pref->model.onlineSeedGate(sumSqNorm / n);
    }
}

void updateAIPercentage()
{
    int totalLen = 0;
    for (int i = 0; i < 4; i++)
    {
        int len = getLearnDataLength((SOLENOID_AI_INDEX)i);
        totalLen += len;
    }
    AIPercentage = ((float)totalLen / ((float)LEARN_SAVE_COUNT * 4)) * 100;
}

// Measurement aid - drops the ready flags (keeping every stored sample) so the batch-fit path runs
// again and the headroom print in task_trainAI measures the deepest path rather than the shallow
// boot-time one. Leave this off; it re-fits and rewrites nvs on every boot.
// #define FORCE_AI_RETRAIN_TEST

void trainAIModels()
{
#ifdef FORCE_AI_RETRAIN_TEST
    // The fit is deterministic, so models that already passed the rmse gate pass it again and
    // clearPressureDataSingle() is never reached - the stored samples survive this.
    Serial.println(F("!! FORCE_AI_RETRAIN_TEST is on - retraining every model. Disable this define. !!"));
    for (int i = 0; i < 4; i++)
    {
        getAIModel((SOLENOID_AI_INDEX)i)->setReady(false);
    }
#endif

    // First load some default values based off info I grabbed from some corvette testing
    // I am using the first 4 weights because I think that makes the most logical sense to include all those. The 5th weight (ratio) I don't think is so great
    // upModel.loadWeights(-0.34525, 0.45432, -0.076937, 0.40201, 0.1, -0.19555);
    // downModel.loadWeights(0.76399, -0.66687, 0.070163, -0.5265, 0.1, 0.17787);
    // upModel.useWeight4 = true;
    // upModel.useWeight5 = false;
    // downModel.useWeight4 = true;
    // downModel.useWeight5 = false;

    for (int i = 0; i < 4; i++)
    {
        if (getAIModel((SOLENOID_AI_INDEX)i)->isReadyToUse.get().i == false)
        {
            if (getLearnDataLength((SOLENOID_AI_INDEX)i) >= LEARN_SAVE_COUNT)
            {
                trainSingleAIModel((SOLENOID_AI_INDEX)i);
            }
        }
        else
        {
            AIReadyBittset = AIReadyBittset | (1 << i);
            initOnlineStateFromLearnData((SOLENOID_AI_INDEX)i);
        }
    }

    Serial.print("AI training bittset: ");
    Serial.println(AIReadyBittset);
    updateAIPercentage();
}

void processLearnSampleQueues()
{
    static int anchorCursor[4] = {0, 0, 0, 0};
    static int samplesSinceAnchor[4] = {0, 0, 0, 0};

    for (int i = 0; i < 4; i++)
    {
        SOLENOID_AI_INDEX index = (SOLENOID_AI_INDEX)i;
        AIModelPreference *pref = getAIModel(index);

        if (!pref->isReadyToUse.get().i)
        {
            continue;
        }

        bool trained = false;
        PressureLearnSaveStruct sample;
        while (dequeueLearnSample(index, &sample))
        {
            if (pref->model.trainOnline(sample.start_pressure, sample.goal_pressure, sample.tank_pressure, sample.timeMS))
            {
                trained = true;
            }

            // Anchor replay: day-to-day driving produces mostly small trim moves, and the RLS
            // forgetting window would slowly wash out what the bootstrap taught about big lifts.
            // Re-feeding an occasional retained bootstrap sample keeps that knowledge anchored.
            samplesSinceAnchor[i]++;
            int len = getLearnDataLength(index);
            if (samplesSinceAnchor[i] >= ML_ONLINE_ANCHOR_INTERVAL && len > 0)
            {
                samplesSinceAnchor[i] = 0;
                PressureLearnSaveStruct *anchor = &getLearnData(index)[anchorCursor[i] % len];
                anchorCursor[i] = (anchorCursor[i] + 1) % len;
                if (pref->model.trainOnline(anchor->start_pressure, anchor->goal_pressure, anchor->tank_pressure, anchor->timeMS))
                {
                    trained = true;
                }
            }
        }

        if (trained)
        {
            // The loop above drains the queue, so this saves once per burst rather than per sample
            pref->saveWeights();
        }
    }
}

double getAiPredictionTime(SOLENOID_AI_INDEX aiIndex, double start_pressure, double end_pressure, double tank_pressure)
{
    return getAIModel(aiIndex)->model.predictDeNormalized(start_pressure, end_pressure, tank_pressure);
}

bool canUseAiPrediction(SOLENOID_AI_INDEX aiIndex)
{
    if (!getaiEnabled())
    {
        return false;
    }
    return getAIModel(aiIndex)->isReadyToUse.get().i;
}

#pragma endregion

#pragma region LED_functions

CRGB leds[LED_NUM];
void setupLEDs()
{
    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, LED_NUM);
    FastLED.setBrightness(50);  // Set brightness (0-255)
    
    // Set all LEDs to red
    fill_solid(leds, LED_NUM, CRGB::DarkCyan);
    FastLED.show();
}

#pragma endregion