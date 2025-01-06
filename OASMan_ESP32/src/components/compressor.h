#ifndef compressor_h
#define compressor_h

#include "user_defines.h"
#include "input_type.h"
#include "solenoid.h"
#include <Arduino.h>

class Compressor
{
private:
    InputType *readPin;
    float currentPressure;
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
    float getTankPressure();
    InputType *getReadPin();
};
extern Compressor *getCompressor();           // defined in airSuspensionUtil.h
extern bool isVehicleOn();                    // defined in airSuspensionUtil.h
extern float readPinPressure(InputType *pin); // defined in Wheel.h
#endif
