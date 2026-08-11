#include "manifoldSaveData.h"
#include "aiPressureUtil.h" // setupPressureSamples (the AI sample store lives there, not here)

SaveData _SaveData;
// Set by background tasks (e.g. sensorless levelling auto-disable) to ask the BLE task to
// re-broadcast the current config values to all authed clients so their UIs stay in sync.
bool sendConfigBT = false;
void requestSendConfigBT()
{
    sendConfigBT = true;
}

void beginSaveData()
{

    // update related data
    _SaveData.updateMode.load("updateMode", false);
    _SaveData.wifiSSID.loadString("wifiSSID", "");
    _SaveData.wifiPassword.loadString("wifiPassword", "");
    _SaveData.updateResult.load("updateResult", 0);

    if (getupdateMode())
    {
        return;
    }

    _SaveData.riseOnStart.load("riseOnStart", false);
    _SaveData.maintainPressure.load("maintainPressure", false);
    _SaveData.sensorlessLeveling.load("sensorlessLevel", false);
    _SaveData.airOutOnShutoff.load("airOutOnShutoff", false);
    _SaveData.heightSensorMode.load("heightSensorMode", false);
    _SaveData.baseProfile.load("baseProfile", 2);
    _SaveData.raiseOnPressure.load("raiseOnPressure", false);
    _SaveData.internalReboot.load("internalReboot", false);
    _SaveData.learnPressureSensors.load("learnPressureSensors", false);
    _SaveData.safetyMode.load("safetyMode", true);

    // pressure sensor values
    _SaveData.pressureInputFrontPassenger.load("PIFP", 0);
    _SaveData.pressureInputRearPassenger.load("PIRP", 1);
    _SaveData.pressureInputFrontDriver.load("PIFD", 2);
    _SaveData.pressureInputRearDriver.load("PIRD", 3);
    _SaveData.pressureInputTank.load("PIT", 4);

    // things moves from inside the user config
    _SaveData.bagMaxPressure.load("bagMaxPressure", MAX_PRESSURE_SAFETY);
    _SaveData.blePasskey.load("blePasskey", BLE_PASSKEY);
    _SaveData.bleName.loadString("bleName", BT_NAME);
    _SaveData.oledI2cAddr.load("oledI2cAddr", (uint64_t)SCREEN_ADDRESS);

    _SaveData.rfButtonAPreset.load("rfButtonAPreset", RIDE_HEIGHT_PRESET_NUMBER);
    _SaveData.rfButtonBPreset.load("rfButtonBPreset", RIDE_HEIGHT_PRESET_NUMBER);
    _SaveData.rfButtonCPreset.load("rfButtonCPreset", RIDE_HEIGHT_PRESET_NUMBER);
    _SaveData.rfButtonDPreset.load("rfButtonDPreset", RIDE_HEIGHT_PRESET_NUMBER);

    _SaveData.systemShutoffTimeM.load("systemShutoffTimeM", SYSTEM_SHUTOFF_TIME_M);
    _SaveData.compressorOnPSI.load("compressorOnPSI", COMPRESSOR_ON_BELOW_PSI);
    _SaveData.compressorOffPSI.load("compressorOffPSI", COMPRESSOR_MAX_PSI);
    _SaveData.compressorCrankOffset.load("compCrankOff", COMPRESSOR_CRANK_OFFSET_S); // key kept under 15 chars (Preferencable::name limit)
    _SaveData.pressureSensorMax.load("pressureSensorMax", pressuretransducermaxPSI);
    _SaveData.AirUpBagStretchTriggerBelowPressure.load("bagStretchTrig", 40); // if bag is currently below 40 psi...
    _SaveData.AirUpBagStretchPressure.load("bagStretchPsi", 0); // go to this pressure to stretch bag out first

    _SaveData.auxillaryOutputPreference.load();

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

    for (int j = 0; j < 4; j++)
    {
        char buf[15];
        snprintf(buf, sizeof(buf), "hsCalMin%i", j);
        _SaveData.heightSensorCalMin[j].loadDouble(buf, 0.0);
        snprintf(buf, sizeof(buf), "hsCalMax%i", j);
        _SaveData.heightSensorCalMax[j].loadDouble(buf, 100.0);
        snprintf(buf, sizeof(buf), "hsCalMinRide%i", j);
        _SaveData.heightSensorCalMinRide[j].loadDouble(buf, 0.0);
    }

    setupPressureSamples(); // allocate + load the AI sample store (skipped above in OTA/update mode)
}

