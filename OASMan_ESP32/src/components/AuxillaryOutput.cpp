#include "AuxillaryOutput.h"

AuxillaryOutput::AuxillaryOutput(InputType *pin) {
    this->solenoid = Solenoid(pin);
    this->doStartupEvent = true; // true on creation, so will trigger on startup
    this->doShutdownEvent = false;

    this->checkForCloseTime = false;
    this->closeTime = 0;
    
}

void AuxillaryOutput::eventTriggered() {
    // logic may be a little odd, but that is because we want it to avoid writing to memory if interval is 0, so we never increment if interval is set to 0. Typical shorter code would have likely involved a roll-over to 1 at the end and immediately back to 0 but we want to avoid that.
    int count = getauxillaryIntervalCounter();
    if (count >= getauxillaryOutputInterval()) {
        setauxillaryIntervalCounter(0);
        openForDuration(getDurationInMillis());
    } else {
        setauxillaryIntervalCounter(count + 1);
    }
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

    int bittset = getauxillaryOutputMode();
    if (bittset & (1 << AUX_MODE_STARTUP_TIMED)) {
        if (doStartupEvent) {
            eventTriggered();
        }
    }
    if (bittset & (1 << AUX_MODE_SHUTDOWN_TIMED)) {
        if (doShutdownEvent) {
            eventTriggered();
        }
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
        case AUX_MODE_TIME_DECISECONDS:
            return duration * 100;
        case AUX_MODE_TIME_SECONDS:
            return duration * 1000;
        case AUX_MODE_TIME_MINUTES:
            return duration * 60000;
        case AUX_MODE_TIME_HOURS:
            return duration * 3600000;
    }
}

void AuxillaryOutput::onOffOverride(bool on) {
    if (on) {
        this->solenoid.open();
    } else {
        this->solenoid.close();
    }
}