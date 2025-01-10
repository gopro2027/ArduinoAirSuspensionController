#include "saveData.h"

// Tutorial with preferences code https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

#define SAVEDATA_NAMESPACE "savedata"

Preferences preferences;

void openNamespace(char *ns, bool ro)
{
    preferences.begin(ns, ro);
}

void endNamespace()
{
    preferences.end();
}

void Preferencable::load(char *name, int defaultValue)
{
    strncpy(this->name, name, sizeof(this->name));
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putInt(this->name, defaultValue);
        this->value.i = defaultValue;
    }
    else
    {
        this->value.i = preferences.getInt(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::set(int val)
{
    if (this->value.i != val)
    {
        this->value.i = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putInt(this->name, val);
        endNamespace();
    }
}

void Preferencable::loadFloat(char *name, float defaultValue)
{
    strncpy(this->name, name, sizeof(this->name));
    openNamespace(SAVEDATA_NAMESPACE, true);
    if (preferences.isKey(name) == false)
    {
        endNamespace();
        openNamespace(SAVEDATA_NAMESPACE, false); // reopen as read write
        preferences.putFloat(this->name, defaultValue);
        this->value.f = defaultValue;
    }
    else
    {
        this->value.f = preferences.getFloat(name, defaultValue);
    }
    endNamespace();
}

void Preferencable::setFloat(float val)
{
    if (this->value.f != val)
    {
        this->value.f = val;
        openNamespace(SAVEDATA_NAMESPACE, false);
        preferences.putFloat(this->name, val);
        endNamespace();
    }
}

SaveData _SaveData;
byte currentProfile[4];
bool sendProfileBT = false;

void beginSaveData()
{
    _SaveData.riseOnStart.load("riseOnStart", false);
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
