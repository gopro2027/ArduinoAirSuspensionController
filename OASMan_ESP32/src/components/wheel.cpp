#include "wheel.h"
#include <Wire.h>
#include <SPI.h>

struct PressureGoalValveTiming
{
    int pressureDelta;          // the pressure percision that we are trying to achieve
    int valveTimingUntilWithin; // the amount of time we will leave the valve open until we get within the range of goalPressure +- pressureDelta
    bool isPerciseMeasurement;  // when using air up quick type functions, will not take into account ones that are set to true here
};

// Must put in sorted order of largest to smallest
PressureGoalValveTiming valveTiming[] = {
    {10, 500, false}, // if current psi outside range of goalPressure +- 10psi, open valves for 500ms until +- 10psi achieved
    {5, 100, false},  // if current psi outside range of goalPressure +- 6psi, open valves for 100ms until +- 6psi achieved
    {0, 10, true},    // if current psi outside range of goalPressure +- 0psi (aka psi is not yet exactly goalPressure), open valves for 10ms until exact goal pressure is achieved. Will not be used in quick mode
};
#define VALVE_TIMING_LIST_COUNT (sizeof(valveTiming) / sizeof(PressureGoalValveTiming))

// This function can be updated in the future to use some better algorithm to decide how long to open the valves for to reach the desigred pressure
int calculateValveOpenTimeMS(int pressureDifferenceAbsolute, bool quickMode)
{
    int lastTime = valveTiming[0].valveTimingUntilWithin;
    for (int i = 0; i < VALVE_TIMING_LIST_COUNT; i++)
    {
        if (quickMode && valveTiming[i].isPerciseMeasurement)
        {
            return lastTime;
        }
        if (pressureDifferenceAbsolute > valveTiming[i].pressureDelta)
        {
            return valveTiming[i].valveTimingUntilWithin;
        }
        lastTime = valveTiming[i].valveTimingUntilWithin;
    }
    return lastTime; // should never get to this case but if it does it returns the smallest time
}

int getMinValveOpenPSI(bool quickMode)
{
    return calculateValveOpenTimeMS(0, quickMode);
}

Wheel::Wheel() {}

Wheel::Wheel(InputType *solenoidInPin, InputType *solenoidOutPin, InputType *pressurePin, byte thisWheelNum)
{
    this->pressurePin = pressurePin;
    this->thisWheelNum = thisWheelNum;
    this->s_AirIn = Solenoid(solenoidInPin);
    this->s_AirOut = Solenoid(solenoidOutPin);
    this->pressureValue = 0;
    this->pressureGoal = 0;
    this->routineStartTime = 0;
    this->flagStartPressureGoalRoutine = false;
    this->quickMode = false;
}

Solenoid *Wheel::getInSolenoid()
{
    return &this->s_AirIn;
}

Solenoid *Wheel::getOutSolenoid()
{
    return &this->s_AirOut;
}

InputType *Wheel::getPressurePin()
{
    return this->pressurePin;
}

float analogToPressure(int nativeAnalogValue)
{
    float floored = float(nativeAnalogValue) - pressureZeroAnalogValue;  // chop off the 0 value
    float totalRange = pressureMaxAnalogValue - pressureZeroAnalogValue; // get the total analog voltage difference between min and max
    float normalized = floored / totalRange;                             // 0 to 1 where 0 is 0psi and 1 is max psi
    float psi = normalized * pressuretransducermaxPSI;                   // multiply out 0 to 1 by our max psi

    return psi;
}

float readPinPressure(InputType *pin)
{
    return analogToPressure(pin->analogRead());
}

void Wheel::readPressure()
{
    this->pressureValue = readPinPressure(this->pressurePin);
}

float Wheel::getPressure()
{
    return this->pressureValue;
}

bool Wheel::isActive()
{
    if (this->s_AirIn.isOpen())
    {
        return true;
    }
    if (this->s_AirOut.isOpen())
    {
        return true;
    }
    return false;
}

void Wheel::initPressureGoal(int newPressure, bool quick)
{
    if (newPressure > MAX_PRESSURE_SAFETY)
    {
        // hardcode not to go above set psi
        return;
    }
    int pressureDif = newPressure - this->getPressure(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI(quick))
    {
        // okay we need to set the values, but only if we are airing out or if the tank has more pressure than what is currently in the bags
        if (pressureDif < 0 || getTankPressure() > this->getPressure())
        {
            this->pressureGoal = newPressure;
            this->quickMode = quick;
            this->routineStartTime = millis();
            this->flagStartPressureGoalRoutine = true;
        }
    }
}

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::loop()
{
    // Serial.println("WheelP: ");
    this->readPressure();
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
            this->readPressure();
            int pressureDif = this->pressureGoal - this->getPressure();
            int pressureDifABS = abs(pressureDif);

            if (pressureDifABS > getMinValveOpenPSI(this->quickMode))
            {
                // Decide which valve to use
                Solenoid *valve;
                if (this->pressureGoal > this->getPressure())
                {
                    valve = &this->s_AirIn;
                }
                else
                {
                    valve = &this->s_AirOut;
                }

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
                // Completed
                break;
            }
        }
    }
}