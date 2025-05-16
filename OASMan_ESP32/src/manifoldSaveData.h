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

// class AIModelPreference {
//     public:
//     Preferencable weights[6];//doubles
//     Preferencable count;//int
//     AIModel model;
//     void loadModel() {
//         model.loadWeights(weights[0].get().d,weights[1].get().d,weights[2].get().d,weights[3].get().d,weights[4].get().d,weights[5].get().d);
//         model.print_weights();
//         Serial.print("Model count: ");
//         Serial.println(count.get().i);
//     }
//     void saveWeights() {
//         weights[0].setDouble(model.w1);
//         weights[1].setDouble(model.w2);
//         weights[2].setDouble(model.w3);
//         weights[3].setDouble(model.w4);
//         weights[4].setDouble(model.w5);
//         weights[5].setDouble(model.b);
//     }
// };

class SaveData
{
public:
    Preferencable riseOnStart;     // byte
    Preferencable baseProfile;     // byte
    Preferencable raiseOnPressure; // byte
    Preferencable internalReboot;  // byte
    Preferencable learnPressureSensors;
    Preferencable safetyMode;

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
    // AIModelPreference upModel;
    // AIModelPreference downModel;
};

struct PressureLearnSaveStruct {
    uint8_t start_pressure;
    uint8_t goal_pressure;
    uint16_t tank_pressure;
    uint32_t timeMS;
    void print() {
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

// void trainAiModel(bool up, double start_pressure, double end_pressure, double tank_pressure, double actual_time);
// double getAiPredictionTime(bool up, double start_pressure, double end_pressure, double tank_pressure);
// uint64_t getAiCount(bool up);

PressureLearnSaveStruct *getUpData();
PressureLearnSaveStruct *getDownData();
int getUpDataLength();
int getDownDataLength();

void appendPressureDataToFile(bool up,uint8_t start_pressure, uint8_t goal_pressure, uint16_t tank_pressure, uint32_t timeMS);

headerDefineSaveFunc(riseOnStart, bool);
headerDefineSaveFunc(maintainPressure, bool);
headerDefineSaveFunc(airOutOnShutoff, bool);
headerDefineSaveFunc(heightSensorMode, bool);
headerDefineSaveFunc(baseProfile, byte);
headerDefineSaveFunc(raiseOnPressure, bool);
headerDefineSaveFunc(internalReboot, bool);
headerDefineSaveFunc(learnPressureSensors, bool);
headerDefineSaveFunc(safetyMode, bool);

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