// This definition includes all the functions to create packets

// NOTICE: If this file is renamed/moved/or other imports added... please be mindfull of how it will affect the Wireless_Controller project

#ifndef BTOas_h
#define BTOas_h
#include <Arduino.h>
#include <string>

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
    PRESETREPORT = 18,
    MAINTAINPRESSURE = 19,
    FALLONSHUTDOWN = 20,
    GETCONFIGVALUES = 21,
    AUTHPACKET = 22,
    HEIGHTSENSORMODE = 23,
    COMPRESSORSTATUS = 24,
    TURNOFF = 25,
    SAFETYMODE = 26,
    DETECTPRESSURESENSORS = 27,
    AISTATUSENABLED = 28
};

enum StatusPacketBittset
{
    COMPRESSOR_FROZEN,
    COMPRESSOR_STATUS_ON,
    ACC_STATUS_ON,
    TIMER_STATUS_EXPIRED, // not really used
    CLOCK,                // not really used
    MAINTAIN_PRESSURE,
    RISE_ON_START,
    AIR_OUT_ON_SHUTOFF,
    HEIGHT_SENSOR_MODE,
    SAFETY_MODE,
    AI_STATUS_ENABLED
};

enum AuthResult
{
    AUTHRESULT_WAITING,
    AUTHRESULT_SUCCESS,
    AUTHRESULT_FAIL,
    AUTHRESULT_UPDATEKEY
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

union BTOasValue8
{
    uint8_t i;
};

// NOTE: Default max for BLE is 23 bytes. We can do some compression if we need in the future but for now this uses 20 total (5 x 32 bits)
struct BTOasPacket
{
    uint16_t cmd; // BTOasIdentifier
    uint8_t sender;
    uint8_t recipient;
    uint8_t args[16];

    uint8_t *tx();
    BTOasValue8 *args8();
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
    StatusPacket(float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE, float TANK_PRESSURE, uint32_t bittset);
};

struct PresetPacket : BTOasPacket
{
    PresetPacket(int profileIndex, float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE);
    int getProfile();
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
struct DetectPressureSensorsPacket : BTOasPacket
{
    DetectPressureSensorsPacket();
};
struct CalibratePacket : BTOasPacket
{
    CalibratePacket();
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
struct FallOnShutdownPacket : BooleanPacket
{
    FallOnShutdownPacket(bool enable);
};
struct HeightSensorModePacket : BooleanPacket
{
    HeightSensorModePacket(bool enable);
};
struct SafetyModePacket : BooleanPacket
{
    SafetyModePacket(bool enable);
};
struct RaiseOnPressureSetPacket : BooleanPacket
{
    RaiseOnPressureSetPacket(bool enable);
};
struct MaintainPressurePacket : BooleanPacket
{
    MaintainPressurePacket(bool enable);
};
struct CompressorStatusPacket : BooleanPacket
{
    CompressorStatusPacket(bool enable);
};
struct AIStatusPacket : BooleanPacket
{
    AIStatusPacket(bool enable);
};
struct RebootPacket : BTOasPacket
{
    RebootPacket();
};
struct TurnOffPacket : BTOasPacket
{
    TurnOffPacket();
};
struct StartwebPacket : BTOasPacket
{
    StartwebPacket();
};
struct ConfigValuesPacket : BTOasPacket
{
    ConfigValuesPacket(bool setValues, uint8_t bagMaxPressure, uint32_t systemShutoffTimeM, uint8_t compressorOnPSI, uint8_t compressorOffPSI, uint16_t pressureSensorMax, uint16_t bagVolumePercentage);
    bool *_setValues();
    uint8_t *_bagMaxPressure();
    uint32_t *_systemShutoffTimeM();
    uint8_t *_compressorOnPSI();
    uint8_t *_compressorOffPSI();
    uint16_t *_pressureSensorMax();
    uint16_t *_bagVolumePercentage();
};
struct AuthPacket : BTOasPacket
{
    AuthPacket(uint32_t blePasskey, AuthResult authResult);
    uint32_t getBlePasskey();
    AuthResult getBleAuthResult();
    void setBleAuthResult(AuthResult ar);
};

#endif