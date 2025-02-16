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
    Preferencable heightSensorMode;
    Preferencable bagMaxPressure;
    Preferencable blePasskey;
    Preferencable systemShutoffTimeM;
    Preferencable compressorOnPSI;
    Preferencable compressorOffPSI;
    Preferencable pressureSensorMax;
    Profile profile[MAX_PROFILE_COUNT];
};

extern SaveData _SaveData;
extern byte currentProfile[4];
extern bool sendProfileBT;

void beginSaveData();
void readProfile(byte profileIndex);
void writeProfile(byte profileIndex);
void savePressuresToProfile(byte profileIndex, float _WHEEL_FRONT_PASSENGER, float _WHEEL_REAR_PASSENGER, float _WHEEL_FRONT_DRIVER, float _WHEEL_REAR_DRIVER);

headerDefineSaveFunc(riseOnStart, bool);
headerDefineSaveFunc(maintainPressure, bool);
headerDefineSaveFunc(airOutOnShutoff, bool);
headerDefineSaveFunc(heightSensorMode, bool);
headerDefineSaveFunc(baseProfile, byte);
headerDefineSaveFunc(raiseOnPressure, bool);
headerDefineSaveFunc(internalReboot, bool);

// values moved from the user defines file
headerDefineSaveFunc(bagMaxPressure, uint8_t);
headerDefineSaveFunc(blePasskey, uint32_t);         // 6 digits base 10
headerDefineSaveFunc(systemShutoffTimeM, uint32_t); // may have to change
headerDefineSaveFunc(compressorOnPSI, uint8_t);
headerDefineSaveFunc(compressorOffPSI, uint8_t);
headerDefineSaveFunc(pressureSensorMax, uint16_t);

#endif