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
    this->solenoidList[FRONT_PASSENGER_IN] = new Solenoid(fpi);
    this->solenoidList[FRONT_PASSENGER_OUT] = new Solenoid(fpo);
    this->solenoidList[REAR_PASSENGER_IN] = new Solenoid(rpi);
    this->solenoidList[REAR_PASSENGER_OUT] = new Solenoid(rpo);
    this->solenoidList[FRONT_DRIVER_IN] = new Solenoid(fdi);
    this->solenoidList[FRONT_DRIVER_OUT] = new Solenoid(fdo);
    this->solenoidList[REAR_DRIVER_IN] = new Solenoid(rdi);
    this->solenoidList[REAR_DRIVER_OUT] = new Solenoid(rdo);
    this->wheelSolenoidMask = 0;
}

Solenoid *Manifold::get(int solenoid)
{
    return this->solenoidList[solenoid];
}

Solenoid **Manifold::getAll()
{
    return this->solenoidList;
}

// TODO: Get rid of this function in the future. It's an abstract match between the two things. Not great
Solenoid *getSolenoidFromIndex(int solenoid)
{
    if (solenoid % 2)
    {
        return getWheel(solenoid / 2)->getOutSolenoid();
    }
    else
    {
        return getWheel(solenoid / 2)->getInSolenoid();
    }
}

// // these are depricated/unused. Pause/unpause all valves
// void Manifold::pauseValvesForBlockingTask()
// {
//     this->wheelSolenoidMask = 0;
//     for (int i = 0; i < 8; i++)
//     {
//         bool val = this->solenoidList[i]->digitalRead() == HIGH; // valve is open
//         if (val)
//         {
//             this->wheelSolenoidMask = this->wheelSolenoidMask | (1 << i);
//             this->solenoidList[i]->digitalWrite(LOW);
//         }
//     }
// }
// void Manifold::unpauseValvesForBlockingTaskCompleted()
// {
//     for (int i = 0; i < 8; i++)
//     {
//         if ((this->wheelSolenoidMask & (1 << i)) > 0)
//         {
//             this->solenoidList[i]->digitalWrite(HIGH);
//         }
//     }
//     this->wheelSolenoidMask = 0; // reset bitset
// }