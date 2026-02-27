#include "chamber_valve.h"
#include "solenoid.h"

ChamberValve::ChamberValve() {}

ChamberValve::ChamberValve(InputType *pin)
{
    referenceMutex = xSemaphoreCreateMutex();
    this->pin = pin;
#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
    this->compressorHold = false;
#endif

    for (int i = 0; i < NUM_REFERENCES; i++) {
        references[i] = NULL;
    }

    // default solenoid to low state... this is important after a software reboot, it could be stuck as HIGH because it is not automatically reset to LOW
    this->bopen = false;
    this->pin->digitalWrite(LOW);
}

#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
bool ChamberValve::hasAnyReferences() {
    for (int i = 0; i < NUM_REFERENCES; i++) {
        if (references[i] != NULL) return true;
    }
    return false;
}
#endif

// returns if successful
bool ChamberValve::checkAndAddReference(Solenoid *reference) {
    while (xSemaphoreTake(referenceMutex, 1) != pdTRUE)
    {
        delay(1);
    }

    bool found = false;
    int lowest = -1;

    // first loop through all references to see if it already exists.
    for (int i = 0; i < NUM_REFERENCES; i++) {
        if (references[i] == NULL && lowest == -1) {
            lowest = i;
        }
        if (references[i] == reference) {
            found = true;
            break;
        }
    }

    // if not found, add it to the lowest empty slot
    bool ret = true;
    if (!found) {
        if (lowest != -1) {
            references[lowest] = reference;
        } else {
            ret = false;
        }
    }

    xSemaphoreGive(referenceMutex);

    return ret;
}

// returns if empty
bool ChamberValve::checkAndRemoveReference(Solenoid *reference) {
    while (xSemaphoreTake(referenceMutex, 1) != pdTRUE)
    {
        delay(1);
    }

    bool empty = true;
    for (int i = 0; i < NUM_REFERENCES; i++) {
        if (references[i] == reference) {
            references[i] = NULL;
        }
        if (references[i] != NULL) {
            empty = false;
        }
    }

    xSemaphoreGive(referenceMutex);

    return empty;
}

void ChamberValve::open(Solenoid *reference)
{
    checkAndAddReference(reference); // this will return false if it fails but otherwise we don't care about the output
    if (this->bopen == false)
    {
        this->pin->digitalWrite(HIGH);
        this->bopen = true;
    }
}
void ChamberValve::close(Solenoid *reference)
{
    if (checkAndRemoveReference(reference)) {
#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
        if (this->bopen == true && !this->compressorHold && !hasAnyReferences())
#else
        if (this->bopen == true)
#endif
        {
            this->pin->digitalWrite(LOW);
            this->bopen = false;
        }
    }
}

#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
void ChamberValve::setCompressorHold(bool hold)
{
    while (xSemaphoreTake(referenceMutex, 1) != pdTRUE)
    {
        delay(1);
    }
    this->compressorHold = hold;
    if (hold) {
        this->pin->digitalWrite(HIGH);
        this->bopen = true;
    } else if (!hasAnyReferences()) {
        this->pin->digitalWrite(LOW);
        this->bopen = false;
    }
    xSemaphoreGive(referenceMutex);
}
#endif
bool ChamberValve::isOpen()
{
    return this->bopen;
}

void ChamberValve::preMarkSolenoidAsGoingToOpen(Solenoid *reference) {
    checkAndAddReference(reference); // we don't care about the output here
}