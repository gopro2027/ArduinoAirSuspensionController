#ifndef wheel_h
#define wheel_h


#include "solenoid.h"
#include <arduino.h>

class Wheel {
private:
  byte solenoidInPin;
  byte solenoidOutPin;
  byte pressurePin;
  byte thisWheelNum;

  bool isInSafePressureRead;
  bool isClosePaused;
  
  byte pressureGoal;
  
  unsigned long routineStartTime;
  float pressureValue;

  Solenoid s_AirIn;
  Solenoid s_AirOut;
  
public:
  Wheel();
  Wheel(byte solenoidInPin, byte solenoidOutPin, byte pressurePin, byte thisWheelNum);
  void initPressureGoal(int newPressure);
  void pressureGoalRoutine();
  void readPressure();
  float getPressure();
  bool isActive();
  bool prepareSafePressureRead();
  void safePressureClose();
  void safePressureReadPauseClose();
  void safePressureReadResumeClose();
  void calcAvg();
  void percisionGoToPressure();
  void percisionGoToPressureQue(byte goalPressure);
};

#define MAX_PRESSURE_SAFETY 200

extern void setGoToPressureGoalPercise(byte wheelnum);
extern bool skipPerciseSet;

#endif
