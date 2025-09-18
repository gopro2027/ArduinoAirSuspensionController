#include "manifoldSaveData.h"

SaveData _SaveData;
byte currentProfile[4];
bool sendProfileBT = false;

int learnDataIndex[4];
PressureLearnSaveStruct learnData[4][LEARN_SAVE_COUNT];
static SemaphoreHandle_t learnDataMutex;

PressureLearnSaveStruct *getLearnData(SOLENOID_AI_INDEX index)
{
    return learnData[index];
}

int getLearnDataLength(SOLENOID_AI_INDEX index)
{
    return learnDataIndex[index];
}

const char *getLogFileName(SOLENOID_AI_INDEX index)
{
    switch (index)
    {
    case SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT:
        return "/UpDataF.dat";
    case SOLENOID_AI_INDEX::AI_MODEL_UP_REAR:
        return "/UpDataR.dat";
    case SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT:
        return "/DownDataF.dat";
    case SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR:
        return "/DownDataR.dat";
    }
}

void initDataFile(SOLENOID_AI_INDEX index)
{
    Serial.print(getLogFileName(index));
    Serial.print(" (");

    PressureLearnSaveStruct *pls = getLearnData(index);
    int size = getLearnDataLength(index);
    Serial.print(size);
    Serial.println("):");

    for (int i = 0; i < size; i++)
    {
        pls[i].print();
        Serial.print(", ");
    }
    Serial.println();
}

#define LOG_DATA_SIZE 2000
#define LOG_FILE_NAME "/log.txt"
void setupSpiffsLog()
{
    // char data[LOG_DATA_SIZE];
    // memset(data,0,LOG_DATA_SIZE);
    // int size = readBytes(LOG_FILE_NAME, data, LOG_DATA_SIZE);
    // if (size > LOG_DATA_SIZE) {
    //     //deleteFile(LOG_FILE_NAME);
    // }
    // if (size > 0) {
    //     Serial.println("\n\nSPIFFS DATA LOG:\n");
    //     Serial.println(data);
    //     Serial.println("\nEND SPIFFS DATA LOG\n\n");
    // }
}

void writeToSpiffsLog(char *text)
{
    // writeBytes(LOG_FILE_NAME, text, strlen(text), "a");
}

void loadAILearnedDataPreferences()
{
    // load the 4 models and learn data
    for (int i = 0; i < 4; i++)
    {
        learnDataIndex[i] = readBytes(getLogFileName((SOLENOID_AI_INDEX)i), learnData[i], LEARN_SAVE_COUNT * sizeof(PressureLearnSaveStruct)) / sizeof(PressureLearnSaveStruct);
        char buf[15];
        snprintf(buf, sizeof(buf), "model%i|%i", i, 0);
        _SaveData.aiModels[i].weights[0].loadDouble(buf, 0.1);
        snprintf(buf, sizeof(buf), "model%i|%i", i, 1);
        _SaveData.aiModels[i].weights[1].loadDouble(buf, 0.1);
        snprintf(buf, sizeof(buf), "model%i|%i", i, 2);
        _SaveData.aiModels[i].weights[2].loadDouble(buf, 0.0);
        snprintf(buf, sizeof(buf), "model%i|r", i);
        _SaveData.aiModels[i].isReadyToUse.load(buf, false);
        _SaveData.aiModels[i].loadModel(); // copy the values to the internal model
        // Serial.println(getAIModel((SOLENOID_AI_INDEX)i)->isReadyToUse.get().i);
    }

    _SaveData.aiModels[SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT].model.up = false;
    _SaveData.aiModels[SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR].model.up = false;

    for (int i = 0; i < 10; i++)
        Serial.println("");
    Serial.println("BEGIN IMPORTANT DATA FOR PRO");
    Serial.println(sizeof(PressureLearnSaveStruct));
    for (int i = 0; i < 4; i++)
    {
        initDataFile((SOLENOID_AI_INDEX)i);
    }
    Serial.println("END IMPORTANT DATA FOR PRO");
    for (int i = 0; i < 10; i++)
        Serial.println("");
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
    _SaveData.aiEnabled.load("aiEnabled", true);
    _SaveData.updateMode.load("updateMode", false);
    _SaveData.wifiSSID.loadString("wifiSSID", "");
    _SaveData.wifiPassword.loadString("wifiPassword", "");
    _SaveData.updateResult.load("updateResult", 0);

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

    loadAILearnedDataPreferences();

    learnDataMutex = xSemaphoreCreateMutex();
    // downDataMutex = xSemaphoreCreateMutex();

    // Reset ai models
    //  _SaveData.upModel.weights[0].setDouble(0.1);
    //  _SaveData.upModel.weights[1].setDouble(0.1);
    //  _SaveData.upModel.weights[2].setDouble(-0.1);
    //  _SaveData.upModel.weights[3].setDouble(0.1);
    //  _SaveData.upModel.weights[4].setDouble(0.1);
    //  _SaveData.upModel.weights[5].setDouble(0.0);

    // _SaveData.downModel.weights[0].setDouble(0.1);
    // _SaveData.downModel.weights[1].setDouble(0.1);
    // _SaveData.downModel.weights[2].setDouble(0.0);
    // _SaveData.downModel.weights[3].setDouble(0.0);
    // _SaveData.downModel.weights[4].setDouble(0.1);
    // _SaveData.downModel.weights[5].setDouble(0.0);

    // _SaveData.upModel.count.set(0);
    // _SaveData.downModel.count.set(0);
}

