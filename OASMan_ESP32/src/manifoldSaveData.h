#ifndef manifoldSaveData_h
#define manifoldSaveData_h

#include <preferencable.h>

#include <Arduino.h>

#include <user_defines.h>

#include "pressureMath.h"

class Profile
{
public:
    Preferencable pressure[4]; // byte
};

class AIModelPreference
{
public:
    Preferencable weights[3];   // doubles
    Preferencable isReadyToUse; // bool
    AIModel model;
    void loadModel()
    {
        model.loadWeights(weights[0].get().d, weights[1].get().d, weights[2].get().d);
        model.print_weights();
    }
    void saveWeights()
    {
        weights[0].setDouble(model.w1);
        weights[1].setDouble(model.w2);
        weights[2].setDouble(model.b);
    }
    void setReady(bool ready)
    {
        isReadyToUse.set(ready);
    }
    void deletePreferences()
    {
        isReadyToUse.deletePreference();
        weights[0].deletePreference();
        weights[1].deletePreference();
        weights[2].deletePreference();
    }
};

class SaveData
{
public:
    Preferencable riseOnStart;     // byte
    Preferencable baseProfile;     // byte
    Preferencable raiseOnPressure; // byte
    Preferencable internalReboot;  // byte
    Preferencable learnPressureSensors;
    Preferencable safetyMode;
    Preferencable aiEnabled;
    Preferencable updateMode;
    Preferencable wifiSSID;
    Preferencable wifiPassword;
    Preferencable updateResult;

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
    Preferencable bleName;
    Preferencable systemShutoffTimeM;
    Preferencable compressorOnPSI;
    Preferencable compressorOffPSI;
    Preferencable pressureSensorMax;
    Preferencable bagVolumePercentage;
    Profile profile[MAX_PROFILE_COUNT];
    AIModelPreference aiModels[4];
};

struct PressureLearnSaveStruct
{
    uint8_t start_pressure;
    uint8_t goal_pressure;
    uint16_t tank_pressure;
    uint32_t timeMS;
    void print()
    {
        // Serial.printf("{0x%X, 0x%X, 0x%X, 0x%X}", start_pressure, goal_pressure, tank_pressure, timeMS);
        Serial.print("{");
        Serial.print((int)start_pressure);
        Serial.print(", ");
        Serial.print((int)goal_pressure);
        Serial.print(", ");
        Serial.print(tank_pressure);
        Serial.print(", ");
        Serial.print(timeMS);
        Serial.print("}");
    }
};

extern SaveData _SaveData;
extern byte currentProfile[4];
extern bool sendProfileBT;

void beginSaveData();
void readProfile(byte profileIndex);
void writeProfile(byte profileIndex);
void savePressuresToProfile(byte profileIndex, float _WHEEL_FRONT_PASSENGER, float _WHEEL_REAR_PASSENGER, float _WHEEL_FRONT_DRIVER, float _WHEEL_REAR_DRIVER);

PressureLearnSaveStruct *getLearnData(SOLENOID_AI_INDEX aiIndex);
int getLearnDataLength(SOLENOID_AI_INDEX aiIndex);

void clearPressureData();

void appendPressureDataToFile(SOLENOID_AI_INDEX aiIndex, uint8_t start_pressure, uint8_t goal_pressure, uint16_t tank_pressure, uint32_t timeMS);

AIModelPreference *getAIModel(SOLENOID_AI_INDEX aiIndex);

headerDefineSaveFunc(riseOnStart, bool);
headerDefineSaveFunc(maintainPressure, bool);
headerDefineSaveFunc(airOutOnShutoff, bool);
headerDefineSaveFunc(heightSensorMode, bool);
headerDefineSaveFunc(baseProfile, byte);
headerDefineSaveFunc(raiseOnPressure, bool);
headerDefineSaveFunc(internalReboot, bool);
headerDefineSaveFunc(learnPressureSensors, bool);
headerDefineSaveFunc(safetyMode, bool);
headerDefineSaveFunc(aiEnabled, bool);
headerDefineSaveFunc(updateMode, bool);
headerDefineSaveFunc(wifiSSID, String);
headerDefineSaveFunc(wifiPassword, String);
headerDefineSaveFunc(updateResult, byte);

// pressure sensor values
headerDefineSaveFunc(pressureInputFrontPassenger, byte);
headerDefineSaveFunc(pressureInputRearPassenger, byte);
headerDefineSaveFunc(pressureInputFrontDriver, byte);
headerDefineSaveFunc(pressureInputRearDriver, byte);
headerDefineSaveFunc(pressureInputTank, byte);

// values moved from the user defines file
headerDefineSaveFunc(bagMaxPressure, uint8_t);
headerDefineSaveFunc(blePasskey, uint32_t); // 6 digits base 10
headerDefineSaveFunc(bleName, String);
headerDefineSaveFunc(systemShutoffTimeM, uint32_t); // may have to change
headerDefineSaveFunc(compressorOnPSI, uint8_t);
headerDefineSaveFunc(compressorOffPSI, uint8_t);
headerDefineSaveFunc(pressureSensorMax, uint16_t);
headerDefineSaveFunc(bagVolumePercentage, uint16_t);

float getHeightSensorMax();

#endif