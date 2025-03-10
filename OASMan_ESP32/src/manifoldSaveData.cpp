#include "manifoldSaveData.h"

SaveData _SaveData;
byte currentProfile[4];
bool sendProfileBT = false;

void beginSaveData()
{
    _SaveData.riseOnStart.load("riseOnStart", false);
    _SaveData.maintainPressure.load("maintainPressure", false);
    _SaveData.airOutOnShutoff.load("airOutOnShutoff", false);
    _SaveData.heightSensorMode.load("heightSensorMode", false);
    _SaveData.baseProfile.load("baseProfile", 2);
    _SaveData.raiseOnPressure.load("raiseOnPressure", false);
    _SaveData.internalReboot.load("internalReboot", false);
    // things moves from inside the user config
    _SaveData.bagMaxPressure.load("bagMaxPressure", MAX_PRESSURE_SAFETY);
    _SaveData.blePasskey.load("blePasskey", BLE_PASSKEY);
    _SaveData.systemShutoffTimeM.load("systemShutoffTimeM", SYSTEM_SHUTOFF_TIME_M);
    _SaveData.compressorOnPSI.load("compressorOnPSI", COMPRESSOR_ON_BELOW_PSI);
    _SaveData.compressorOffPSI.load("compressorOffPSI", COMPRESSOR_MAX_PSI);
    _SaveData.pressureSensorMax.load("pressureSensorMax", pressuretransducermaxPSI);
    _SaveData.bagVolumePercentage.load("bagVolumePercentage", 100);
    for (int i = 0; i < MAX_PROFILE_COUNT; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            // first create a custom name for it. This would probably be better off done as different namespaces or something but idc
            char buf[15];
            snprintf(buf, sizeof(buf), "profile%i|%i", i, j);
            _SaveData.profile[i].pressure[j].load(buf, 50);
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

createSaveFuncInt(riseOnStart, bool);
createSaveFuncInt(maintainPressure, bool);
createSaveFuncInt(airOutOnShutoff, bool);
createSaveFuncInt(heightSensorMode, bool);
createSaveFuncInt(baseProfile, byte);
createSaveFuncInt(raiseOnPressure, bool);
createSaveFuncInt(internalReboot, bool);

// values moved from the user defines file
createSaveFuncInt(bagMaxPressure, uint8_t);
createSaveFuncInt(blePasskey, uint32_t);         // 6 digits base 10
createSaveFuncInt(systemShutoffTimeM, uint32_t); // may have to change
createSaveFuncInt(compressorOnPSI, uint8_t);
createSaveFuncInt(compressorOffPSI, uint8_t);
createSaveFuncInt(pressureSensorMax, uint16_t);
createSaveFuncInt(bagVolumePercentage, uint16_t);

float getHeightSensorMax()
{
    return 100.0f;
}