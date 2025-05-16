#include "manifoldSaveData.h"

SaveData _SaveData;
byte currentProfile[4];
bool sendProfileBT = false;

#define LEARN_SAVE_COUNT 400
int upDataIndex = 0;
PressureLearnSaveStruct upData[LEARN_SAVE_COUNT];
int downDataIndex = 0;
PressureLearnSaveStruct downData[LEARN_SAVE_COUNT];
static SemaphoreHandle_t learnDataMutex;

PressureLearnSaveStruct *getUpData() {
    return upData;
}
PressureLearnSaveStruct *getDownData() {
    return downData;
}
int getUpDataLength() {
    return upDataIndex;
}
int getDownDataLength() {
    return downDataIndex;
}


const char *getLogFileName(bool up) {
    if (up) {
        return "UpDatat";
    } else {
        return "DownDatat";
    }
}

void initDataFile(bool up) {
    Serial.print(getLogFileName(up));
    Serial.print(" (");

    PressureLearnSaveStruct *pls = up ? upData : downData;
    int size = up ? upDataIndex : downDataIndex;
    Serial.print(size);
    Serial.println("):");


    for (int i = 0; i < size; i++) {
        pls[i].print();
        Serial.print(", ");
    }
    Serial.println();
}

void beginSaveData()
{
    _SaveData.riseOnStart.load("riseOnStart", false);
    _SaveData.maintainPressure.load("maintainPressure", false);
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

    // _SaveData.upModel.weights[0].loadDouble("upmod0", 0.1);
    // _SaveData.upModel.weights[1].loadDouble("upmod1", 0.1);
    // _SaveData.upModel.weights[2].loadDouble("upmod2", -0.1);
    // _SaveData.upModel.weights[3].loadDouble("upmod3", 0.1);
    // _SaveData.upModel.weights[4].loadDouble("upmod4", 0.1);
    // _SaveData.upModel.weights[5].loadDouble("upmod5", 0.0);
    // _SaveData.upModel.count.load("upmodcount",0);

    // _SaveData.downModel.weights[0].loadDouble("downmod0", 0.1);
    // _SaveData.downModel.weights[1].loadDouble("downmod1", 0.1);
    // _SaveData.downModel.weights[2].loadDouble("downmod2", 0.0);
    // _SaveData.downModel.weights[3].loadDouble("downmod3", 0.0);
    // _SaveData.downModel.weights[4].loadDouble("downmod4", 0.1);
    // _SaveData.downModel.weights[5].loadDouble("downmod5", 0.0);
    // _SaveData.downModel.count.load("downmodcount",0);

    // _SaveData.upModel.loadModel();
    // _SaveData.downModel.loadModel();

    // temporary override based on corvette data:
    // Training data on all 5 weights... seems to be lackluster with some occelation
    // _SaveData.upModel.model.loadWeights(-0.037828, 0.49027, -0.24911, 0.34766, -0.26119, -0.032774);
    // _SaveData.downModel.model.loadWeights(1.0217, -0.63391, -0.057, -0.57535, -0.22858, 0.30494);
    // _SaveData.upModel.model.useWeight4 = true;
    // _SaveData.upModel.model.useWeight5 = true;

    // training data on just 3 base weights seem to work good.
    // _SaveData.upModel.model.loadWeights(-0.54644, 0.65552, -0.077218, 0.70383, -4.1894, 0.0057034);
    // _SaveData.downModel.model.loadWeights(1.0276, -0.93082, 0.069306, -0.9215, -1.4924, -0.084615);
    // _SaveData.upModel.model.useWeight4 = false;
    // _SaveData.upModel.model.useWeight5 = false;
    // _SaveData.downModel.model.useWeight4 = false;
    // _SaveData.downModel.model.useWeight5 = false;

    // // training data on just 3 base weights seem to work good.
    // _SaveData.upModel.model.loadWeights(-0.34525, 0.45432, -0.076937, 0.40201, 0.1, -0.19555);
    // _SaveData.downModel.model.loadWeights(0.76399, -0.66687, 0.070163, -0.5265, 0.1, 0.17787);
    // _SaveData.upModel.model.useWeight4 = true;
    // _SaveData.upModel.model.useWeight5 = false;
    // _SaveData.downModel.model.useWeight4 = true;
    // _SaveData.downModel.model.useWeight5 = false;


    upDataIndex = readBytes(getLogFileName(true), upData, LEARN_SAVE_COUNT * sizeof(PressureLearnSaveStruct)) / sizeof(PressureLearnSaveStruct);
    downDataIndex = readBytes(getLogFileName(false), downData, LEARN_SAVE_COUNT * sizeof(PressureLearnSaveStruct)) / sizeof(PressureLearnSaveStruct);

    for (int i = 0; i < 10; i++)
        Serial.println("");
    Serial.println("BEGIN IMPORTANT DATA FOR PRO");
    Serial.println(sizeof(PressureLearnSaveStruct));
    initDataFile(true);
    initDataFile(false);
    Serial.println("END IMPORTANT DATA FOR PRO");
    for (int i = 0; i < 10; i++)
        Serial.println("");

    learnDataMutex = xSemaphoreCreateMutex();
    //downDataMutex = xSemaphoreCreateMutex();

    //Reset ai models
    // _SaveData.upModel.weights[0].setDouble(0.1);
    // _SaveData.upModel.weights[1].setDouble(0.1);
    // _SaveData.upModel.weights[2].setDouble(-0.1);
    // _SaveData.upModel.weights[3].setDouble(0.1);
    // _SaveData.upModel.weights[4].setDouble(0.1);
    // _SaveData.upModel.weights[5].setDouble(0.0);

    // _SaveData.downModel.weights[0].setDouble(0.1);
    // _SaveData.downModel.weights[1].setDouble(0.1);
    // _SaveData.downModel.weights[2].setDouble(0.0);
    // _SaveData.downModel.weights[3].setDouble(0.0);
    // _SaveData.downModel.weights[4].setDouble(0.1);
    // _SaveData.downModel.weights[5].setDouble(0.0);
   
    // _SaveData.upModel.count.set(0);
    // _SaveData.downModel.count.set(0);
}