ProfileRaw readProfile(byte profileIndex)
{
    static ProfileRaw profile;
    profile.profileNum = profileIndex;
    profile.pressure[WHEEL_FRONT_PASSENGER] = _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_PASSENGER].get().i;
    profile.pressure[WHEEL_REAR_PASSENGER] = _SaveData.profile[profileIndex].pressure[WHEEL_REAR_PASSENGER].get().i;
    profile.pressure[WHEEL_FRONT_DRIVER] = _SaveData.profile[profileIndex].pressure[WHEEL_FRONT_DRIVER].get().i;
    profile.pressure[WHEEL_REAR_DRIVER] = _SaveData.profile[profileIndex].pressure[WHEEL_REAR_DRIVER].get().i;
    return profile;
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
createSaveFuncInt(sensorlessLeveling, bool);
createSaveFuncInt(airOutOnShutoff, bool);
createSaveFuncInt(heightSensorMode, bool);
createSaveFuncInt(baseProfile, byte);
createSaveFuncInt(raiseOnPressure, bool);
createSaveFuncInt(internalReboot, bool);
createSaveFuncInt(learnPressureSensors, bool);
createSaveFuncInt(safetyMode, bool);
createSaveFuncInt(updateMode, bool);

createSaveFuncString(wifiSSID);
createSaveFuncString(wifiPassword);
createSaveFuncInt(updateResult, byte);

// pressure sensor values
createSaveFuncInt(pressureInputFrontPassenger, byte);
createSaveFuncInt(pressureInputRearPassenger, byte);
createSaveFuncInt(pressureInputFrontDriver, byte);
createSaveFuncInt(pressureInputRearDriver, byte);
createSaveFuncInt(pressureInputTank, byte);

// values moved from the user defines file
createSaveFuncInt(bagMaxPressure, uint8_t);
createSaveFuncInt(blePasskey, uint32_t); // 6 digits base 10
createSaveFuncString(bleName);
createSaveFuncInt(oledI2cAddr, uint8_t);
createSaveFuncInt(systemShutoffTimeM, uint32_t); // may have to change
createSaveFuncInt(compressorOnPSI, uint8_t);
createSaveFuncInt(compressorOffPSI, uint8_t);
createSaveFuncInt(compressorCrankOffset, uint8_t);
createSaveFuncInt(pressureSensorMax, uint16_t);
createSaveFuncInt(AirUpBagStretchTriggerBelowPressure, uint8_t);
createSaveFuncInt(AirUpBagStretchPressure, uint8_t);

createSaveFuncInt(rfButtonAPreset, uint8_t);
createSaveFuncInt(rfButtonBPreset, uint8_t);
createSaveFuncInt(rfButtonCPreset, uint8_t);
createSaveFuncInt(rfButtonDPreset, uint8_t);

// auxillary output preference is in it's own class so we have custom functions defined
AuxillaryOutputMode getauxillaryOutputMode() {
    return (AuxillaryOutputMode)_SaveData.auxillaryOutputPreference.auxillaryOutputMode.get().i;
}
AuxillaryOutputModeTimeUnit getauxillaryOutputModeTimeUnit() {
    return (AuxillaryOutputModeTimeUnit)_SaveData.auxillaryOutputPreference.auxillaryOutputModeTimeUnit.get().i;
}
uint8_t getauxillaryOutputTime() {
    return _SaveData.auxillaryOutputPreference.auxillaryOutputTime.get().i;
}
uint8_t getauxillaryOutputInterval() {
    return _SaveData.auxillaryOutputPreference.auxillaryOutputInterval.get().i;
}

// not part of the config, this is the counter value we store to decide when to trigger based on the interval
uint8_t getauxillaryIntervalCounter() {
    return _SaveData.auxillaryOutputPreference.auxillaryIntervalCounter.get().i;
}
void setauxillaryIntervalCounter(uint8_t value) {
    _SaveData.auxillaryOutputPreference.auxillaryIntervalCounter.set(value);
}


void saveAuxillaryOutputPreference(AuxillaryOutputModePayload payload) {
    _SaveData.auxillaryOutputPreference.save(payload);
}


float getHeightSensorMax()
{
    return 100.0f;
}

float getheightCalMin(byte wheelNum)
{
    return _SaveData.heightSensorCalMin[wheelNum].get().d;
}
float getheightCalMax(byte wheelNum)
{
    return _SaveData.heightSensorCalMax[wheelNum].get().d;
}
void setheightCalMin(byte wheelNum, float value)
{
    _SaveData.heightSensorCalMin[wheelNum].setDouble(value);
}
void setheightCalMax(byte wheelNum, float value)
{
    _SaveData.heightSensorCalMax[wheelNum].setDouble(value);
}
float getheightCalMinRide(byte wheelNum)
{
    return _SaveData.heightSensorCalMinRide[wheelNum].get().d;
}
void setheightCalMinRide(byte wheelNum, float value)
{
    _SaveData.heightSensorCalMinRide[wheelNum].setDouble(value);
}
