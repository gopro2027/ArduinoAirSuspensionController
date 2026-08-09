#ifndef manifoldSaveData_h
#define manifoldSaveData_h

#include <preferencable.h>

#include <Arduino.h>

#include <user_defines.h>

#include "pressureMath.h"
#include "BTOas.h"
class Profile
{
public:
    Preferencable pressure[4]; // byte
};

class AuxillaryOutputPreference {
    public:
    Preferencable auxillaryOutputMode; // byte
    Preferencable auxillaryOutputModeTimeUnit; // byte
    Preferencable auxillaryOutputTime; // uint8_t
    Preferencable auxillaryOutputInterval; // uint8_t

    Preferencable auxillaryIntervalCounter; // uint8_t
    void load() {
        auxillaryOutputMode.load("auxillaryOutputMode", AUX_MODE_NONE);
        auxillaryOutputModeTimeUnit.load("auxillaryOutputModeTimeUnit", AUX_MODE_TIME_DECISECONDS);
        auxillaryOutputTime.load("auxillaryOutputTime", 1);
        auxillaryOutputInterval.load("auxillaryOutputInterval", 0);

        auxillaryIntervalCounter.load("auxillaryIntervalCounter", 0);
    }
    void save(AuxillaryOutputModePayload payload) {
        auxillaryOutputMode.set(payload.mode);
        auxillaryOutputModeTimeUnit.set(payload.timeUnit);
        auxillaryOutputTime.set(payload.time);
        auxillaryOutputInterval.set(payload.interval);
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
    Preferencable sensorlessLeveling;
    Preferencable airOutOnShutoff;
    Preferencable heightSensorMode;
    Preferencable bagMaxPressure;
    Preferencable blePasskey;
    Preferencable bleName;
    Preferencable oledI2cAddr;
    Preferencable systemShutoffTimeM;
    Preferencable compressorOnPSI;
    Preferencable compressorOffPSI;
    Preferencable compressorCrankOffset;
    Preferencable pressureSensorMax;
    Preferencable bagVolumePercentage;
    Preferencable AirUpBagStretchTriggerBelowPressure;
    Preferencable AirUpBagStretchPressure;

    Preferencable rfButtonAPreset;
    Preferencable rfButtonBPreset;
    Preferencable rfButtonCPreset;
    Preferencable rfButtonDPreset;
    Preferencable heightSensorCalMin[4]; // double, raw height % at calibrated lowest point
    Preferencable heightSensorCalMax[4]; // double, raw height % at calibrated highest point
    Preferencable heightSensorCalMinRide[4]; // double, normalized height % at calibrated minimum ride height

    AuxillaryOutputPreference auxillaryOutputPreference;

    Profile profile[MAX_PROFILE_COUNT];
    Preferencable mlSampleRecord; // ML_SAMPLE_RECORD_VERSION (only version — weights are never persisted)
};

struct ProfileRaw {
    int profileNum;
    byte pressure[4];
};

extern SaveData _SaveData;
extern bool sendConfigBT;
void requestSendConfigBT();

void beginSaveData();
ProfileRaw readProfile(byte profileIndex);
void savePressuresToProfile(byte profileIndex, float _WHEEL_FRONT_PASSENGER, float _WHEEL_REAR_PASSENGER, float _WHEEL_FRONT_DRIVER, float _WHEEL_REAR_DRIVER);

headerDefineSaveFunc(riseOnStart, bool);
headerDefineSaveFunc(maintainPressure, bool);
headerDefineSaveFunc(sensorlessLeveling, bool);
headerDefineSaveFunc(airOutOnShutoff, bool);
headerDefineSaveFunc(heightSensorMode, bool);
headerDefineSaveFunc(baseProfile, byte);
headerDefineSaveFunc(raiseOnPressure, bool);
headerDefineSaveFunc(internalReboot, bool);
headerDefineSaveFunc(learnPressureSensors, bool);
headerDefineSaveFunc(safetyMode, bool);
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
headerDefineSaveFunc(oledI2cAddr, uint8_t);
headerDefineSaveFunc(systemShutoffTimeM, uint32_t); // may have to change
headerDefineSaveFunc(compressorOnPSI, uint8_t);
headerDefineSaveFunc(compressorOffPSI, uint8_t);
headerDefineSaveFunc(compressorCrankOffset, uint8_t); // seconds to hold the compressor off after power up / accessory power
headerDefineSaveFunc(pressureSensorMax, uint16_t);
headerDefineSaveFunc(bagVolumePercentage, uint16_t);
headerDefineSaveFunc(AirUpBagStretchTriggerBelowPressure, uint8_t);
headerDefineSaveFunc(AirUpBagStretchPressure, uint8_t);

headerDefineSaveFunc(rfButtonAPreset, uint8_t);
headerDefineSaveFunc(rfButtonBPreset, uint8_t);
headerDefineSaveFunc(rfButtonCPreset, uint8_t);
headerDefineSaveFunc(rfButtonDPreset, uint8_t);

headerDefineSaveFunc(auxillaryOutputMode, AuxillaryOutputMode);
headerDefineSaveFunc(auxillaryOutputModeTimeUnit, AuxillaryOutputModeTimeUnit);
headerDefineSaveFunc(auxillaryOutputTime, uint8_t);
headerDefineSaveFunc(auxillaryOutputInterval, uint8_t);
headerDefineSaveFunc(auxillaryIntervalCounter, uint8_t);

void saveAuxillaryOutputPreference(AuxillaryOutputModePayload payload);

float getHeightSensorMax();

float getheightCalMin(byte wheelNum);
float getheightCalMax(byte wheelNum);
float getheightCalMinRide(byte wheelNum);
void setheightCalMin(byte wheelNum, float value);
void setheightCalMax(byte wheelNum, float value);
void setheightCalMinRide(byte wheelNum, float value);

#endif