#ifndef saveData_h
#define saveData_h

#include <Arduino.h>
#include <EEPROM.h>

#include "user_defines.h"

struct Profile
{
    byte pressure[4];
};

struct Calibration
{
    bool hasCalibrated;
    float voltageDividerCalibration; // voltage divider read value for 0psi
    float adcCalibration;            // adc read value for 0psi
};

struct EEPROM_DATA_
{
    byte riseOnStart;
    byte baseProfile;
    byte raiseOnPressure;
    byte internalReboot;
    Calibration calibration;
    byte padding[80]; // decrement as neccessary to maintain EEPROM_DATA_ when adding info
    Profile profile[MAX_PROFILE_COUNT];
};
#define EEPROM_SIZE sizeof(EEPROM_DATA_)

extern EEPROM_DATA_ EEPROM_DATA;
extern byte currentProfile[4];
extern bool sendProfileBT;

void saveEEPROM();
void saveEEPROMLoop();
void beginEEPROM();
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
void setCalibration();

#endif