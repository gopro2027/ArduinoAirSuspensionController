#ifndef saveData_h
#define saveData_h

#include <Arduino.h>
#include <Preferences.h>

#include "user_defines.h"

union PreferencableValue
{
    int i;
    float f;
};

class Preferencable
{

public:
    char name[15]; // 15 is max len. Note for future devs: I didn't add any code to make sure it is 0 terminated so be careful how you choose a name i guess
    PreferencableValue value;
    void load(char *name, int defaultValue);
    void set(int val);
    void loadFloat(char *name, float defaultValue);
    void setFloat(float val);
    PreferencableValue get()
    {
        return value;
    }
};

class Profile
{
public:
    Preferencable pressure[4]; // byte
};

class Calibration
{
public:
    Preferencable hasCalibrated;             // bool
    Preferencable voltageDividerCalibration; // float voltage divider read value for 0psi
    Preferencable adcCalibration;            // float adc read value for 0psi
};

class SaveData
{
public:
    Preferencable riseOnStart;     // byte
    Preferencable baseProfile;     // byte
    Preferencable raiseOnPressure; // byte
    Preferencable internalReboot;  // byte
    Calibration calibration;
    Profile profile[MAX_PROFILE_COUNT];
};

extern SaveData _SaveData;
extern byte currentProfile[4];
extern bool sendProfileBT;

void beginSaveData();
void readProfile(byte profileIndex);
void writeProfile(byte profileIndex);
bool getRiseOnStart();
void setRiseOnStart(bool value);
byte getBaseProfile();
void setBaseProfile(byte value);
bool getRaiseOnPressureSet();
void setRaiseOnPressureSet(bool value);
bool getReboot();
void setReboot(bool value);
Calibration *getCalibration();

#endif