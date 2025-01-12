#include "BTOas.h"

uint8_t *BTOasPacket::tx()
{
    return (uint8_t *)&cmd;
}

// Outgoing packets
StatusPacket::StatusPacket()
{
    this->cmd = STATUSREPORT;
    // 0 through 4
    this->args[WHEEL_FRONT_PASSENGER].f = getWheel(WHEEL_FRONT_PASSENGER)->getPressure();
    this->args[WHEEL_REAR_PASSENGER].f = getWheel(WHEEL_REAR_PASSENGER)->getPressure();
    this->args[WHEEL_FRONT_DRIVER].f = getWheel(WHEEL_FRONT_DRIVER)->getPressure();
    this->args[WHEEL_REAR_DRIVER].f = getWheel(WHEEL_REAR_DRIVER)->getPressure();
}

// Incoming packets

int AirsmPacket::getRelativeValue()
{
    return this->args[0].i;
}

int ProfilePacket::getProfileIndex()
{
    return this->args[0].i;
}

bool BooleanPacket::getBoolean()
{
    return this->args[0].i != 0;
}

int SetAirheightPacket::getWheelIndex()
{
    return this->args[0].i;
}
int SetAirheightPacket::getPressure()
{
    return this->args[1].i;
}

void runReceivedPacket(BTOasPacket *packet)
{
    switch (packet->cmd)
    {
    case BTOasIdentifier::AIRUP:
        airUp();
        break;
    case BTOasIdentifier::AIROUT:
        airOut();
        break;
    case BTOasIdentifier::AIRSM:
        airUpRelativeToAverage(((AirsmPacket *)packet)->getRelativeValue());
        break;
    case BTOasIdentifier::SAVETOPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        writeProfile(((SaveToProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::READPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        readProfile(((ReadProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::AIRUPQUICK: // add if (profileIndex > MAX_PROFILE_COUNT)
        // load profile then air up
        readProfile(((AirupQuickPacket *)packet)->getProfileIndex());
        airUp(true);
        break;
    case BTOasIdentifier::BASEPROFILE:
        setBaseProfile(((BaseProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::SETAIRHEIGHT:
    {
        SetAirheightPacket *ahp = (SetAirheightPacket *)packet;
        switch (ahp->getWheelIndex())
        {
        case WHEEL_FRONT_PASSENGER:
            setRideHeightFrontPassenger(ahp->getPressure());
            break;
        case WHEEL_REAR_PASSENGER:
            setRideHeightRearPassenger(ahp->getPressure());
            break;
        case WHEEL_FRONT_DRIVER:
            setRideHeightFrontDriver(ahp->getPressure());
            break;
        case WHEEL_REAR_DRIVER:
            setRideHeightRearDriver(ahp->getPressure());
            break;
        }
    }
    break;
    case BTOasIdentifier::RISEONSTART:
        setRiseOnStart(((RiseOnStartPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::RAISEONPRESSURESET:
        setRaiseOnPressureSet(((RaiseOnPressureSetPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::REBOOT:
        setReboot(true);
        Serial.println(F("Rebooting..."));
        break;
    case BTOasIdentifier::CALIBRATE:
        Serial.println(F("calibrate does nothign lmao"));
        break;
    case BTOasIdentifier::STARTWEB:
        Serial.println(F("Starting OTA..."));
        startOTAServiceRequest = true;
        break;
    }
}