#include "wheel.h"
#include <Wire.h>
#include <SPI.h>

int getTankPressure();//from main

const int PRESSURE_DELTA = 3;//Pressure will go to +- psi to verify before starting a routine
const unsigned long ROUTINE_TIMEOUT = 10 * 1000;//10 seconds is too long
const int pressureAdjustment = 0;//static adjustment

Wheel::Wheel() {}

Wheel::Wheel(InputType *solenoidInPin, InputType *solenoidOutPin, InputType *pressurePin, byte thisWheelNum) {
  this->pressurePin = pressurePin;
  this->thisWheelNum = thisWheelNum;
  this->s_AirIn = Solenoid(solenoidInPin);
  this->s_AirOut = Solenoid(solenoidOutPin);
  this->routineStartTime = 0;
  this->pressureValue = 0;
  this->pressureGoal = 0;
  this->isInSafePressureRead = false;
  this->isClosePaused = false;
}

float readPinPressure(InputType *pin) {
  return float((float(pin->analogRead())-pressureZeroAnalogValue)*pressuretransducermaxPSI)/(pressureMaxAnalogValue-pressureZeroAnalogValue) + pressureAdjustment; //conversion equation to convert analog reading to psi
}

bool Wheel::prepareSafePressureRead() {
  this->isInSafePressureRead = false;
  if (this->s_AirIn.isOpen()) {
    this->s_AirIn.close();
    this->isInSafePressureRead = true;
  }
  return this->isInSafePressureRead;
}

void Wheel::safePressureClose() {
  if (this->isInSafePressureRead) {
    this->s_AirIn.open();
    this->isInSafePressureRead = false;
  }
}

void Wheel::safePressureReadPauseClose() {
  this->isClosePaused = false;
  if (this->s_AirOut.isOpen()) {
    this->s_AirOut.close();
    this->isClosePaused = true;
  }
}

void Wheel::safePressureReadResumeClose() {
  if (this->isClosePaused) {
    this->s_AirOut.open();
    this->isClosePaused = false;
  }
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

#define sleepTimeAirDelta 10
#define sleepTimeWait 150

void Wheel::percisionGoToPressureQue(byte goalPressure) {
  this->pressureGoal = goalPressure;
  setGoToPressureGoalPercise(this->thisWheelNum);
}
void Wheel::percisionGoToPressure() {
  int goalPressure = this->pressureGoal;

  getManifold()->pauseValvesForBlockingTask();
  
  unsigned long startTime = millis();
  delay(sleepTimeWait);
  while (millis() - startTime < 5000) {
    int currentPressure = readPinPressure(this->pressurePin);
    if (currentPressure == goalPressure) {
      break;
    }
    if (currentPressure < goalPressure) {
      //air up
      this->s_AirIn.open();
      delay(sleepTimeAirDelta);
      this->s_AirIn.close();
    } else if (currentPressure > goalPressure) {
      //air out
      this->s_AirOut.open();
      delay(sleepTimeAirDelta);
      this->s_AirOut.close();
    }
    delay(sleepTimeWait);
  }

  getManifold()->unpauseValvesForBlockingTaskCompleted();
  
}

void Wheel::initPressureGoal(int newPressure) {
  skipPerciseSet = false;
  if (newPressure > MAX_PRESSURE_SAFETY) {
    //hardcode not to go above 150PSI
    return;
  }
  int pressureDif = newPressure - this->pressureValue;//negative if airing out, positive if airing up
  if (abs(pressureDif) <= PRESSURE_DELTA) {
    this->percisionGoToPressureQue(newPressure);
  } else {
    //okay we need to set the values
    this->routineStartTime = millis();
    this->pressureGoal = newPressure;
    if (pressureDif < 0) {
      this->s_AirOut.open();
    } else {
      if (getTankPressure() > newPressure) {
        this->s_AirIn.open();
      } else {
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
    }
    if (readPressure >= this->pressureGoal) {
      //stop
      this->s_AirIn.close();
      this->percisionGoToPressureQue(this->pressureGoal);
    } else {
      if (millis() > this->routineStartTime + ROUTINE_TIMEOUT) {
        this->s_AirIn.close();
      }
    }
  }
  if (this->s_AirOut.isOpen()) {
    if (readPressure <= this->pressureGoal) {
      //stop
      this->s_AirOut.close();
      this->percisionGoToPressureQue(this->pressureGoal);
    } else {
      if (millis() > this->routineStartTime + ROUTINE_TIMEOUT) {
        this->s_AirOut.close();
      }
    }
  }
}
