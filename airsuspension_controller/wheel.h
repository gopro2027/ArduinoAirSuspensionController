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
public:
  Wheel();
  Wheel(int solenoidInPin, int solenoidOutPin, int pressurePin);
  void initPressureGoal(int newPressure);
  void pressureGoalRoutine();
  void readPressure();
  float getPressure();
  bool isActive();
};

const int MAX_PRESSURE_SAFETY = 150;
extern int displayCode;

#endif
