#include "compressor.h"
#include <Wire.h>
#include <SPI.h>

Compressor::Compressor() {}

Compressor::Compressor(InputType *triggerPin, InputType *readPin)
{
    this->readPin = readPin;
    this->s_trigger = Solenoid(triggerPin);
    this->currentPressure = TANK_READING_NOT_READY_YET_VALUE;
    this->pressureArrayCounter = 0;
    this->freezeTimerLastReadValue = 0;
    this->lastFreezeTime = 0;
    this->pauseExecutionUntilTime = 0;
}

InputType *Compressor::getReadPin()
{
    return this->readPin;
}

float Compressor::readPressure()
{
    return readPinPressure(this->readPin);
}

float mockTankPressure = 0;
float Compressor::getTankPressure()
{
#if TANK_PRESSURE_MOCK == true
    mockTankPressure++;
    if (mockTankPressure > 300)
    {
        mockTankPressure = 0;
    }
    return mockTankPressure;
#else
    if (this->currentPressure < 0)
    {
        return 0;
    }
    else
    {
        return this->currentPressure;
    }
#endif
}

void Compressor::loop()
{
    unsigned long curTime = millis();

    // READING TANK PRESSURE LOGIC:

    // if (!isAnyWheelActive()) // TODO: need to add this check on reading but also at the same time make it safe and turn off the compressor. or maybe just dont add it bc the only issue is innacurate read values when valves are open, and who really cares right???
    sampleReading(this->currentPressure, this->readPressure(), this->pressureArray, this->pressureArrayCounter, PRESSURE_AVERAGE_ARRAY_SIZE);

    // Turn compressor off if i haven't completed a proper reading yet
    if (this->currentPressure == TANK_READING_NOT_READY_YET_VALUE)
    {
        // ok I know you aren't supposed to use equals on floats but i am checking the exact value so it's fine
        this->s_trigger.close();
        return;
    }

    // COMPRESSOR CONTROL LOGIC:

    // no matter which state compressor is in, check if it is up to max psi and turn it off if needed and return without any further execution. This is most important tank check and should ideally be ran first to turn off in any case where pressure is too high.
    if (this->getTankPressure() >= COMPRESSOR_MAX_PSI)
    {
        this->s_trigger.close();
        return;
    }

    // if compressor is on, check if it is frozen by checking every 15 seconds or so the value, and if the change is less than 3psi then tell the compressor to pause for a bit. if compressor is not running, continually update last read time and pressure.
    if (this->s_trigger.isOpen())
    {
        if (this->lastFreezeTime + FREEZE_TIME_CHECK_MS < curTime)
        {

            int changeOverLastTimeframe = abs(this->currentPressure - this->freezeTimerLastReadValue);

            if (changeOverLastTimeframe < 3)
            {
                // compressor is deemed frozen, tell it to pause execution until set time in the future
                this->pauseExecutionUntilTime = curTime + FREEZE_TIME_PAUSE_MS;
            }

            this->lastFreezeTime = curTime;
            this->freezeTimerLastReadValue = this->currentPressure;
        }
    }
    else
    {
        // compressor is not on, just set values to current
        this->lastFreezeTime = curTime;
        this->freezeTimerLastReadValue = this->currentPressure;
    }

    // part 2 of freeze check code. pause execution if our pauseExecutionUntilTime is in the future
    if (curTime < this->pauseExecutionUntilTime)
    {
        this->s_trigger.close();
        return;
    }

    // if vehicle is off disable compressor
    if (!isVehicleOn())
    {
        this->s_trigger.close();
        return;
    }

    // run the actual compressor routine...

    // if compressor off, check if tank psi is below low psi and turn on if needed
    if (!this->s_trigger.isOpen())
    {
        if (this->getTankPressure() < COMPRESSOR_ON_BELOW_PSI)
        {
            this->s_trigger.open();
        }
    }
}