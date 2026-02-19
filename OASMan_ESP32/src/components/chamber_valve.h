#ifndef chamber_valve_h
#define chamber_valve_h

#include "input_type.h"
class Solenoid;

#define NUM_REFERENCES 4

class ChamberValve
{
private:
    InputType *pin;
    bool bopen;
    Solenoid *references[NUM_REFERENCES]; // 4 bags so 4 references
    SemaphoreHandle_t referenceMutex;

    bool checkAndAddReference(Solenoid *reference);
    bool checkAndRemoveReference(Solenoid *reference);

public:
    ChamberValve();
    ChamberValve(InputType *pin);
    void open(Solenoid *reference);
    void close(Solenoid *reference);
    bool isOpen();
    void preMarkSolenoidAsGoingToOpen(Solenoid *reference);
};

#endif