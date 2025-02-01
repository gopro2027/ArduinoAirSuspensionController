#ifndef manifold_h
#define manifold_h

#include "input_type.h"
#include "solenoid.h"
#include "components/wheel.h"
#include <user_defines.h>

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

Solenoid *getSolenoidFromIndex(int solenoid);

extern Manifold *getManifold(); // defined in airSuspensionUtil.h
extern Wheel *getWheel(int i);

#endif
