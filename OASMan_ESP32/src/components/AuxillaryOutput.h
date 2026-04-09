#ifndef auxillary_output_h
#define auxillary_output_h

#include "input_type.h"
#include <user_defines.h>
#include "solenoid.h"
#include "manifoldSaveData.h"

class AuxillaryOutput
{
private:
    Solenoid solenoid;
    bool doStartupEvent;
    bool doShutdownEvent;

    bool checkForCloseTime;
    unsigned long closeTime;

public:
    AuxillaryOutput();
    AuxillaryOutput(InputType *pin);
    void loop();
    void openForDuration(uint32_t duration);
    uint32_t getDurationInMillis();
    void setDoStartupEvent(bool doStartupEvent) { this->doStartupEvent = doStartupEvent; }
    void setDoShutdownEvent(bool doShutdownEvent) { this->doShutdownEvent = doShutdownEvent; }
    void onOffOverride(bool on);
};

#endif