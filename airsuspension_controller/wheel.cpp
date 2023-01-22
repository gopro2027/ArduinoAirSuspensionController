#include "wheel.h"
#include <Wire.h>
#include <SPI.h>

int getTankPressure();//from main

const int PRESSURE_DELTA = 3;//Pressure will go to +- 3 psi to verify
const unsigned long ROUTINE_TIMEOUT = 10 * 1000;//10 seconds is too long

Wheel::Wheel() {}

Wheel::Wheel(int solenoidInPin, int solenoidOutPin, int pressurePin) {
  this->solenoidInPin = solenoidInPin;
  this->solenoidOutPin = solenoidOutPin;
  this->pressurePin = pressurePin;
  this->s_AirIn = Solenoid(solenoidInPin);
  this->s_AirOut = Solenoid(solenoidOutPin);
  this->routineStartTime = 0;
  this->pressureValue = 0;
  this->pressureGoal = 0;
}

const float pressureZero = 102.4; //analog reading of pressure transducer at 0psi
const float pressureMax = 921.6; //analog reading of pressure transducer at 100psi
const int pressuretransducermaxPSI = 300; //psi value of transducer being used

float readPinPressure(int pin) {
  return float((float(analogRead(pin))-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero); //conversion equation to convert analog reading to psi
}

void Wheel::readPressure() {
  this->pressureValue = readPinPressure(this->pressurePin);
}

float Wheel::getPressure() {
  return this->pressureValue;
}

bool Wheel::isActive() {
  if (this->s_AirIn.isOpen()) {
    return true;
  }
  if (this->s_AirOut.isOpen()) {
    return true;
  }
  return false;
}

void Wheel::initPressureGoal(int newPressure) {
  if (newPressure > MAX_PRESSURE_SAFETY) {
    //hardcode not to go above 150PSI
    return;
  }
  int pressureDif = newPressure - this->pressureValue;//negative if airing out, positive if airing up
  if (abs(pressureDif) <= PRESSURE_DELTA) {
    //literally do nothing, it's close enough already
    return;
  } else {
    //okay we need to set the values
    this->routineStartTime = millis();
    this->pressureGoal = newPressure;
    if (pressureDif < 0) {
      Serial.println("Airing out front!");
      this->s_AirOut.open();
    } else {
      if (getTankPressure() > newPressure) {
        Serial.println("Airing in");
        this->s_AirIn.open();
      } else {
        Serial.println("not enough tank pressure!");
        //don't even bother trying cuz there won't be enough pressure in the tank lol but i guess it won't hurt anything even if it did it just might act weird
      }
    }
  }
}

void Wheel::pressureGoalRoutine() {
  int readPressure = this->pressureValue;
  if (this->s_AirIn.isOpen()) {
    if (readPressure > MAX_PRESSURE_SAFETY) {
      this->s_AirIn.close();
      displayCode = 1;
    }
    if (readPressure >= this->pressureGoal) {
      //stop
      this->s_AirIn.close();
      displayCode = 2;
      displayCode2 = readPressure;
      displayCode3 = this->pressureGoal;
    } else {
      if (millis() > this->routineStartTime + ROUTINE_TIMEOUT) {
        this->s_AirIn.close();
        displayCode = 3;
      }
    }
  }
  if (this->s_AirOut.isOpen()) {
    if (readPressure <= this->pressureGoal) {
      //stop
      this->s_AirOut.close();
    } else {
      if (millis() > this->routineStartTime + ROUTINE_TIMEOUT) {
        this->s_AirOut.close();
      }
    }
  }
}