extern uint8_t AIReadyBittset;
extern uint8_t AIPercentage;
void clearPressureData()
{
    for (int i = 0; i < 4; i++)
    {
        // reset the file
        deleteFile(getLogFileName((SOLENOID_AI_INDEX)i));

        // reset the models too
        _SaveData.aiModels[i].deletePreferences();
    }
    AIReadyBittset = 0;
    AIPercentage = 0;
    loadAILearnedDataPreferences();
}

extern void updateAIPercentage();
void appendPressureDataToFile(SOLENOID_AI_INDEX aiIndex, uint8_t start_pressure, uint8_t goal_pressure, uint16_t tank_pressure, uint32_t timeMS)
{
    int *size = &learnDataIndex[aiIndex];

    if (abs((int)start_pressure - (int)goal_pressure) < 1)
    {
        // Don't want to spam a ton of super low valve timings where the pressures basically didn't move. This happened to a tester and the result was a ton of useless repetitive data saved where we could have had more useful data.
        return;
    }

    // first initial check for size before we open the semaphore, just to prevent constantly opening a semaphore every time this is called if it's full
    if (*size < LEARN_SAVE_COUNT)
    {

        // quick check to make sure it actually went in the right direction.... idk why it was messing up sometimes
        if (aiIndex == SOLENOID_AI_INDEX::AI_MODEL_UP_FRONT || aiIndex == SOLENOID_AI_INDEX::AI_MODEL_UP_REAR)
        {
            if ((int)goal_pressure - (int)start_pressure < 0)
            {
                return;
            }
        }
        else
        {
            if ((int)start_pressure - (int)goal_pressure < 0)
            {
                return;
            }
        }

        // while (xSemaphoreTake(learnDataMutex, 1) != pdTRUE)
        // {
        //     delay(1);
        // }

        PressureLearnSaveStruct *pls = getLearnData(aiIndex);

        // This is the actual important size check since it is inside of the semaphore now and safe
        if (*size < LEARN_SAVE_COUNT)
        {
            pls[*size].start_pressure = start_pressure;
            pls[*size].goal_pressure = goal_pressure;
            pls[*size].tank_pressure = tank_pressure;
            pls[*size].timeMS = timeMS;

            // This is full write. Do append instead
            // writeBytes(getLogFileName(aiIndex),pls,(*size)*sizeof(PressureLearnSaveStruct));

            writeBytes(getLogFileName(aiIndex), &pls[*size], sizeof(PressureLearnSaveStruct), "a");

            *size = *size + 1; // moved to after append
        }

        // xSemaphoreGive(learnDataMutex);
    }

    updateAIPercentage();
}

AIModelPreference *getAIModel(SOLENOID_AI_INDEX aiIndex)
{
    return &_SaveData.aiModels[aiIndex];
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
createSaveFuncInt(learnPressureSensors, bool);
createSaveFuncInt(safetyMode, bool);
createSaveFuncInt(aiEnabled, bool);
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
createSaveFuncInt(blePasskey, uint32_t);         // 6 digits base 10
createSaveFuncString(bleName);
createSaveFuncInt(systemShutoffTimeM, uint32_t); // may have to change
createSaveFuncInt(compressorOnPSI, uint8_t);
createSaveFuncInt(compressorOffPSI, uint8_t);
createSaveFuncInt(pressureSensorMax, uint16_t);
createSaveFuncInt(bagVolumePercentage, uint16_t);

float getHeightSensorMax()
{
    return 100.0f;
}
