#include "compressor.h"
#include <Wire.h>
#include <SPI.h>

Compressor::Compressor() {}

Compressor::Compressor(InputType *triggerPin, InputType *readPin)
{
    this->readPin = readPin;
    this->s_trigger = Solenoid(triggerPin);
    this->stateOnPause = false;
    this->isPaused = false;
    this->currentPressure = 0;
}

InputType *Compressor::getReadPin()
{
    return this->readPin;
}

void Compressor::pause()
{
    if (!this->isPaused)
    {
        this->isPaused = true;
        this->stateOnPause = this->s_trigger.isOpen();
        if (this->s_trigger.isOpen())
        {
            this->s_trigger.close();
        }
    }
}

void Compressor::resume()
{
    if (this->isPaused)
    {
        this->isPaused = false;
        if (this->stateOnPause == true)
        {
            this->s_trigger.open();
        }
    }
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
    if (this->isPaused && !this->s_trigger.isOpen())
    { // first check if it is paused, and then go ahead and double check that it is closed, because if it is open (enabled) we still want to do the loop to close it if it reaches max pressure
        return;
    }
    this->currentPressure = this->readPressure();
    if (!this->s_trigger.isOpen())
    {
        if (this->getTankPressure() < COMPRESSOR_ON_BELOW_PSI)
        {
            this->s_trigger.open();
        }
    }

    if (this->getTankPressure() >= COMPRESSOR_MAX_PSI)
    {
        this->s_trigger.close();
    }
}