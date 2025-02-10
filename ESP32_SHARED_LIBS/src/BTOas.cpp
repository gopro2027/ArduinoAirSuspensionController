#include "BTOas.h"

uint8_t *BTOasPacket::tx()
{
    return (uint8_t *)&cmd;
}

BTOasValue16 *BTOasPacket::args16()
{
    return (BTOasValue16 *)this->args;
}
BTOasValue32 *BTOasPacket::args32()
{
    return (BTOasValue32 *)this->args;
}

BTOasPacket::BTOasPacket()
{
    // blank packet
    this->cmd = IDLE;
    this->sender = 0;
    this->recipient = 0;
    memset(this->args, 0, sizeof(this->args));
}

void BTOasPacket::dump()
{
    for (int i = 0; i < BTOAS_PACKET_SIZE; i++)
    {
        Serial.print(this->tx()[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
}

// Outgoing packets
StatusPacket::StatusPacket(float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE, float TANK_PRESSURE, uint16_t bittset)
{
    this->cmd = STATUSREPORT;
    // 0 through 4
    this->args16()[WHEEL_FRONT_PASSENGER].i = WHEEL_FRONT_PASSENGER_PRESSURE; // getWheel(WHEEL_FRONT_PASSENGER)->getPressure();
    this->args16()[WHEEL_REAR_PASSENGER].i = WHEEL_REAR_PASSENGER_PRESSURE;   // getWheel(WHEEL_REAR_PASSENGER)->getPressure();
    this->args16()[WHEEL_FRONT_DRIVER].i = WHEEL_FRONT_DRIVER_PRESSURE;       // getWheel(WHEEL_FRONT_DRIVER)->getPressure();
    this->args16()[WHEEL_REAR_DRIVER].i = WHEEL_REAR_DRIVER_PRESSURE;         // getWheel(WHEEL_REAR_DRIVER)->getPressure();
    this->args16()[_TANK_INDEX].i = TANK_PRESSURE;                            // getCompressor()->getTankPressure();
    this->args16()[5].i = bittset;

    // doesn't matter for this because it is generic broadcasted for everyone
    this->sender = 0;
    this->recipient = 0;
}

PresetPacket::PresetPacket(int profileIndex, float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE)
{
    this->cmd = PRESETREPORT;
    // 0 through 4
    this->args16()[WHEEL_FRONT_PASSENGER].i = WHEEL_FRONT_PASSENGER_PRESSURE; // getWheel(WHEEL_FRONT_PASSENGER)->getPressure();
    this->args16()[WHEEL_REAR_PASSENGER].i = WHEEL_REAR_PASSENGER_PRESSURE;   // getWheel(WHEEL_REAR_PASSENGER)->getPressure();
    this->args16()[WHEEL_FRONT_DRIVER].i = WHEEL_FRONT_DRIVER_PRESSURE;       // getWheel(WHEEL_FRONT_DRIVER)->getPressure();
    this->args16()[WHEEL_REAR_DRIVER].i = WHEEL_REAR_DRIVER_PRESSURE;         // getWheel(WHEEL_REAR_DRIVER)->getPressure();
    this->args16()[4].i = profileIndex;                                       // profile index

    // doesn't matter for this because it is generic broadcasted for everyone
    this->sender = 0;
    this->recipient = 0;
}

int PresetPacket::getProfile()
{
    return this->args16()[4].i;
}

IdlePacket::IdlePacket()
{
    // idle packet is the same as a blank BTOasPacket
}

AssignRecipientPacket::AssignRecipientPacket(int assignmentNumber)
{
    this->cmd = ASSIGNRECEPIENT;
    this->sender = 0;
    this->recipient = 0; // client should assume if they haven't been assigned yet then they are being assigned this
    this->args32()[0].i = assignmentNumber;
}

MessagePacket::MessagePacket(short recipient, std::string message)
{
    this->cmd = MESSAGE;
    this->sender = 0;
    this->recipient = recipient;
    memcpy(this->args, (uint8_t *)message.data(), message.length());
}

// Incoming packets
AirupPacket::AirupPacket()
{
    this->cmd = AIRUP;
}
AiroutPacket::AiroutPacket()
{
    this->cmd = AIROUT;
}
AirsmPacket::AirsmPacket(int relativeValue)
{
    this->cmd = AIRSM;
    this->args32()[0].i = relativeValue;
}
SaveToProfilePacket::SaveToProfilePacket(int profileIndex)
{
    this->cmd = SAVETOPROFILE;
    this->args32()[0].i = profileIndex;
}
SaveCurrentPressuresToProfilePacket::SaveCurrentPressuresToProfilePacket(int profileIndex)
{
    this->cmd = SAVECURRENTPRESSURESTOPROFILE;
    this->args32()[0].i = profileIndex;
}
ReadProfilePacket::ReadProfilePacket(int profileIndex)
{
    this->cmd = READPROFILE;
    this->args32()[0].i = profileIndex;
}
AirupQuickPacket::AirupQuickPacket(int profileIndex)
{
    this->cmd = AIRUPQUICK;
    this->args32()[0].i = profileIndex;
}
BaseProfilePacket::BaseProfilePacket(int profileIndex)
{
    this->cmd = BASEPROFILE;
    this->args32()[0].i = profileIndex;
}
SetAirheightPacket::SetAirheightPacket(int wheelIndex, int pressure)
{
    this->cmd = SETAIRHEIGHT;
    this->args32()[0].i = wheelIndex;
    this->args32()[1].i = pressure;
}
RiseOnStartPacket::RiseOnStartPacket(bool enable)
{
    this->cmd = RISEONSTART;
    this->args32()[0].i = enable;
}
FallOnShutdownPacket::FallOnShutdownPacket(bool enable)
{
    this->cmd = FALLONSHUTDOWN;
    this->args32()[0].i = enable;
}
RaiseOnPressureSetPacket::RaiseOnPressureSetPacket(bool enable)
{
    this->cmd = RAISEONPRESSURESET;
    this->args32()[0].i = enable;
}
MaintainPressurePacket::MaintainPressurePacket(bool enable)
{
    this->cmd = MAINTAINPRESSURE;
    this->args32()[0].i = enable;
}
RebootPacket::RebootPacket()
{
    this->cmd = REBOOT;
}
StartwebPacket::StartwebPacket()
{
    this->cmd = STARTWEB;
}

int AirsmPacket::getRelativeValue()
{
    return this->args32()[0].i;
}

int ProfilePacket::getProfileIndex()
{
    return this->args32()[0].i;
}

bool BooleanPacket::getBoolean()
{
    return this->args32()[0].i != 0;
}

int SetAirheightPacket::getWheelIndex()
{
    return this->args32()[0].i;
}
int SetAirheightPacket::getPressure()
{
    return this->args32()[1].i;
}