void clearPressureData() {
    deletePreference(getLogFileName(true));
    deletePreference(getLogFileName(false));
}


void appendPressureDataToFile(bool up,uint8_t start_pressure, uint8_t goal_pressure, uint16_t tank_pressure, uint32_t timeMS) {

    int *size = up ? &upDataIndex : &downDataIndex;
    // first initial check for size before we open the semaphore, just to prevent constantly opening a semaphore every time this is called if it's full
    if (*size < LEARN_SAVE_COUNT) {

        // quick check to make sure it actually went in the right direction.... idk why it was messing up sometimes
        if (up) {
            if ((int)goal_pressure - (int)start_pressure < 0) {
                return;
            }
        } else {
            if ((int)start_pressure - (int)goal_pressure < 0) {
                return;
            }
        }

        while (xSemaphoreTake(learnDataMutex, 1) != pdTRUE)
        {
            delay(1);
        }

        PressureLearnSaveStruct *pls = up ? upData : downData;

        // This is the actual important size check since it is inside of the semaphore now and safe
        if (*size < LEARN_SAVE_COUNT) {
            pls[*size].start_pressure = start_pressure;
            pls[*size].goal_pressure = goal_pressure;
            pls[*size].tank_pressure = tank_pressure;
            pls[*size].timeMS = timeMS;
            *size = *size + 1;

            saveBytes(getLogFileName(up),up?upData:downData,(*size)*sizeof(PressureLearnSaveStruct));
        }

        xSemaphoreGive(learnDataMutex);
    }
}

// AIModelPreference *getAIModelPreference(bool up) {
//     AIModelPreference *model;
//     if (up) {
//         model = &_SaveData.upModel;
//     } else {
//         model = &_SaveData.downModel;
//     }
//     return model;
// }

// void trainAiModel(bool up, double start_pressure, double end_pressure, double tank_pressure, double actual_time) {
//     AIModelPreference *model = getAIModelPreference(up);
//     model->model.train(start_pressure, end_pressure, tank_pressure, actual_time);

//     while (xSemaphoreTake(learnDataMutex, 1) != pdTRUE)
//     {
//         delay(1);
//     }
//     model->count.set(model->count.get().i+1);
//     model->saveWeights();
//     xSemaphoreGive(learnDataMutex);
// }

// double getAiPredictionTime(bool up, double start_pressure, double end_pressure, double tank_pressure) {
//     AIModelPreference *model = getAIModelPreference(up);
//     return model->model.predictDeNormalized(start_pressure, end_pressure, tank_pressure);
// }

// uint64_t getAiCount(bool up) {
//     AIModelPreference *model = getAIModelPreference(up);
//     return model->count.get().i;
// }

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
createSaveFuncInt(learnPressureSensors, bool);
createSaveFuncInt(safetyMode, bool);

// pressure sensor values
createSaveFuncInt(pressureInputFrontPassenger, byte);
createSaveFuncInt(pressureInputRearPassenger, byte);
createSaveFuncInt(pressureInputFrontDriver, byte);
createSaveFuncInt(pressureInputRearDriver, byte);
createSaveFuncInt(pressureInputTank, byte);

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