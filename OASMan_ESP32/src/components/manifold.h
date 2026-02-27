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
#if SIX_VALVE_MANIFOLD == true
    ChamberValve *chamberTank;
    ChamberValve *chamberExhaust;
    SemaphoreHandle_t chamberCheckMutex;
#endif

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

#if SIX_VALVE_MANIFOLD == true
    Manifold(InputType *fp,
            InputType *rp,
            InputType *fd,
            InputType *rd,
            InputType *chamberTankInput,
            InputType *chamberExhaustInput
        );
    bool canOpenDirectionSixValveThreadSafe(Solenoid *toPreMarkAsOpening);
#if SIX_VALVE_MANIFOLD_OPEN_TANK_VALVE_WHEN_COMPRESSOR_IS_RUNNING == true
    void updateCompressorTankValve();
#endif
#endif

    Solenoid *get(int solenoid);
    Solenoid **getAll();
    void debugOut();
};

extern Manifold *getManifold(); // defined in airSuspensionUtil.h
extern Wheel *getWheel(int i);

#endif
