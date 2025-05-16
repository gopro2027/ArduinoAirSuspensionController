#ifndef solenoid_h
#define solenoid_h

#include "input_type.h"

class Solenoid
{
private:
    InputType *pin;
    bool bopen;
    SOLENOID_AI_INDEX aiIndex = SOLENOID_AI_INDEX::AI_MODEL_UNDEFINED;

public:
    Solenoid();
    Solenoid(InputType *pin, SOLENOID_AI_INDEX aiIndex = SOLENOID_AI_INDEX::AI_MODEL_UNDEFINED);
    void open();
    void close();
    bool isOpen();
    SOLENOID_AI_INDEX getAIIndex();
};

#endif
