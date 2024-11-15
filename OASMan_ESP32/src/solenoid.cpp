#include "solenoid.h"
#include <Wire.h>
#include <SPI.h>

Solenoid::Solenoid() {}

Solenoid::Solenoid(InputType *pin) {
  this->pin = pin;
  this->bopen = false;
}
void Solenoid::open() {
  if (this->bopen == false) {
    this->pin->digitalWrite(HIGH);
    this->bopen = true;
  }
}
void Solenoid::close() {
  if (this->bopen == true) {
    this->pin->digitalWrite(LOW);
    this->bopen = false;
  }
}
bool Solenoid::isOpen() {
  return this->bopen;
}
