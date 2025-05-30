#include "solenoid.h"
#include <Wire.h>
#include <SPI.h>

Solenoid::Solenoid() {}

Solenoid::Solenoid(InputType *pin, SOLENOID_AI_INDEX aiIndex)
{
    this->pin = pin;
    this->aiIndex = aiIndex;

    // default solenoid to low state... this is important after a software reboot, it could be stuck as HIGH because it is not automatically reset to LOW
    this->bopen = false;
    this->pin->digitalWrite(LOW);
}
void Solenoid::open()
{
    if (this->bopen == false)
    {
        this->pin->digitalWrite(HIGH);
        this->bopen = true;
    }
}
void Solenoid::close()
{
    if (this->bopen == true)
    {
        this->pin->digitalWrite(LOW);
        this->bopen = false;
    }
}
bool Solenoid::isOpen()
{
    return this->bopen;
}
SOLENOID_AI_INDEX Solenoid::getAIIndex() 
{
    return this->aiIndex;
}
