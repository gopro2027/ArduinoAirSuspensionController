#include "airSuspensionUtil.h"
#include "saveData.h"

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
// InputType *manifoldSafetyWire;

Wheel *getWheel(int i)
{
    return wheel[i];
}

void setRideHeightFrontPassenger(byte value)
{
    currentProfile[WHEEL_FRONT_PASSENGER] = value;
    if (getRaiseOnPressureSet())
    {
        getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(value);
    }
}
void setRideHeightRearPassenger(byte value)
{
    currentProfile[WHEEL_REAR_PASSENGER] = value;
    if (getRaiseOnPressureSet())
    {
        getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(value);
    }
}
void setRideHeightFrontDriver(byte value)
{
    currentProfile[WHEEL_FRONT_DRIVER] = value;
    if (getRaiseOnPressureSet())
    {
        getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(value);
    }
}
void setRideHeightRearDriver(byte value)
{
    currentProfile[WHEEL_REAR_DRIVER] = value;
    if (getRaiseOnPressureSet())
    {
        getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(value);
    }
}

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
byte goToPerciseBitset = 0;
void setGoToPressureGoalPercise(byte wheelnum)
{
    goToPerciseBitset = goToPerciseBitset | (1 << wheelnum);
}
void setNotGoToPressureGoalPercise(byte wheelnum)
{
    goToPerciseBitset = goToPerciseBitset & ~(1 << wheelnum);
}
bool shouldDoPressureGoalOnWheel(byte wheelnum)
{
    return (goToPerciseBitset >> wheelnum) & 1;
}
bool skipPerciseSet = false; // this is like a global flag to tell it to not do percise pressure set only from the main pressure goal routine
void pressureGoalRoutine()
{
    bool a = false;
    if (isAnyWheelActive())
    {
        readPressures();
        a = true;
    }
    for (int i = 0; i < 4; i++)
    {
        getWheel(i)->pressureGoalRoutine();
    }
    if (a == false)
    {
        if (goToPerciseBitset != 0)
        {
            // Uncomment this to make it run twice for more precision
            for (byte i = 0; i < 4; i++)
            {
                if (shouldDoPressureGoalOnWheel(i))
                {
                    if (skipPerciseSet == false)
                        getWheel(i)->percisionGoToPressure();
                }
            }
            // run a second time :P and also set it to not run again
            for (byte i = 0; i < 4; i++)
            {
                if (shouldDoPressureGoalOnWheel(i))
                {
                    if (skipPerciseSet == false)
                        getWheel(i)->percisionGoToPressure();
                    setNotGoToPressureGoalPercise(i);
                }
            }
        }
    }
}

void airUp()
{
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(currentProfile[WHEEL_FRONT_PASSENGER]);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(currentProfile[WHEEL_REAR_PASSENGER]);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(currentProfile[WHEEL_FRONT_DRIVER]);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(currentProfile[WHEEL_REAR_DRIVER]);
}

void airOut()
{
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(30);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(30);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(30);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(30);
}

void airUpRelativeToAverage(int value)
{
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(getWheel(WHEEL_FRONT_PASSENGER)->getPressure() + value);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(getWheel(WHEEL_REAR_PASSENGER)->getPressure() + value);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(getWheel(WHEEL_FRONT_DRIVER)->getPressure() + value);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(getWheel(WHEEL_REAR_DRIVER)->getPressure() + value);
}

float pressureValueTank = 0;
int getTankPressure()
{
#if TANK_PRESSURE_MOCK == true
    return 200;
#else
    return pressureValueTank;
#endif
}

float readPinPressure(InputType *pin);
const int time_solenoid_movement_delta = 500; // ms
const int time_solenoid_open_time = 1;        // ms
void readPressures()
{
    pressureValueTank = compressor->readPressure();

    // check if any air up solenoids are open and if so, close them for reading
    bool safePressureReadAny = false;
    for (int i = 0; i < 4; i++)
    {
        if (getWheel(i)->prepareSafePressureRead())
        {
            safePressureReadAny = true;
        }
    }

    // wait a bit of time for the solenoids to physically close
    if (safePressureReadAny)
    {
        for (int i = 0; i < 4; i++)
        {
            getWheel(i)->safePressureReadPauseClose();
        }
        delay(time_solenoid_movement_delta);
    }

    // read the pressures
    for (int i = 0; i < 4; i++)
    {
        getWheel(i)->readPressure();
    }

    // re-open solenoids if necessary
    for (int i = 0; i < 4; i++)
    {
        getWheel(i)->safePressureClose();
    }

    // give them a brief pause to stay open (not super necessary)
    if (safePressureReadAny)
    {
        delay(time_solenoid_open_time);
        // resume wheels after delay
        for (int i = 0; i < 4; i++)
        {
            getWheel(i)->safePressureReadResumeClose();
        }
    }
}

void compressorLogic()
{
    if (isAnyWheelActive())
    {
        compressor->pause();
    }
    else
    {
        compressor->resume();
    }
    compressor->loop();
}
