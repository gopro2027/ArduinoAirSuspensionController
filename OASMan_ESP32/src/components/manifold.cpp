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

#if SIX_VALVE_MANIFOLD == true
Manifold::Manifold(InputType *fp,
                   InputType *rp,
                   InputType *fd,
                   InputType *rd,
                   InputType *chamberTankInput,
                   InputType *chamberExhaustInput
                )
{
    chamberTank = new ChamberValve(chamberTankInput);
    chamberExhaust = new ChamberValve(chamberExhaustInput);
    this->solenoidList[FRONT_PASSENGER_IN] = new Solenoid(fp, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_PASSENGER_OUT] = new Solenoid(fp, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_PASSENGER_IN] = new Solenoid(rp, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_PASSENGER_OUT] = new Solenoid(rp, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
    this->solenoidList[FRONT_DRIVER_IN] = new Solenoid(fd, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT);
    this->solenoidList[FRONT_DRIVER_OUT] = new Solenoid(fd, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT);
    this->solenoidList[REAR_DRIVER_IN] = new Solenoid(rd, chamberTank, SOLENOID_AI_INDEX::AI_MODEL_UP_REAR);
    this->solenoidList[REAR_DRIVER_OUT] = new Solenoid(rd, chamberExhaust, SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR);
    chamberCheckMutex = xSemaphoreCreateMutex();
}
#endif

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

#if SIX_VALVE_MANIFOLD == true
bool Manifold::canOpenDirectionSixValveThreadSafe(Solenoid *toPreMarkAsOpening) {
    while (xSemaphoreTake(chamberCheckMutex, 1) != pdTRUE)
    {
        delay(1);
    }
    bool ret = true;
    if (toPreMarkAsOpening->getChamberValve() == chamberTank) {
        // we are trying to open the tank valve. Check if exhaust valve is currently open
        if (chamberExhaust->isOpen()) {
            ret = false;
        } else {
            // it's not open to pre-mark the tank chamber that it is going to be selected as the one open
            chamberTank->preMarkSolenoidAsGoingToOpen(toPreMarkAsOpening);
            ret = true;
        }
    } else {
        // similar routine for the opposite, we are trying to open exhaust so check if tank is currently open
        if (chamberTank->isOpen()) {
            ret = false;
        } else {
            chamberExhaust->preMarkSolenoidAsGoingToOpen(toPreMarkAsOpening);
            ret = true;
        }
    }
    xSemaphoreGive(chamberCheckMutex);
    return ret;
}
#endif