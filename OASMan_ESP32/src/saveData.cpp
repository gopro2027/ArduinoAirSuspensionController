#include "saveData.h"

EEPROM_DATA_ EEPROM_DATA;
byte currentProfile[4];
bool sendProfileBT = false;

void saveEEPROM() {
  EEPROM.put(0, EEPROM_DATA);
  EEPROM.commit();
}
void beginEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, EEPROM_DATA);
}

void readProfile(byte profileIndex) {
  currentProfile[WHEEL_FRONT_PASSENGER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER];
  currentProfile[WHEEL_REAR_PASSENGER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER];
  currentProfile[WHEEL_FRONT_DRIVER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER];
  currentProfile[WHEEL_REAR_DRIVER] = EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER];
  sendProfileBT = true;
}

void writeProfile(byte profileIndex) {

  if (currentProfile[WHEEL_FRONT_PASSENGER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER] ||
      currentProfile[WHEEL_REAR_PASSENGER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER] ||
      currentProfile[WHEEL_FRONT_DRIVER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER] ||
      currentProfile[WHEEL_REAR_DRIVER] != EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER]) {

    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER] = currentProfile[WHEEL_FRONT_PASSENGER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER] = currentProfile[WHEEL_REAR_PASSENGER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER] = currentProfile[WHEEL_FRONT_DRIVER];
    EEPROM_DATA.profile[profileIndex].pressure[WHEEL_REAR_DRIVER] = currentProfile[WHEEL_REAR_DRIVER];
    saveEEPROM();
  }
}

bool getRiseOnStart() {
    return EEPROM_DATA.riseOnStart;
}
void setRiseOnStart(bool value) {
  if (getRiseOnStart() != value) {
    EEPROM_DATA.riseOnStart = value;
    saveEEPROM();
  }
}

byte getBaseProfile() {
    return EEPROM_DATA.baseProfile;
}
void setBaseProfile(byte value) {
  if (getBaseProfile() != value) {
    EEPROM_DATA.baseProfile = value;
    saveEEPROM();
  }
}

bool getRaiseOnPressureSet() {
    return EEPROM_DATA.raiseOnPressure;
}
void setRaiseOnPressureSet(bool value) {
  if (getRaiseOnPressureSet() != value) {
    EEPROM_DATA.raiseOnPressure = value;
    saveEEPROM();
  }
}