#ifndef solenoid_h
#define solenoid_h

#include "input_type.h"
class ChamberValve;

class Solenoid
{
private:
    InputType *pin;
    ChamberValve *chamber_valve;
    bool bopen;
    SOLENOID_AI_INDEX aiIndex = SOLENOID_AI_INDEX::AI_MODEL_UNDEFINED;

public:
    Solenoid();
    Solenoid(InputType *pin, SOLENOID_AI_INDEX aiIndex = SOLENOID_AI_INDEX::AI_MODEL_UNDEFINED);
    Solenoid(InputType *pin, ChamberValve *chamber_valve, SOLENOID_AI_INDEX aiIndex = SOLENOID_AI_INDEX::AI_MODEL_UNDEFINED);
    void open();
    void close();
    bool isOpen();
    SOLENOID_AI_INDEX getAIIndex();

    ChamberValve *getChamberValve() {
        return this->chamber_valve;
    }
};

#endif
