#include "wheel.h"
#include <Wire.h>
#include <SPI.h>

int getTankPressure();//from main

const int PRESSURE_DELTA = 3;//Pressure will go to +- psi to verify before starting a routine
const unsigned long ROUTINE_TIMEOUT = 10 * 1000;//10 seconds is too long
const int pressureAdjustment = -10;//my sensors are reading about -10 too high

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

#define pressureZero (float)409.6 //analog reading of pressure transducer at 0psi.         for nano: (0.5/5)*1024 = 102.4. for esp32: (0.5/5)*4096 = 409.6
#define pressureMax (float)3686.4 //analog reading of pressure transducer at max psi (300). for nano: (4.5/5)*1024 = 921.6. for esp32: (4.5/5)*4096 = 3686.4
#define pressuretransducermaxPSI 300 //psi value of transducer being used

float readPinPressure(InputType *pin) {
  return float((float(pin->analogRead())-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero) + pressureAdjustment; //conversion equation to convert analog reading to psi
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

  // TODO absolutely must refactor this code to access the solenoids properly
  // DISABLED TEMPORARILY, MUST REENABLE IN THE FUTURE. 10/14/2024
  /*int wheelSolenoidMask = 0;
  for (int i = 0; i < 8; i++) {
    bool val = digitalRead(i+6) == HIGH;// solenoidFrontPassengerInPin
    if (val) {
      wheelSolenoidMask = wheelSolenoidMask | (1 << i);
      digitalWrite(i+6, LOW);
    }
  }*/
  
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

  /*for (int i = 0; i < 8; i++) {
    bool val = (wheelSolenoidMask & (1 << i)) > 0;
    if (val) {
      digitalWrite(i+6, HIGH);
    }
  }*/
  
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
