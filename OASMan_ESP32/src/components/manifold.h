#ifndef manifold_h
#define manifold_h

#include "input_type.h"
#include "solenoid.h"
#include "components/wheel.h"
#include <user_defines.h>
#include "chamber_valve.h"

class Manifold
{
private:
    Solenoid *solenoidList[SOLENOID_COUNT];

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
    Manifold(InputType *fp,
            InputType *rp,
            InputType *fd,
            InputType *rd,
            InputType *chamberTankInput,
            InputType *chamberExhaustInput
        );
    Solenoid *get(int solenoid);
    Solenoid **getAll();
    void debugOut();
    // void pauseValvesForBlockingTask();
    // void unpauseValvesForBlockingTaskCompleted();
};

// Solenoid *getSolenoidFromIndex(int solenoid);

extern Manifold *getManifold(); // defined in airSuspensionUtil.h
extern Wheel *getWheel(int i);

#endif
