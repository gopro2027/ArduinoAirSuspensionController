// This definition includes all the functions to create packets

#ifndef BTOas_h
#define BTOas_h
#include <Arduino.h>

#include "user_defines.h"
#include "airSuspensionUtil.h"
extern bool startOTAServiceRequest;

enum BTOasIdentifier
{
    STATUSREPORT = 1,
    AIRUP = 2,
    AIROUT = 3,
    AIRSM = 4,
    SAVETOPROFILE = 5,
    READPROFILE = 6,
    AIRUPQUICK = 7,
    BASEPROFILE = 8,
    SETAIRHEIGHT = 9,
    RISEONSTART = 10,
    RAISEONPRESSURESET = 11,
    REBOOT = 12,
    CALIBRATE = 13,
    STARTWEB = 14
};

union BTOasValue
{
    uint32_t i;
    float f;
};

// NOTE: Default max for BLE is 23 bytes. We can do some compression if we need in the future but for now this uses 20 total (5 x 32 bits)
struct BTOasPacket
{
    BTOasIdentifier cmd;
    BTOasValue args[4];
    uint8_t *tx();
};

#define BTOAS_PACKET_SIZE sizeof(BTOasPacket)

void runReceivedPacket(BTOasPacket *packet);

// Outgoing packets
struct StatusPacket : BTOasPacket
{
    StatusPacket();
};

// Incoming packets
struct AirupPacket : BTOasPacket
{
};
struct AiroutPacket : BTOasPacket
{
};
struct AirsmPacket : BTOasPacket
{
    int getRelativeValue();
};
struct ProfilePacket : BTOasPacket
{
    int getProfileIndex();
};
struct BooleanPacket : BTOasPacket
{
    bool getBoolean();
};
struct SaveToProfilePacket : ProfilePacket
{
};
struct ReadProfilePacket : ProfilePacket
{
};
struct AirupQuickPacket : ProfilePacket
{
};
struct BaseProfilePacket : ProfilePacket
{
};
struct SetAirheightPacket : BTOasPacket
{
    int getWheelIndex();
    int getPressure();
};
struct RiseOnStartPacket : BooleanPacket
{
};
struct RaiseOnPressureSetPacket : BooleanPacket
{
};
struct RebootPacket : BTOasPacket
{
};
struct StartwebPacket : BTOasPacket
{
};

#endif