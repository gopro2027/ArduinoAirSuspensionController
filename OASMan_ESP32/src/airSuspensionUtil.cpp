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
bool ADS1115A_exists;
bool ADS1115B_exists;
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
    // A or B missing no longer reboots; their inputs just read 0 instead (see InputType::analogRead)
    if (!ADS1115A.begin(ADS_A_ADDRESS))
    {
        ADS1115A_exists = false;
        Serial.println(F("Failed to initialize ADS A (pressure sensors)"));
    } else {
        ADS1115A_exists = true;
    }

    if (!ADS1115B.begin(ADS_B_ADDRESS))
    {
        ADS1115B_exists = false;
        Serial.println(F("Failed to initialize ADS B (height sensors)"));
    } else {
        ADS1115B_exists = true;
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

    // Fast conversions (~1.16 ms vs the ~7.8 ms default 128 SPS) so the closed-loop pressure controller
    // can read bag+tank every tick even with all 4 corners moving. Slightly noisier raw values, which the
    // offset fit + control deadband absorb. ; was: RATE_ADS1115_128SPS (library default)
    ADS1115A.setDataRate(RATE_ADS1115_860SPS);
    ADS1115B.setDataRate(RATE_ADS1115_860SPS);
    ADS1115C.setDataRate(RATE_ADS1115_860SPS);
    ADS1115D.setDataRate(RATE_ADS1115_860SPS);
}

