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

  //byte pressureAverageCount;//1 byte, 0 to 256 (so pressureAverageTotal can contain 256 values between 0 and 256)
  //byte pressureAverage;// 1 byte, final value

  bool isInSafePressureRead;
  bool isClosePaused;
  
  //unsigned int pressureAverageTotal;//2 bytes, 0 to 65535
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
  //byte getPressureAverage();
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
