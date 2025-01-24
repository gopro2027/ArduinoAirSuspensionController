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

// Outgoing packets
StatusPacket::StatusPacket()
{
    memset(this->args, 0, sizeof(this->args));
    this->cmd = STATUSREPORT;
    // 0 through 4
    this->args16()[WHEEL_FRONT_PASSENGER].i = getWheel(WHEEL_FRONT_PASSENGER)->getPressure();
    this->args16()[WHEEL_REAR_PASSENGER].i = getWheel(WHEEL_REAR_PASSENGER)->getPressure();
    this->args16()[WHEEL_FRONT_DRIVER].i = getWheel(WHEEL_FRONT_DRIVER)->getPressure();
    this->args16()[WHEEL_REAR_DRIVER].i = getWheel(WHEEL_REAR_DRIVER)->getPressure();
    this->args16()[4].i = getCompressor()->getTankPressure();

    // doesn't matter for this because it is generic broadcasted for everyone
    this->sender = 0;
    this->recipient = 0;
}

IdlePacket::IdlePacket()
{
    memset(this->args, 0, sizeof(this->args));
    this->cmd = IDLE;
    this->sender = 0;
    this->recipient = 0;
}

AssignRecipientPacket::AssignRecipientPacket(int assignmentNumber)
{
    memset(this->args, 0, sizeof(this->args));
    this->cmd = ASSIGNRECEPIENT;
    this->sender = 0;
    this->recipient = 0; // client should assume if they haven't been assigned yet then they are being assigned this
    this->args32()[0].i = assignmentNumber;
}

MessagePacket::MessagePacket(short recipient, std::string message)
{
    memset(this->args, 0, sizeof(this->args));
    this->cmd = MESSAGE;
    this->sender = 0;
    this->recipient = recipient;
    memcpy(this->args, (uint8_t *)message.data(), message.length());
}

// Incoming packets

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