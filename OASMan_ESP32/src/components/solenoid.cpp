#include "solenoid.h"
#include <Wire.h>
#include <SPI.h>
#include "chamber_valve.h"

Solenoid::Solenoid() {}

Solenoid::Solenoid(InputType *pin, ChamberValve *chamber_valve, SOLENOID_AI_INDEX aiIndex)
{
    this->pin = pin;
    this->chamber_valve = chamber_valve;
    this->aiIndex = aiIndex;

    // default solenoid to low state... this is important after a software reboot, it could be stuck as HIGH because it is not automatically reset to LOW
    this->bopen = false;
    this->pin->digitalWrite(LOW);
}

Solenoid::Solenoid(InputType *pin, SOLENOID_AI_INDEX aiIndex) : Solenoid(pin, NULL, aiIndex)
{
}

void Solenoid::open()
{
    if (this->bopen == false)
    {
        this->pin->digitalWrite(HIGH);
        if (this->chamber_valve != NULL) {
            this->chamber_valve->open(this);
        }
        this->bopen = true;
    }
}
void Solenoid::close()
{
    if (this->bopen == true)
    {
        this->pin->digitalWrite(LOW);
        if (this->chamber_valve != NULL) {
            this->chamber_valve->close(this);
        }
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
