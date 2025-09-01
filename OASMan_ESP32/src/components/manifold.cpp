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
    this->solenoidList[FRONT_PASSENGER_IN] = new Solenoid(fpi, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_PASSENGER_OUT] = new Solenoid(fpo, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_PASSENGER_IN] = new Solenoid(rpi, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_PASSENGER_OUT] = new Solenoid(rpo, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
    this->solenoidList[FRONT_DRIVER_IN] = new Solenoid(fdi, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_DRIVER_OUT] = new Solenoid(fdo, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_DRIVER_IN] = new Solenoid(rdi, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_DRIVER_OUT] = new Solenoid(rdo, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
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

void Manifold::debugOut()
{
    for (int i = 0; i < SOLENOID_COUNT; i++)
    {
        Solenoid *solenoid = this->solenoidList[i];
        if (solenoid)
        {
            Serial.printf("%s ", solenoid->isOpen() ? "1" : "0");
        }
    }
    Serial.println();
}

// TODO: Get rid of this function in the future. It's an abstract match between the two things. Not great
// Solenoid *getSolenoidFromIndex(int solenoid)
// {
//     if (solenoid % 2)
//     {
//         return getWheel(solenoid / 2)->getOutSolenoid();
//     }
//     else
//     {
//         return getWheel(solenoid / 2)->getInSolenoid();
//     }
// }

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