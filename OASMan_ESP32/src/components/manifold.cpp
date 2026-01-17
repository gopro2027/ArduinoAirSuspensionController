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
}

Manifold::Manifold(InputType *fp,
                   InputType *rp,
                   InputType *fd,
                   InputType *rd,
                   InputType *chamberTankInput,
                   InputType *chamberExhaustInput
                )
{
    ChamberValve *chamberTank = new ChamberValve(chamberTankInput);
    ChamberValve *chamberExhaust = new ChamberValve(chamberExhaustInput);
    this->solenoidList[FRONT_PASSENGER_IN] = new Solenoid(fp, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_PASSENGER_OUT] = new Solenoid(fp, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_PASSENGER_IN] = new Solenoid(rp, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_PASSENGER_OUT] = new Solenoid(rp, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
    this->solenoidList[FRONT_DRIVER_IN] = new Solenoid(fd, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_DRIVER_OUT] = new Solenoid(fd, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_DRIVER_IN] = new Solenoid(rd, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_DRIVER_OUT] = new Solenoid(rd, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
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
