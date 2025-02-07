// This definition includes all the functions to create packets

// NOTICE: If this file is renamed/moved/or other imports added... please be mindfull of how it will affect the Wireless_Controller project

#ifndef BTOas_h
#define BTOas_h
#include <Arduino.h>

#include "user_defines.h"

enum BTOasIdentifier
{
    IDLE = 0,
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
    STARTWEB = 14,
    ASSIGNRECEPIENT = 15,
    MESSAGE = 16,
    SAVECURRENTPRESSURESTOPROFILE = 17,
    PRESETREPORT = 1,
};

enum StatusPacketBittset
{
    COMPRESSOR_FROZEN,
    COMPRESSOR_STATUS_ON,
    ACC_STATUS_ON,
    TIMER_STATUS_EXPIRED,
    CLOCK,
};

union BTOasValue32
{
    uint32_t i;
    float f;
};

union BTOasValue16
{
    uint16_t i;
};

// NOTE: Default max for BLE is 23 bytes. We can do some compression if we need in the future but for now this uses 20 total (5 x 32 bits)
struct BTOasPacket
{
    uint16_t cmd; // BTOasIdentifier
    uint8_t sender;
    uint8_t recipient;
    uint8_t args[16];

    uint8_t *tx();
    BTOasValue16 *args16();
    BTOasValue32 *args32();

    void dump();

    BTOasPacket();
};

#define BTOAS_PACKET_SIZE sizeof(BTOasPacket)

void runReceivedPacket(BTOasPacket *packet);

// Outgoing packets
struct StatusPacket : BTOasPacket
{
    StatusPacket(float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE, float TANK_PRESSURE, uint16_t bittset);
};

struct PresetPacket : BTOasPacket
{
    PresetPacket(int profileIndex, float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE);
};

struct AssignRecipientPacket : BTOasPacket
{
    AssignRecipientPacket(int assignmentNumber);
};

struct IdlePacket : BTOasPacket
{
    IdlePacket();
};

struct MessagePacket : BTOasPacket
{
    MessagePacket(short recipient, std::string message);
};

// Incoming packets
struct AirupPacket : BTOasPacket
{
    AirupPacket();
};
struct AiroutPacket : BTOasPacket
{
    AiroutPacket();
};
struct AirsmPacket : BTOasPacket
{
    AirsmPacket(int relativeValue);
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
    SaveToProfilePacket(int profileIndex);
};
struct SaveCurrentPressuresToProfilePacket : ProfilePacket
{
    SaveCurrentPressuresToProfilePacket(int profileIndex);
};
struct ReadProfilePacket : ProfilePacket
{
    ReadProfilePacket(int profileIndex);
};
struct AirupQuickPacket : ProfilePacket
{
    AirupQuickPacket(int profileIndex);
};
struct BaseProfilePacket : ProfilePacket
{
    BaseProfilePacket(int profileIndex);
};
struct SetAirheightPacket : BTOasPacket
{
    SetAirheightPacket(int wheelIndex, int pressure);
    int getWheelIndex();
    int getPressure();
};
struct RiseOnStartPacket : BooleanPacket
{
    RiseOnStartPacket(bool enable);
};
struct RaiseOnPressureSetPacket : BooleanPacket
{
    RaiseOnPressureSetPacket(bool enable);
};
struct RebootPacket : BTOasPacket
{
    RebootPacket();
};
struct StartwebPacket : BTOasPacket
{
    StartwebPacket();
};

#endif