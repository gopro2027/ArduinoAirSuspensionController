#include "compressor.h"

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
    return readPinPressure(this->readPin, false); // always false, will always read pressure sensor here not level sensor
}

float mockTankPressure = 0;
float Compressor::getTankPressure()
{
#if TANK_PRESSURE_MOCK == true
    // mockTankPressure++;
    // if (mockTankPressure > 300)
    // {
    //     mockTankPressure = 0;
    // }
    // return mockTankPressure;
    return 200;
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

bool Compressor::isFrozen()
{
    return (millis() < this->pauseExecutionUntilTime);
}

bool Compressor::isOn()
{
    return this->s_trigger.isOpen();
}

void Compressor::enableDisableOverride(bool enable)
{
    if (this->getTankPressure() < getcompressorOffPSI())
    {
        if (enable)
        {
            this->pauseExecutionUntilTime = 0;
            this->updateFreezeTimer(millis());
            this->s_trigger.open();
        }
        else
        {
            this->pauseExecutionUntilTime = ULONG_MAX;
            this->updateFreezeTimer(millis());
            this->s_trigger.close();
        }
    }
}

void Compressor::updateFreezeTimer(unsigned long curTime)
{
    this->lastFreezeTime = curTime;
    this->freezeTimerLastReadValue = this->getTankPressure();
}

void Compressor::loop()
{
    unsigned long curTime = millis();

    // READING TANK PRESSURE LOGIC:

    if (this->currentPressure == TANK_READING_NOT_READY_YET_VALUE) // ok I know you aren't supposed to use equals on floats but i am checking the exact value so it's fine
    {
        this->currentPressure = this->readPressure();
    }

    // if (!isAnyWheelActive()) // TODO: need to add this check on reading but also at the same time make it safe and turn off the compressor. or maybe just dont add it bc the only issue is innacurate read values when valves are open, and who really cares right???
    sampleReading(this->currentPressure, this->readPressure(), this->pressureArray, this->pressureArrayCounter, PRESSURE_AVERAGE_ARRAY_SIZE);

    // COMPRESSOR CONTROL LOGIC:

    // Is safety mode is on, we aren't sure the compressor wire is correct, so disable compressor output
    if (getsafetyMode() == true)
    {
        this->s_trigger.close();
        return;
    }

    // no matter which state compressor is in, check if it is up to max psi and turn it off if needed and return without any further execution. This is most important tank check and should ideally be ran first to turn off in any case where pressure is too high.
    if (this->getTankPressure() >= getcompressorOffPSI())
    {
        this->s_trigger.close();
        return;
    }

    // if compressor is on, check if it is frozen by checking every 15 seconds or so the value, and if the change is less than 3psi then tell the compressor to pause for a bit. if compressor is not running, continually update last read time and pressure.
    if (this->s_trigger.isOpen() && !isAnyWheelActive())
    {
        if (this->lastFreezeTime + FREEZE_TIME_CHECK_MS < curTime)
        {

            float changeOverLastTimeframe = this->getTankPressure() - this->freezeTimerLastReadValue;

            if (changeOverLastTimeframe < 1.75f && changeOverLastTimeframe >= 0)
            {
                // compressor is deemed frozen, tell it to pause execution until set time in the future
                this->pauseExecutionUntilTime = curTime + FREEZE_TIME_PAUSE_MS;
            }

            this->updateFreezeTimer(curTime);
        }
    }
    else
    {
        // compressor is not on OR one of the wheels is active (we don't want to bother trying to figure out if it's frozen if it's actively moving around pressures), update value just right now then.
        this->updateFreezeTimer(curTime);
    }

    // part 2 of freeze check code. pause execution if our pauseExecutionUntilTime is in the future
    if (this->isFrozen())
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
        if (this->getTankPressure() < getcompressorOnPSI())
        {
            this->s_trigger.open();
        }
    }
}

Solenoid *Compressor::getOverrideSolenoid()
{
    return &this->s_trigger;
}