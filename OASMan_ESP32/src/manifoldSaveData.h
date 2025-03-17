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
    Preferencable learnPressureSensors;

    Preferencable pressureInputFrontPassenger;
    Preferencable pressureInputRearPassenger;
    Preferencable pressureInputFrontDriver;
    Preferencable pressureInputRearDriver;
    Preferencable pressureInputTank;

    Preferencable maintainPressure;
    Preferencable airOutOnShutoff;
    Preferencable heightSensorMode;
    Preferencable bagMaxPressure;
    Preferencable blePasskey;
    Preferencable systemShutoffTimeM;
    Preferencable compressorOnPSI;
    Preferencable compressorOffPSI;
    Preferencable pressureSensorMax;
    Preferencable bagVolumePercentage;
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
headerDefineSaveFunc(learnPressureSensors, bool);

// pressure sensor values
headerDefineSaveFunc(pressureInputFrontPassenger, byte);
headerDefineSaveFunc(pressureInputRearPassenger, byte);
headerDefineSaveFunc(pressureInputFrontDriver, byte);
headerDefineSaveFunc(pressureInputRearDriver, byte);
headerDefineSaveFunc(pressureInputTank, byte);

// values moved from the user defines file
headerDefineSaveFunc(bagMaxPressure, uint8_t);
headerDefineSaveFunc(blePasskey, uint32_t);         // 6 digits base 10
headerDefineSaveFunc(systemShutoffTimeM, uint32_t); // may have to change
headerDefineSaveFunc(compressorOnPSI, uint8_t);
headerDefineSaveFunc(compressorOffPSI, uint8_t);
headerDefineSaveFunc(pressureSensorMax, uint16_t);
headerDefineSaveFunc(bagVolumePercentage, uint16_t);

float getHeightSensorMax();

#endif