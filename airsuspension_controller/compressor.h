#ifndef compressor_h
#define compressor_h

#include "solenoid.h"
#include <arduino.h>

class Compressor {
private:
  int triggerPin;
  int readPin;
  int currentPressure;
  bool stateOnPause;
  bool isPaused;
  Solenoid s_trigger; // Not a solenoid, but works the same way
public:
  Compressor();
  Compressor(int triggerPin, int readPin);
  void loop();
  void pause(); // call pause and resume for thread blocking tasks
  void resume();
};

// These will not be exact depending on how accurate your pressure sensors are.
// For example: Mine will read 220psi when the actual pressure is 180psi
#define COMPRESSOR_ON_BELOW_PSI 160
#define COMPRESSOR_MAX_PSI 200

float readPinPressure(int pin); // this may need to be extern

#endif
