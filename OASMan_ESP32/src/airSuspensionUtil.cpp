#include "airSuspensionUtil.h"
#include "manifoldSaveData.h"

#pragma region variables

InputType *pressureInputs[5];
Manifold *manifold;
Compressor *compressor;
Wheel *wheel[4];

#if USE_ADS == true
Adafruit_ADS1115 ADS1115A;
#if USE_2_ADS == true
Adafruit_ADS1115 ADS1115B;
#endif
#endif

Manifold *getManifold()
{
    return manifold;
}
Compressor *getCompressor()
{
    return compressor;
}
// InputType *manifoldSafetyWire;

Wheel *getWheel(int i)
{
    return wheel[i];
}

#pragma endregion

#pragma region setting_current_profile
void setRideHeightFrontPassenger(byte value)
{
    currentProfile[WHEEL_FRONT_PASSENGER] = value;
    if (getraiseOnPressure())
    {
        getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(value);
    }
}
void setRideHeightRearPassenger(byte value)
{
    currentProfile[WHEEL_REAR_PASSENGER] = value;
    if (getraiseOnPressure())
    {
        getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(value);
    }
}
void setRideHeightFrontDriver(byte value)
{
    currentProfile[WHEEL_FRONT_DRIVER] = value;
    if (getraiseOnPressure())
    {
        getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(value);
    }
}
void setRideHeightRearDriver(byte value)
{
    currentProfile[WHEEL_REAR_DRIVER] = value;
    if (getraiseOnPressure())
    {
        getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(value);
    }
}
#pragma endregion

#pragma region initialization

#if USE_ADS == true
void initializeADS()
{
    if (!ADS1115A.begin(ADS_A_ADDRESS))
    {
        Serial.println(F("Failed to initialize ADS A"));
#if ADS_MOCK_BYPASS == false
        while (1)
            ;
#endif
    }
#if USE_2_ADS == true
    if (!ADS1115B.begin(ADS_B_ADDRESS))
    {
        Serial.println(F("Failed to initialize ADS B"));
#if ADS_MOCK_BYPASS == false
        while (1)
            ;
#endif
    }
#endif
}
#endif

void setupManifold()
{
#if USE_ADS == true
    initializeADS();
#endif

    manifold = new Manifold(
        solenoidFrontPassengerInPin,
        solenoidFrontPassengerOutPin,
        solenoidRearPassengerInPin,
        solenoidRearPassengerOutPin,
        solenoidFrontDriverInPin,
        solenoidFrontDriverOutPin,
        solenoidRearDriverInPin,
        solenoidRearDriverOutPin);
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

void airUp(bool quick)
{
    // TODO: if quick, skip high percision
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(currentProfile[WHEEL_FRONT_PASSENGER], quick);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(currentProfile[WHEEL_REAR_PASSENGER], quick);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(currentProfile[WHEEL_FRONT_DRIVER], quick);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(currentProfile[WHEEL_REAR_DRIVER], quick);
}

void airOut()
{
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(AIR_OUT_PRESSURE_PSI);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(AIR_OUT_PRESSURE_PSI);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(AIR_OUT_PRESSURE_PSI);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(AIR_OUT_PRESSURE_PSI);
}

void airUpRelativeToAverage(int value)
{
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue() + value, true);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue() + value, true);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue() + value, true);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue() + value, true);
}

bool isCarMoving()
{
    // TODO: add gps code to check if we are driving
    return true;
}

void airOutWithSafetyCheck()
{
    // only air out if car is not moving!
    if (!isCarMoving())
    {
        if (getairOutOnShutoff())
        {
            readProfile(0); // packet 0 should be the lowest setting!
            airUp(false);
        }
    }
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
    sampleReading(vehicleOn, accessoryWire->digitalRead() == LOW, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
    if (isVehicleOn())
    {
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

#pragma region pressure sensor learn

void learnPressureSensorsRoutine()
{
    // manifold->get(SOLENOID_INDEX::FRONT_PASSENGER_OUT).
}

#pragma endregion