void setupManifold()
{
    initializeADS();
    motionDetectInit(); // optional IMU, ignored if not found

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
unsigned long ebrakeOnSince = 0;

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
    if (ebrakeOn && !previousReading)
        ebrakeOnSince = millis();

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

bool isEBrakeOnFor(unsigned long ms)
{
    return isEBrakeOn() && millis() - ebrakeOnSince >= ms;
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
bool isEBrakeOnFor(unsigned long ms)
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

#define GPS_STOPPED_DWELL_MS 5000   // gps must read <= 1mph this long to count as stopped
#define ACC_OFF_PARKED_DWELL_MS 3000 // must stay under AIR_OUT_AFTER_SHUTDOWN_MS or fall on shutdown never sees the car as off
#define EBRAKE_PARKED_DWELL_MS 5000

enum GpsMotionState
{
    GPS_MOTION_UNKNOWN, // no gps, bad fix, 1-3mph, or not stopped long enough yet
    GPS_MOTION_MOVING,
    GPS_MOTION_STOPPED
};

// good gps fix: > 3mph is moving, <= 1mph for GPS_STOPPED_DWELL_MS is stopped
static GpsMotionState getGPSMotionState() {
    static unsigned long gpsStoppedSince = 0;
    if (!gps_exists || !isGPSCurrentlyAccurate()) {
        gpsStoppedSince = 0;
        return GPS_MOTION_UNKNOWN;
    }
    int speed = getVehicleSpeed();
    if (speed > 3) {
        gpsStoppedSince = 0;
        return GPS_MOTION_MOVING;
    }
    if (speed > 1) {
        gpsStoppedSince = 0;
        return GPS_MOTION_UNKNOWN; // if it's bewteen 1 and 3 just say unknown so it can get external secondary checks try to confirm instead. Essentially the thinking is GPS_MOTION_MOVING is the strictest 'we know we are driving' and GPS_MOTION_UNKNOWN is the general 'we just can't tell from the gps, do further checks to confirm'
    }
    if (gpsStoppedSince == 0)
        gpsStoppedSince = millis();
    return millis() - gpsStoppedSince >= GPS_STOPPED_DWELL_MS ? GPS_MOTION_STOPPED : GPS_MOTION_UNKNOWN;
}

// Not moving (or barely moving). Checks the most accurate source available: GPS -> IMU -> acc/ebrake. This should NOT be used when we need 100% certainty that the vehicle is parked.
bool isVehicleStationary() {

    // When the GPS is working, we trust the accuracy of this the most in regards to whether the vehicle is at a safe state to execute slightly unsafe functions.
    GpsMotionState gps = getGPSMotionState();
    if (gps != GPS_MOTION_UNKNOWN) {
        // gps is either moving or stopped with certainty, so return the result of the gps check
        return gps == GPS_MOTION_STOPPED;
    }

    // When the GPS is uncertain, we here trust the IMU to give us a more accurate result.
    if (motionImuPresent()) {
        return motionImuSaysStationary(); // Simply return the result of the imu
    }

    // Lastly we can check how stable the actual bags are right now, basically checking if the suspension is moving. If we are parked or driving very slowly this will not be moving.
    if (!areAllWheelsStable()) {
        return false;
    }

    // On a board like the v5 this will actually never be called because the IMU is always present, but essentially all v4 and older are expected to hit this, which will require either the car is off for 3 seconds or the ebrake is pulled for 5 seconds.
    return isVehicleDefinitelyParked(false);
}

// 100% certainty checks only: car off for a few seconds and/or ebrake (if connected) pulled for a few seconds. v2 is the only board that will essentially never return true for isVehicleDefinitelyParked or getGPSMotionState, and that is expected.
bool isVehicleDefinitelyParked(bool requireBothCarOffAndEbrake) {
    bool carOff = isVehicleOffFor(ACC_OFF_PARKED_DWELL_MS);
    bool ebrakePulled = getebrakeConnected() && isEBrakeOnFor(EBRAKE_PARKED_DWELL_MS);
    if (requireBothCarOffAndEbrake)
        return carOff && ebrakePulled;
    return carOff || ebrakePulled;

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

bool areAllWheelsStable()
{
    for (int i = 0; i < 4; i++)
    {
        if (!getWheel(i)->isPressureStable())
        {
            return false;
        }
    }
    return true;
}

// function renamed to 'bag stretch'
static void initPressureGoalWithStretch(byte wheelNum, int goal)
{
    Wheel *w = getWheel(wheelNum);
    int belowP = getAirUpBagStretchTriggerBelowPressure();
    int unroll = getAirUpBagStretchPressure();
    // the functionality of this feature was changed a bit from the original idea. I've kind of deemed the original idea of 'going past the pressure' kind of useless on it's own, as it's just going to make almost every time you go to a preset be inaccurate.
    // instead what this function will do is check if the current pressure is below the AirUpBagStretchTriggerBelowPressure, which by default is 40psi, and if it is, it will go to the AirUpBagStretchPressure pressure + goal first in order to stretch the bag out.
    if (!getheightSensorMode() && // don't do this feature if height sensor mode is enabled
        unroll > 0 && // unroll being 0 is functionally disabled
        //goal < unroll && // only run if the goal pressure is below stretch pressure, otherwise no need to stretch if it's already going past it // UPDATE: unroll is now just an addition to goal, so this check is unnecessary
        goal > w->getSelectedInputValue() && // only run if we are airing up
        w->getSelectedInputValue() < belowP) // only run if the current pressure is below the AirUpBagStretchTriggerBelowPressure
    {
        bool reachable = w->initPressureGoal(goal + unroll, true, [w, goal]()
                            { w->initPressureGoal(goal); }); //this could technically be initPressureGoalWithStretch (recursive) but with a chance of an infinite loop, so we will go straight to initPressureGoal for now unless we want to add more functionality and failsafe checks to initPressureGoalWithStretch in the future
        if (reachable == false)
        {
            // The goal + unroll is probably higher than the max pressure or something, so just go to the goal directly as if unroll didn't exist.
            w->initPressureGoal(goal);
        }
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

#define AIR_OUT_ON_SHUTOFF_GIVE_UP_MS 30000 // stop retrying this long after key-off so it can't air out unexpectedly later

// returns true once there is nothing left to do (aired out, feature off, or gave up), false if the safety checks aren't met yet so the caller retries
bool tryAirOutOnShutoff()
{
#if ENABLE_AIR_OUT_ON_SHUTOFF
#if !AIR_OUT_ON_SHUTOFF_DOUBLE_LOCK_MODE
    if (!getairOutOnShutoff() || isVehicleOffFor(AIR_OUT_ON_SHUTOFF_GIVE_UP_MS))
    {
        return true;
    }
    // The first call to isVehicleStationary is mostly redundant and mostly only acts as an additional safety check on the boards with more safety support like the v5.
    // This mostly is reliant on the second isVehicleDefinitelyParked(true) which verifies that both the car is off and ebrake is pulled for a few seconds.
    // So this feature (shutdown on air out) is only really intended to work on v4+ boards, since v3 doesn't have ebrake input and v2 doesn't have acc wire input.
    if (isVehicleStationary() && isVehicleDefinitelyParked(true))
    {
        loadProfileAirUp(0); // packet 0 should be the lowest setting!
        return true;
    }
    return false;
#endif
#endif
    return true;
}

#pragma endregion

#pragma region accessory_wire

bool vehicleOn = true; // want this to be true to begin so it sets the current time first thing so it knows it was on and doesn't run the off routine first.
unsigned long vehicleOffSince = 0;
unsigned long lastTimeLive = 0;
bool isVehicleOn()
{
    return vehicleOn;
}

bool isVehicleOffFor(unsigned long ms)
{
    return !vehicleOn && millis() - vehicleOffSince >= ms;
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
bool airOutOnShutoffPending = false; // retried every loop while the car stays off, until the safety checks pass
void accessoryWireLoop()
{
    bool previousVehicleOn = vehicleOn;
#if BOARD_ALWAYS_ON_ACC_UNUSED_USE_BT_CONN_AS_VEHICLE_ON
    sampleReading(vehicleOn, getBLEConnectedClientCount() > 0, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
#else
    sampleReading(vehicleOn, accessoryWire->digitalRead() == HIGH, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
#endif
    if (isVehicleOn())
    {
        if (previousVehicleOn == false) {
            getAuxillaryOutput()->setDoStartupEvent(true);
        }
        // accessory wire is supplying 12v (car on)
        notifyKeepAlive();
        hasJustShutoff = false;
        airOutOnShutoffPending = false;
    }
    else
    {
        if (previousVehicleOn)
            vehicleOffSince = millis();
        // vehicle is off... first check if 5 seconds have passed
        if (isKeepAliveTimeReadyForShutdownRoutine())
        {
            // one time check that it just happened for the first time
            if (hasJustShutoff == false)
            {
                hasJustShutoff = true;
                airOutOnShutoffPending = true;
                getAuxillaryOutput()->setDoShutdownEvent(true);
            }
        }
        // actually check if air out code is enabled and do as asked, keeps retrying since the car may not be stationary/parked yet (ex: bags still settling)
        if (airOutOnShutoffPending && tryAirOutOnShutoff())
        {
            airOutOnShutoffPending = false;
        }
    }
#if BOARD_ALWAYS_ON_ACC_UNUSED_USE_BT_CONN_AS_VEHICLE_ON
    // vehicle on/off is driven by bluetooth connectivity, so keep the accessory output latched high
    outputKeepESPAlive->digitalWrite(HIGH);
#else
    // acc off with a low battery for a while, shut down to protect the battery (overrides bluetooth keep alive)
    static unsigned long lowBatterySince = 0;
    uint16_t batteryMv = getBatteryMilliVolts();
    if (!isVehicleOn() && batteryMv > 0 && batteryMv < BATTERY_SHUTDOWN_MV)
    {
        if (lowBatterySince == 0)
        {
            lowBatterySince = millis();
        }
        else if (millis() - lowBatterySince > BATTERY_SHUTDOWN_DELAY_MS)
        {
            Serial.printf("Low battery shutdown (%u mV)\n", batteryMv);
            forceShutoff = true;
        }
    }
    else
    {
        lowBatterySince = 0;
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
#endif
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

#pragma region battery_voltage

#ifdef BATTERY_VOLTAGE_PIN
const int batteryVoltageSampleSize = 10; // called every 100ms, so a 1 second average
float batteryVoltageHistory[batteryVoltageSampleSize];
int batteryVoltageCounter = 0;
float batteryMilliVolts = 0; // stays 0 until the first full average
#endif

void batteryVoltageLoop()
{
#ifdef BATTERY_VOLTAGE_PIN
    sampleReading(batteryMilliVolts, analogReadMilliVolts(BATTERY_VOLTAGE_PIN) / BATTERY_VOLTAGE_DIVIDER_RATIO, batteryVoltageHistory, batteryVoltageCounter, batteryVoltageSampleSize);
#endif
}

// Smoothed vehicle battery mV, or 0 when the board has no sense pin or the reading is broken
uint16_t getBatteryMilliVolts()
{
#ifdef BATTERY_VOLTAGE_PIN
    if (batteryMilliVolts < BATTERY_MIN_VALID_MV)
    {
        return 0;
    }
    return (uint16_t)batteryMilliVolts;
#else
    return 0;
#endif
}

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