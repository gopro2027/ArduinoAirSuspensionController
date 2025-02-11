#include "manifoldSaveData.h"

SaveData _SaveData;
byte currentProfile[4];
bool sendProfileBT = false;

void beginSaveData()
{
    _SaveData.riseOnStart.load("riseOnStart", false);
    _SaveData.maintainPressure.load("maintainPressure", false);
    _SaveData.airOutOnShutoff.load("airOutOnShutoff", false);
    _SaveData.baseProfile.load("baseProfile", 0);
    _SaveData.raiseOnPressure.load("raiseOnPressure", false);
    _SaveData.internalReboot.load("internalReboot", false);
    for (int i = 0; i < MAX_PROFILE_COUNT; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            // first create a custom name for it. This would probably be better off done as different namespaces or something but idc
            snprintf(_SaveData.profile[i].pressure[j].name, sizeof(_SaveData.profile[i].pressure[j].name), "profile%i|%i", i, j);
            _SaveData.profile[i].pressure[j].load(_SaveData.profile[i].pressure[j].name, 50);
        }
    }
}

void readProfile(byte profileIndex)
{
    currentProfile[WHEEL_FRONT_PASSENGER] = _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER].get().i;
    currentProfile[WHEEL_REAR_PASSENGER] = _SaveData.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER].get().i;
    currentProfile[WHEEL_FRONT_DRIVER] = _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER].get().i;
    currentProfile[WHEEL_REAR_DRIVER] = _SaveData.profile[profileIndex].pressure[WHEEL_REAR_DRIVER].get().i;
    sendProfileBT = true;
}

void writeProfile(byte profileIndex)
{

    if (currentProfile[WHEEL_FRONT_PASSENGER] != _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER].get().i ||
        currentProfile[WHEEL_REAR_PASSENGER] != _SaveData.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER].get().i ||
        currentProfile[WHEEL_FRONT_DRIVER] != _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER].get().i ||
        currentProfile[WHEEL_REAR_DRIVER] != _SaveData.profile[profileIndex].pressure[WHEEL_REAR_DRIVER].get().i)
    {

        _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER].set(currentProfile[WHEEL_FRONT_PASSENGER]);
        _SaveData.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER].set(currentProfile[WHEEL_REAR_PASSENGER]);
        _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER].set(currentProfile[WHEEL_FRONT_DRIVER]);
        _SaveData.profile[profileIndex].pressure[WHEEL_REAR_DRIVER].set(currentProfile[WHEEL_REAR_DRIVER]);
    }
}

void savePressuresToProfile(byte profileIndex, float _WHEEL_FRONT_PASSENGER, float _WHEEL_REAR_PASSENGER, float _WHEEL_FRONT_DRIVER, float _WHEEL_REAR_DRIVER)
{
    _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER].set((int)_WHEEL_FRONT_PASSENGER);
    _SaveData.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER].set((int)_WHEEL_REAR_PASSENGER);
    _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER].set((int)_WHEEL_FRONT_DRIVER);
    _SaveData.profile[profileIndex].pressure[WHEEL_REAR_DRIVER].set((int)_WHEEL_REAR_DRIVER);
}

bool getRiseOnStart()
{
    return _SaveData.riseOnStart.get().i;
}
void setRiseOnStart(bool value)
{
    if (getRiseOnStart() != value)
    {
        _SaveData.riseOnStart.set(value);
    }
}

bool getMaintainPressure()
{
    return _SaveData.maintainPressure.get().i;
}
void setMaintainPressure(bool value)
{
    if (getMaintainPressure() != value)
    {
        _SaveData.maintainPressure.set(value);
    }
}

bool getAirOutOnShutoff()
{
    return _SaveData.airOutOnShutoff.get().i;
}
void setAirOutOnShutoff(bool value)
{
    if (getAirOutOnShutoff() != value)
    {
        _SaveData.airOutOnShutoff.set(value);
    }
}

byte getBaseProfile()
{
    return _SaveData.baseProfile.get().i;
}
void setBaseProfile(byte value)
{
    if (getBaseProfile() != value)
    {
        _SaveData.baseProfile.set(value);
    }
}

bool getRaiseOnPressureSet()
{
    return _SaveData.raiseOnPressure.get().i;
}
void setRaiseOnPressureSet(bool value)
{
    if (getRaiseOnPressureSet() != value)
    {
        _SaveData.raiseOnPressure.set(value);
    }
}

bool getReboot()
{
    return _SaveData.internalReboot.get().i;
}
void setReboot(bool value)
{
    if (getReboot() != value)
    {
        _SaveData.internalReboot.set(value);
    }
}
