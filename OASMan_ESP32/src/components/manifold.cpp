#include "manifold.h"

Manifold::Manifold() {}

Manifold::Manifold(InputType *fpi,
                   InputType *fpo,
                   InputType *rpi,
                   InputType *rpo,
                   InputType *fdi,
                   InputType *fdo,
                   InputType *rdi,
                   InputType *rdo)
{
    this->solenoidList[FRONT_PASSENGER_IN] = fpi;
    this->solenoidList[FRONT_PASSENGER_OUT] = fpo;
    this->solenoidList[REAR_PASSENGER_IN] = rpi;
    this->solenoidList[REAR_PASSENGER_OUT] = rpo;
    this->solenoidList[FRONT_DRIVER_IN] = fdi;
    this->solenoidList[FRONT_DRIVER_OUT] = fdo;
    this->solenoidList[REAR_DRIVER_IN] = rdi;
    this->solenoidList[REAR_DRIVER_OUT] = rdo;
    this->wheelSolenoidMask = 0;
}

InputType *Manifold::get(SOLENOID_INDEX solenoid)
{
    return this->solenoidList[solenoid];
}

InputType **Manifold::getAll()
{
    return this->solenoidList;
}

// these are depricated/unused. Pause/unpause all valves
void Manifold::pauseValvesForBlockingTask()
{
    this->wheelSolenoidMask = 0;
    for (int i = 0; i < 8; i++)
    {
        bool val = this->solenoidList[i]->digitalRead() == HIGH; // valve is open
        if (val)
        {
            this->wheelSolenoidMask = this->wheelSolenoidMask | (1 << i);
            this->solenoidList[i]->digitalWrite(LOW);
        }
    }
}
void Manifold::unpauseValvesForBlockingTaskCompleted()
{
    for (int i = 0; i < 8; i++)
    {
        if ((this->wheelSolenoidMask & (1 << i)) > 0)
        {
            this->solenoidList[i]->digitalWrite(HIGH);
        }
    }
    this->wheelSolenoidMask = 0; // reset bitset
}