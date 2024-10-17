#include "manifold.h"

Manifold::Manifold() {}

Manifold::Manifold(InputType * fpi,
                    InputType * fpo,
                    InputType * rpi,
                    InputType * rpo,
                    InputType * fdi,
                    InputType * fdo,
                    InputType * rdi,
                    InputType * rdo) {
    this->solenoidList[FRONT_PASSENGER_IN] = fpi;
    this->solenoidList[FRONT_PASSENGER_OUT] = fpo;
    this->solenoidList[REAR_PASSENGER_IN] = rpi;
    this->solenoidList[REAR_PASSENGER_OUT] = rpo;
    this->solenoidList[FRONT_DRIVER_IN] = fdi;
    this->solenoidList[FRONT_DRIVER_OUT] = fdo;
    this->solenoidList[REAR_DRIVER_IN] = rdi;
    this->solenoidList[REAR_DRIVER_OUT] = rdo;
}

InputType *Manifold::get(SOLENOID_INDEX solenoid) {
    return this->solenoidList[solenoid];
}

InputType **Manifold::getAll() {
    return this->solenoidList;
}