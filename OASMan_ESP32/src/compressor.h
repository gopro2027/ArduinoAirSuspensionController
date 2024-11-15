#ifndef compressor_h
#define compressor_h

#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include <Arduino.h>

class Compressor {
private:
  InputType *readPin;
  int currentPressure;
  bool stateOnPause;
  bool isPaused;
  Solenoid s_trigger; // Not a solenoid, but works the same way
public:
  Compressor();
  Compressor(InputType *triggerPin, InputType *readPin);
  void loop();
  void pause(); // call pause and resume for thread blocking tasks
  void resume();
  float readPressure();
};

float readPinPressure(InputType *pin); // this may need to be extern

#endif
