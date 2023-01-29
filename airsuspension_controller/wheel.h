#ifndef wheel_h
#define wheel_h


#include "solenoid.h"

class Wheel {
private:
  int solenoidInPin;
  int solenoidOutPin;
  int pressurePin;
  float pressureValue;
  Solenoid s_AirIn;
  Solenoid s_AirOut;
  unsigned long routineStartTime;
  int pressureGoal;
  bool isInSafePressureRead;
  bool isClosePaused;
public:
  Wheel();
  Wheel(int solenoidInPin, int solenoidOutPin, int pressurePin);
  void initPressureGoal(int newPressure);
  void pressureGoalRoutine();
  void readPressure();
  float getPressure();
  bool isActive();
  bool prepareSafePressureRead();
  void safePressureClose();
  void safePressureReadPauseClose();
  void safePressureReadResumeClose();
};

const int MAX_PRESSURE_SAFETY = 200;
extern int displayCode;
extern int displayCode2;
extern int displayCode3;

#endif
