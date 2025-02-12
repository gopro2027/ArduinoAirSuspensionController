#ifndef manifoldSaveData_h
#define manifoldSaveData_h

#include <preferencable.h>

#include <Arduino.h>

#include <user_defines.h>

#define createSaveFuncInt(VARNAME, _TYPE) \
    _TYPE get##VARNAME()                  \
    {                                     \
        return _SaveData.VARNAME.get().i; \
    }                                     \
    void set##VARNAME(_TYPE value)        \
    {                                     \
        if (get##VARNAME() != value)      \
        {                                 \
            _SaveData.VARNAME.set(value); \
        }                                 \
    }

#define headerDefineSaveFunc(VARNAME, _TYPE) \
    _TYPE get##VARNAME();                    \
    void set##VARNAME(_TYPE value);

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
// bool getRiseOnStart();
// void setRiseOnStart(bool value);
// bool getMaintainPressure();
// void setMaintainPressure(bool value);
// bool getAirOutOnShutoff();
// void setAirOutOnShutoff(bool value);
// byte getBaseProfile();
// void setBaseProfile(byte value);
// bool getRaiseOnPressureSet();
// void setRaiseOnPressureSet(bool value);
// bool getReboot();
// void setReboot(bool value);

headerDefineSaveFunc(riseOnStart, bool);
headerDefineSaveFunc(maintainPressure, bool);
headerDefineSaveFunc(airOutOnShutoff, bool);
headerDefineSaveFunc(baseProfile, byte);
headerDefineSaveFunc(raiseOnPressure, bool);
headerDefineSaveFunc(internalReboot, bool);

// values moved from the user defines file
headerDefineSaveFunc(bagMaxPressure, uint8_t);
headerDefineSaveFunc(blePasskey, uint32_t);         // 6 digits base 10
headerDefineSaveFunc(systemShutoffTimeM, uint64_t); // may have to change
headerDefineSaveFunc(compressorOnPSI, uint8_t);
headerDefineSaveFunc(compressorOffPSI, uint8_t);
headerDefineSaveFunc(pressureSensorMax, uint8_t);

#endif