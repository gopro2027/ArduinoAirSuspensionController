#include "wheel.h"

struct PressureGoalValveTiming
{
    int pressureDelta;          // the pressure percision that we are trying to achieve
    int valveTimingUntilWithin; // the amount of time we will leave the valve open until we get within the range of goalPressure +- pressureDelta
    bool isPerciseMeasurement;  // when using air up quick type functions, will not take into account ones that are set to true here
};

// Must put in sorted order of largest to smallest
PressureGoalValveTiming valveTiming[] = {
    {100, 500, false},
    {50, 250, false},
    {25, 125, false},
    {10, 50, false}, // if current psi outside range of goalPressure +- 10psi, open valves for 75ms until +- 10psi achieved
    {5, 20, false},  // if current psi outside range of goalPressure +- 5psi, open valves for 20ms until +- 6psi achieved
    {0, 5, true},    // if current psi outside range of goalPressure +- 0psi (aka psi is not yet exactly goalPressure), open valves for 5ms until exact goal pressure is achieved. Will not be used in quick mode
};
#define VALVE_TIMING_LIST_COUNT (sizeof(valveTiming) / sizeof(PressureGoalValveTiming))

// This function can be updated in the future to use some better algorithm to decide how long to open the valves for to reach the desigred pressure
PressureGoalValveTiming *getValveTiming(int pressureDifferenceAbsolute, bool quickMode)
{
    PressureGoalValveTiming *lastTime = &valveTiming[0];
    for (int i = 0; i < VALVE_TIMING_LIST_COUNT; i++)
    {
        if (quickMode && valveTiming[i].isPerciseMeasurement)
        {
            return lastTime;
        }
        if (pressureDifferenceAbsolute > valveTiming[i].pressureDelta)
        {
            return &valveTiming[i];
        }
        lastTime = &valveTiming[i];
    }
    return lastTime; // should never get to this case but if it does it returns the smallest time
}

int getMinValveOpenPSI(bool quickMode)
{
    return getheightSensorMode() ? 0 : getValveTiming(0, quickMode)->pressureDelta;
}

// TODO: Turn this into a function based on the values above so that it is smooth, and add a multiplier to change in the settings for people with larger volume bags where it's too slow by default
int calculateValveOpenTimeMS(int pressureDifferenceAbsolute, bool quickMode)
{
    return getheightSensorMode() ? 0 : (getValveTiming(pressureDifferenceAbsolute, quickMode)->valveTimingUntilWithin * ((float)getbagVolumePercentage() / 100.0f)); // Note: Added 0 for height sensor mode but it is unused
}

Wheel::Wheel() {}

Wheel::Wheel(Solenoid *solenoidInPin, Solenoid *solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum)
{
    this->pressurePin = pressurePin;
    this->levelSensorPin = levelSensorPin;
    this->thisWheelNum = thisWheelNum;
    this->s_AirIn = solenoidInPin;
    this->s_AirOut = solenoidOutPin;
    this->pressureValue = 0;
    this->pressureGoal = 0;
    this->routineStartTime = 0;
    this->flagStartPressureGoalRoutine = false;
    this->quickMode = false;
}

Solenoid *Wheel::getInSolenoid()
{
    return this->s_AirIn;
}

Solenoid *Wheel::getOutSolenoid()
{
    return this->s_AirOut;
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
    return this->s_AirIn->isOpen() || this->s_AirOut->isOpen();
}

void Wheel::initPressureGoal(int newPressure, bool quick)
{
    if (newPressure > (getheightSensorMode() ? getHeightSensorMax() * 1.03f : getbagMaxPressure()))
    {
        // hardcode not to go above set psi
        return;
    }
    int pressureDif = newPressure - this->getSelectedInputValue(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI(quick))
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
            this->quickMode = quick;
            this->routineStartTime = millis();
            this->flagStartPressureGoalRoutine = true;
        }
    }
}

// height sensor: AA-ROT-120 https://www.aliexpress.us/item/3256807527882480.html https://www.amazon.com/Height-Sensor-Suspension-Leveling-AA-ROT-120/dp/B08DJ3HX1B https://www.aliexpress.us/item/3256806751644782.html
// Output voltage UA:0.5-4.5

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::loop()
{
    // Serial.println("WheelP: ");
    this->readInputs();
    if (this->flagStartPressureGoalRoutine)
    {
        this->flagStartPressureGoalRoutine = false;
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

            if (pressureDifABS > getMinValveOpenPSI(this->quickMode))
            {
                // Decide which valve to use
                Solenoid *valve;
                if (this->pressureGoal > this->getSelectedInputValue())
                {
                    valve = this->s_AirIn;
                    this->s_AirOut->close();
                }
                else
                {
                    valve = this->s_AirOut;
                    this->s_AirIn->close();
                }

                if (!getheightSensorMode())
                {
                    // Pressure sensor logic. You can't read the pressure accurately with the valve open. Essentially must open the valve for a guesstimate amount of time, then close it, then you are able to read the pressure.

                    // Choose time to open for
                    int valveTime = calculateValveOpenTimeMS(pressureDifABS, this->quickMode);

                    // Open valve for calculated time
                    valve->open();
                    delay(valveTime);
                    valve->close();

                    // Sleep 150ms to allow time for valve to fully close and pressure to equalize a bit
                    delay(150);
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
        }
        // close both after (only applies for level sensor logic)
        this->s_AirIn->close();
        this->s_AirOut->close();
    }
}