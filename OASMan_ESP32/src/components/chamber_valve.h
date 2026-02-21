#ifndef chamber_valve_h
#define chamber_valve_h

#include "input_type.h"
#include <user_defines.h>
class Solenoid;

#define NUM_REFERENCES 4

class ChamberValve
{
private:
    InputType *pin;
    bool bopen;
    Solenoid *references[NUM_REFERENCES]; // 4 bags so 4 references
    SemaphoreHandle_t referenceMutex;
#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
    bool compressorHold; // when true, valve stays open for compressor filling tank
    bool hasAnyReferences();
#endif

    bool checkAndAddReference(Solenoid *reference);
    bool checkAndRemoveReference(Solenoid *reference);

public:
    ChamberValve();
    ChamberValve(InputType *pin);
    void open(Solenoid *reference);
    void close(Solenoid *reference);
    bool isOpen();
    void preMarkSolenoidAsGoingToOpen(Solenoid *reference);
#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
    void setCompressorHold(bool hold);
#endif
};

#endif