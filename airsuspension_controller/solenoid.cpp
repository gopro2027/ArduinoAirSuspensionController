#include "solenoid.h"
#include <Wire.h>
#include <SPI.h>

Solenoid::Solenoid() {}

Solenoid::Solenoid(int pin) {
  pinMode(pin, OUTPUT);
  this->pin = pin;
  this->bopen = false;
}
void Solenoid::open() {
  digitalWrite(this->pin, HIGH);
  this->bopen = true;
}
void Solenoid::close() {
  digitalWrite(this->pin, LOW);
  this->bopen = false;
}
bool Solenoid::isOpen() {
  return this->bopen;
}
