#ifndef manifoldSaveData_h
#define manifoldSaveData_h

#include <preferencable.h>

#include <Arduino.h>

#include <user_defines.h>

class Profile
{
public:
    Preferencable pressure[4]; // byte
};

class SaveData
{
public:
    Preferencable riseOnStart;     // byte
    Preferencable baseProfile;     // byte
    Preferencable raiseOnPressure; // byte
    Preferencable internalReboot;  // byte
    Preferencable maintainPressure;
    Preferencable airOutOnShutoff;
    Profile profile[MAX_PROFILE_COUNT];
};

extern SaveData _SaveData;
extern byte currentProfile[4];
extern bool sendProfileBT;

void beginSaveData();
void readProfile(byte profileIndex);
void writeProfile(byte profileIndex);
void savePressuresToProfile(byte profileIndex, float _WHEEL_FRONT_PASSENGER, float _WHEEL_REAR_PASSENGER, float _WHEEL_FRONT_DRIVER, float _WHEEL_REAR_DRIVER);
bool getRiseOnStart();
void setRiseOnStart(bool value);
bool getMaintainPressure();
void setMaintainPressure(bool value);
bool getAirOutOnShutoff();
void setAirOutOnShutoff(bool value);
byte getBaseProfile();
void setBaseProfile(byte value);
bool getRaiseOnPressureSet();
void setRaiseOnPressureSet(bool value);
bool getReboot();
void setReboot(bool value);

#endif