#include "AuxillaryOutput.h"

AuxillaryOutput::AuxillaryOutput(InputType *pin) {
    this->solenoid = Solenoid(pin);
    this->doStartupEvent = true;
    this->doShutdownEvent = false;

    this->checkForCloseTime = false;
    this->closeTime = 0;
    
}

void AuxillaryOutput::loop() {
    // getauxillaryOutputMode()
    // getauxillaryOutputModeTimeUnit()
    // getauxillaryOutputTime()
    // if (getauxillaryOutputMode() == AUX_MODE_MANUAL_SWITCHED) {
    //     if (getauxillaryOutputModeTimeUnit() == AUX_MODE_TIME_SECONDS) {
    //         if (getauxillaryOutputTime() > 0) {
    //             this->solenoid.open();
    //         } else {
    //             this->solenoid.close();
    //         }
    //     }
    // }

    switch (getauxillaryOutputMode()) {
        case AUX_MODE_MANUAL_SWITCHED:
            break;
        case AUX_MODE_MANUAL_TIMED:
            break;
        case AUX_MODE_STARTUP_TIMED:
            if (doStartupEvent) {
                openForDuration(getDurationInMillis());
            }
            break;
        case AUX_MODE_SHUTDOWN_TIMED:
            if (doShutdownEvent) {
                openForDuration(getDurationInMillis());
            }
            break;
    }

    if (checkForCloseTime && millis() > closeTime) {
        this->solenoid.close();
        checkForCloseTime = false;
        closeTime = 0;
    }

    this->doStartupEvent = true;
    this->doShutdownEvent = true;
}

void AuxillaryOutput::openForDuration(uint32_t duration) {
    this->solenoid.open();
    checkForCloseTime = true;
    closeTime = millis() + duration;
}

uint32_t AuxillaryOutput::getDurationInMillis() {
    uint32_t duration = getauxillaryOutputTime();
    AuxillaryOutputModeTimeUnit timeUnit = (AuxillaryOutputModeTimeUnit)getauxillaryOutputModeTimeUnit();
    switch (timeUnit) {
        case AUX_MODE_TIME_SECONDS:
            return duration * 1000;
        case AUX_MODE_TIME_MINUTES:
            return duration * 60000;
        case AUX_MODE_TIME_HOURS:
            return duration * 3600000;
    }
}