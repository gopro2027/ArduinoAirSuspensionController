#ifndef saveData_h
#define saveData_h

#include <Arduino.h>
#include <EEPROM.h>

#include "user_defines.h"

struct Profile
{
    byte pressure[4];
};

struct EEPROM_DATA_
{
    byte riseOnStart;
    byte baseProfile;
    byte raiseOnPressure;
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

#endif