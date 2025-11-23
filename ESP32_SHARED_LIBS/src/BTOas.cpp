#include "BTOas.h"

uint8_t *BTOasPacket::tx()
{
    return (uint8_t *)&cmd;
}

BTOasValue8 *BTOasPacket::args8()
{
    return (BTOasValue8 *)this->args;
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
StatusPacket::StatusPacket(float WHEEL_FRONT_PASSENGER_PRESSURE, float WHEEL_REAR_PASSENGER_PRESSURE, float WHEEL_FRONT_DRIVER_PRESSURE, float WHEEL_REAR_DRIVER_PRESSURE, float TANK_PRESSURE, uint32_t bittset, uint8_t AIPercentage, uint8_t AIReadyBittset)
{
    this->cmd = STATUSREPORT;
    // 0 through 4
    this->args16()[WHEEL_FRONT_PASSENGER].i = WHEEL_FRONT_PASSENGER_PRESSURE; // getWheel(WHEEL_FRONT_PASSENGER)->getPressure();
    this->args16()[WHEEL_REAR_PASSENGER].i = WHEEL_REAR_PASSENGER_PRESSURE;   // getWheel(WHEEL_REAR_PASSENGER)->getPressure();
    this->args16()[WHEEL_FRONT_DRIVER].i = WHEEL_FRONT_DRIVER_PRESSURE;       // getWheel(WHEEL_FRONT_DRIVER)->getPressure();
    this->args16()[WHEEL_REAR_DRIVER].i = WHEEL_REAR_DRIVER_PRESSURE;         // getWheel(WHEEL_REAR_DRIVER)->getPressure();
    this->args16()[_TANK_INDEX].i = TANK_PRESSURE;                            // getCompressor()->getTankPressure();
    this->args8()[10].i = AIPercentage;
    this->args8()[11].i = AIReadyBittset;
    this->args32()[3].i = bittset;

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
DetectPressureSensorsPacket::DetectPressureSensorsPacket()
{
    this->cmd = DETECTPRESSURESENSORS;
}
CalibratePacket::CalibratePacket()
{
    this->cmd = CALIBRATE;
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
HeightSensorModePacket::HeightSensorModePacket(bool enable)
{
    this->cmd = HEIGHTSENSORMODE;
    this->args32()[0].i = enable;
}
SafetyModePacket::SafetyModePacket(bool enable)
{
    this->cmd = SAFETYMODE;
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
CompressorStatusPacket::CompressorStatusPacket(bool enable)
{
    this->cmd = COMPRESSORSTATUS;
    this->args32()[0].i = enable;
}
AIStatusPacket::AIStatusPacket(bool enable)
{
    this->cmd = AISTATUSENABLED;
    this->args32()[0].i = enable;
}
RebootPacket::RebootPacket()
{
    this->cmd = REBOOT;
}
TurnOffPacket::TurnOffPacket()
{
    this->cmd = TURNOFF;
}
ResetAIPacket::ResetAIPacket()
{
    this->cmd = RESETAIPKT;
}
StartwebPacket::StartwebPacket(String ssid, String password)
{
    this->cmd = STARTWEB;
    strcpy((char *)&this->args[0], ssid.c_str());
    strcpy((char *)&this->args[50], password.c_str());
}
String StartwebPacket::getSSID()
{
    return String((char *)&this->args[0]);
}
String StartwebPacket::getPassword()
{
    return String((char *)&this->args[50]);
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
ConfigValuesPacket::ConfigValuesPacket(bool setValues, uint8_t bagMaxPressure, uint32_t systemShutoffTimeM, uint8_t compressorOnPSI, uint8_t compressorOffPSI, uint16_t pressureSensorMax, uint16_t bagVolumePercentage)
{
    this->cmd = GETCONFIGVALUES;
    *this->_systemShutoffTimeM() = systemShutoffTimeM;
    *this->_bagMaxPressure() = bagMaxPressure;
    *this->_compressorOnPSI() = compressorOnPSI;
    *this->_compressorOffPSI() = compressorOffPSI;
    *this->_pressureSensorMax() = pressureSensorMax;
    *this->_bagVolumePercentage() = bagVolumePercentage;
    *this->_setValues() = setValues;
}

uint32_t *ConfigValuesPacket::_systemShutoffTimeM()
{
    return (uint32_t *)&(this->args32()[0].i);
}
uint16_t *ConfigValuesPacket::_pressureSensorMax()
{
    return (uint16_t *)&(this->args16()[2].i);
}
uint16_t *ConfigValuesPacket::_bagVolumePercentage()
{
    return (uint16_t *)&(this->args16()[3].i);
}
uint8_t *ConfigValuesPacket::_bagMaxPressure()
{
    return (uint8_t *)&(this->args8()[8 + 0].i);
}
uint8_t *ConfigValuesPacket::_compressorOnPSI()
{
    return (uint8_t *)&(this->args8()[8 + 1].i);
}
uint8_t *ConfigValuesPacket::_compressorOffPSI()
{
    return (uint8_t *)&(this->args8()[8 + 2].i);
}
bool *ConfigValuesPacket::_setValues()
{
    return (bool *)&(this->args8()[8 + 3].i);
}
AuthPacket::AuthPacket(uint32_t blePasskey, AuthResult authResult)
{
    this->cmd = AUTHPACKET;
    this->args32()[0].i = blePasskey;
    this->args32()[1].i = authResult;
}
uint32_t AuthPacket::getBlePasskey()
{
    return this->args32()[0].i;
}
AuthResult AuthPacket::getBleAuthResult()
{
    return (AuthResult)this->args32()[1].i;
}
void AuthPacket::setBleAuthResult(AuthResult ar)
{
    this->args32()[1].i = (uint32_t)ar;
}
BroadcastNamePacket::BroadcastNamePacket(String broadcastName)
{
    this->cmd = BROADCASTNAME;
    int len = broadcastName.length();
    if (len > 8)
        len = 8;
    strncpy((char *)&this->args[0], broadcastName.c_str(), len);
}
String BroadcastNamePacket::getBroadcastName()
{
    return String((char *)&this->args[0]);
}
BP32Packet::BP32Packet(BP32CMD bp32Cmd, bool value)
{
    this->cmd = BP32PKT;
    this->args16()[0].i = bp32Cmd;
    this->args16()[1].i = value;
}

UpdateStatusRequestPacket::UpdateStatusRequestPacket()
{
    this->cmd = UPDATESTATUSREQUEST;
    memset(this->args, 0, sizeof(this->args));
    this->_setStatus = false;
}
UpdateStatusRequestPacket::UpdateStatusRequestPacket(String status)
{
    this->cmd = UPDATESTATUSREQUEST;
    this->setStatus(status);
    this->_setStatus = false;
}
String UpdateStatusRequestPacket::getStatus()
{
    return String((char *)&this->args[0]);
}
void UpdateStatusRequestPacket::setStatus(String status)
{
    int len = status.length();
    if (len > sizeof(this->args))
        len = sizeof(this->args);
    strncpy((char *)&this->args[0], status.c_str(), len);
}