#ifndef manifold_h
#define manifold_h

#include "input_type.h"

enum SOLENOID_INDEX
{
    FRONT_PASSENGER_IN,
    FRONT_PASSENGER_OUT,
    REAR_PASSENGER_IN,
    REAR_PASSENGER_OUT,
    FRONT_DRIVER_IN,
    FRONT_DRIVER_OUT,
    REAR_DRIVER_IN,
    REAR_DRIVER_OUT
};
#define SOLENOID_COUNT 8

class Manifold
{
private:
    InputType *solenoidList[SOLENOID_COUNT];
    int wheelSolenoidMask = 0;

public:
    Manifold();
    Manifold(InputType *fpi,
             InputType *fpo,
             InputType *rpi,
             InputType *rpo,
             InputType *fdi,
             InputType *fdo,
             InputType *rdi,
             InputType *rdo);
    InputType *get(int solenoid);
    InputType **getAll();
    void pauseValvesForBlockingTask();
    void unpauseValvesForBlockingTaskCompleted();
};

extern Manifold *getManifold(); // defined in airSuspensionUtil.h

#endif
