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
    sampleReading(vehicleOn, accessoryWire->digitalRead() == HIGH, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
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

#pragma region parabolaLearn
#ifdef parabolaLearn
// #include "SPIFFS.h"
// void setupSpiffs()
// {
//     if (!SPIFFS.begin(true))
//     {
//         Serial.println("An Error has occurred while mounting SPIFFS");
//         return;
//     }

//     File file = SPIFFS.open("/log.txt", FILE_READ);
//     if (!file)
//     {
//         Serial.println("There was an error opening the file for writing");
//         return;
//     }
//     Serial.println("Log.txt data:");
//     Serial.println(file.readString());
//     Serial.println("LOG EOL");

//     file.close();
// }
void learnParabolaSetup()
{

    getCompressor()->enableDisableOverride(true); // make sure compressor is on
    // STEP 1: Air out all bags
    downAllBags(20 * 1000);

    // getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(20);
    // getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(20);
    // getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(20);
    // getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(20);
    // while (isAnyWheelActive())
    // {
    //     delay(20);
    // }

    delay(500); // wait for pressures to stabilize
}

void doParabolaRoutineOneWay(bool up)
{
    // file.print(up ? "UP: " : "DOWN: ");
    const int sz = 10;
    double pressures[sz];
    double times[sz];

    times[0] = 0;
    pressures[0] = averageBags();
    char buf[30];
    const int valve_timing = 200;
    for (int i = 1; i < sz; i++)
    {
        if (up)
        {
            upAllBags(valve_timing);
        }
        else
        {
            downAllBags(valve_timing);
        }

        delay(500); // wait for pressures to stabilize
        times[i] = i * valve_timing;
        pressures[i] = averageBags();

        snprintf(buf, sizeof(buf), "(%d,%d),", (int)(pressures[i]), (int)(times[i]));
        // file.print(buf);
    }
    // file.print("\n");

    double A, B, C;
    A = B = C = 0;
    // calculateAverageOfSamples(pressures, times, sz, A, B, C);
    // updateParabola(up, A, B, C);
}

// returns if it completed or not
bool learnParabolaLoop()
{
    if (getCompressor()->isOn())
    {
        return false; // not done filling up yet!
    }

    Serial.println("Running parabola routine!");

    // File file = SPIFFS.open("/log.txt", FILE_APPEND);

    // do up parabola
    delay(500);
    doParabolaRoutineOneWay(true);
    delay(500);
    doParabolaRoutineOneWay(false);
    // file.print("\n");
    // file.close();
    return true;
}
#endif
#pragma endregion