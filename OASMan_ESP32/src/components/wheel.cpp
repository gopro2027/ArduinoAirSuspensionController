#include "wheel.h"
#include <Wire.h>
#include <SPI.h>

int getTankPressure(); // from main

struct PressureGoalValveTiming
{
    int pressureDelta;          // the pressure percision that we are trying to achieve
    int valveTimingUntilWithin; // the amount of time we will leave the valve open until we get within the range of goalPressure +- pressureDelta
};

// Must put in sorted order of largest to smallest
PressureGoalValveTiming valveTiming[] = {
    {10, 500}, // if current psi outside range of goalPressure +- 10psi, open valves for 500ms until +- 10psi achieved
    {5, 100},  // if current psi outside range of goalPressure +- 6psi, open valves for 100ms until +- 6psi achieved
    {1, 10},   // if current psi outside range of goalPressure +- 1psi, open valves for 100ms until +- 1psi achieved
};
#define VALVE_TIMING_LIST_COUNT 3 // the number of valve timing you have defined above
int getMinValveOpenPSI()
{
    return valveTiming[VALVE_TIMING_LIST_COUNT - 1].pressureDelta;
}

// This function can be updated in the future to use some better algorithm to decide how long to open the valves for to reach the desigred pressure
int Wheel::calculateValveOpenTimeMS(int pressureDifferenceAbsolute)
{
    for (int i = 0; i < VALVE_TIMING_LIST_COUNT; i++)
    {
        if (pressureDifferenceAbsolute > valveTiming[i].pressureDelta)
        {
            return valveTiming[i].valveTimingUntilWithin;
        }
    }
    return 0;
}

const int pressureAdjustment = 0; // static adjustment

Wheel::Wheel() {}

Wheel::Wheel(InputType *solenoidInPin, InputType *solenoidOutPin, InputType *pressurePin, byte thisWheelNum)
{
    this->pressurePin = pressurePin;
    this->thisWheelNum = thisWheelNum;
    this->s_AirIn = Solenoid(solenoidInPin);
    this->s_AirOut = Solenoid(solenoidOutPin);
    this->pressureValue = 0;
    this->pressureGoal = 0;
    this->flagStartPressureGoalRoutine = false;
}

float readPinPressure(InputType *pin)
{
    return float((float(pin->analogRead()) - pressureZeroAnalogValue) * pressuretransducermaxPSI) / (pressureMaxAnalogValue - pressureZeroAnalogValue) + pressureAdjustment; // conversion equation to convert analog reading to psi
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
    // TODO: Implement the quick part
    if (newPressure > MAX_PRESSURE_SAFETY)
    {
        // hardcode not to go above 150PSI
        return;
    }
    int pressureDif = newPressure - this->getPressure(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI())
    {
        // okay we need to set the values
        if (pressureDif < 0 || getTankPressure() > newPressure)
        {
            this->pressureGoal = newPressure;
            this->flagStartPressureGoalRoutine = true;
        }
    }
}

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::loop()
{
    this->readPressure();
    if (this->flagStartPressureGoalRoutine)
    {
        this->flagStartPressureGoalRoutine = false;
        unsigned long routineStartTime = millis();
        for (;;)
        {
            // 10 second timeout in case tank doesn't have a whole lot of air or something
            if (millis() > routineStartTime + ROUTINE_TIMEOUT_MS)
            {
                break;
            }

            // Main routine
            this->readPressure();
            int pressureDif = this->pressureGoal - this->getPressure();
            int pressureDifABS = abs(pressureDif);

            if (pressureDifABS > getMinValveOpenPSI())
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
                int valveTime = this->calculateValveOpenTimeMS(pressureDifABS);

                // Open valve for calculated time
                valve->open();
                task_sleep(valveTime);
                valve->close();

                // Sleep 150ms to allow time for valve to fully close and pressure to equalize a bit
                task_sleep(150);
            }
            else
            {
                // Completed
                break;
            }
        }
    }
}