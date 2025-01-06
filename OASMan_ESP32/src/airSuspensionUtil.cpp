#include "airSuspensionUtil.h"
#include "saveData.h"

#pragma region variables

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
    getWheel(WHEEL_FRONT_PASSENGER)->initPressureGoal(getWheel(WHEEL_FRONT_PASSENGER)->getPressure() + value, true);
    getWheel(WHEEL_REAR_PASSENGER)->initPressureGoal(getWheel(WHEEL_REAR_PASSENGER)->getPressure() + value, true);
    getWheel(WHEEL_FRONT_DRIVER)->initPressureGoal(getWheel(WHEEL_FRONT_DRIVER)->getPressure() + value, true);
    getWheel(WHEEL_REAR_DRIVER)->initPressureGoal(getWheel(WHEEL_REAR_DRIVER)->getPressure() + value, true);
}

#pragma endregion

#pragma region tank_comp_functions

void compressorLogic()
{
    // TODO: check, This resume and pause logic may be able to be removed!!! It says it is used for thread blocking tasks which is no longer an issue
    // I guess we may still want to keep it  for the case that if a valve is open the tanks pressure is not accurate??? Could probably be removed tbh
    // if (isAnyWheelActive())
    // {
    //     getCompressor()->pause();
    // }
    // else
    // {
    //     getCompressor()->resume();
    // }

    getCompressor()->loop();
}

#pragma endregion

#pragma region calibration

// this function is used to grab the realistic values because 5v is not perfect
// Make sure to add warning for ensure all other functions and not running ect ect
// This is also very specific to the main designed oasman board, assuming tank is on voltage divider and other values are on adc
void calibratePressureValues()
{
    getCompressor()->pause();

    // step 1: dump all valves
    for (int i = 0; i < SOLENOID_COUNT; i++)
    {
        manifold->get(i)->digitalWrite(HIGH);
    }

    delay(20 * 1000); // sleep litteral 20 seconds so tanks dump all the air

    // close all valves
    for (int i = 0; i < SOLENOID_COUNT; i++)
    {
        manifold->get(i)->digitalWrite(LOW);
    }

    // step 2: read valves at 0psi

    const int sampleSize = 25;
    double totalVoltageDivider = 0;
    double totalADC = 0;

    for (int i = 0; i < sampleSize; i++)
    {
        int v_vd = getCompressor()->getReadPin()->analogRead(true);
        int v_adc = getWheel(WHEEL_FRONT_PASSENGER)->getPressurePin()->analogRead(true);
        // Serial.print("v: ");
        // Serial.print(v_vd);
        // Serial.print("\t");
        // Serial.println(v_adc);
        totalVoltageDivider += v_vd; // read tank pressure analog value at 0psi (voltage divider)
        totalADC += v_adc;           // read first bag pressure at 0psi (adc)
        delay(250);                  // should take a total of 6.25 seconds for this loop to complete
    }

    // step 3: save these values for use in input_type

    Calibration *calibration = getCalibration();
    calibration->voltageDividerCalibration.setFloat(totalVoltageDivider / sampleSize);
    calibration->adcCalibration.setFloat(totalADC / sampleSize);
    calibration->hasCalibrated.set(true);

    getCompressor()->resume();
}

#pragma endregion

#pragma region accessory_wire

template <typename T>
void sampleReading(T &result, T reading, T *arr, int &counter, const int arrSize)
{
    arr[counter] = reading;
    counter++;
    if (counter >= arrSize)
    {
        double total = 0; // choosing double because max we read is float and integers we can also fit into doubles then convert back to integers
        for (int i = 0; i < arrSize; i++)
        {
            total += (double)arr[i];
        }

        total = total / arrSize;
        result = (T)total;
        counter = 0;
    }
}

bool vehicleOn = false;
bool isVehicleOn()
{
    return vehicleOn;
}

#if ENABLE_ACCESSORY_WIRE_FUNCTIONALITY == true

InputType *accessoryWire;
InputType *outputKeepESPAlive;
unsigned long lastTimeLive = 0;
void accessoryWireSetup()
{
    accessoryWire = accessoryInput;
    outputKeepESPAlive = outputKeepAlivePin;
    outputKeepESPAlive->digitalWrite(HIGH);
    lastTimeLive = millis();
}
const int accessoryWireSampleSize = 5;
bool vehicleOnHistory[accessoryWireSampleSize];
int vehicleOnCounter = 0;
void accessoryWireLoop()
{
    sampleReading(vehicleOn, accessoryWire->digitalRead() == LOW, vehicleOnHistory, vehicleOnCounter, accessoryWireSampleSize);
    if (isVehicleOn())
    {
        // accessory wire is supplying 12v (car on)
        notifyKeepAlive();
    }
    if (millis() > (lastTimeLive + SYSTEM_SHUTOFF_TIME_MS))
    {
        outputKeepESPAlive->digitalWrite(LOW); // acc wire has been off for some time, shut down system
    }
    else
    {
        outputKeepESPAlive->digitalWrite(HIGH);
    }
}

// in the future we can call this from bluetooth functions to keep the device alive longer if actively using bluetooth while the vehicle is off
void notifyKeepAlive()
{
    lastTimeLive = millis();
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
void notifyKeepAlive() {}
#endif

#pragma endregion