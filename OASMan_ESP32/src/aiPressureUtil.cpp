#include "aiPressureUtil.h"
#include "manifoldSaveData.h" // _SaveData.mlSampleRecord (the on-disk sample layout version)
#include "sampleReading.tcc"  // readBytes / writeBytes / deleteFile

#pragma region sample_store

static int learnDataIndex[4];
// Heap-allocated lazily in loadSamples. beginSaveData() returns before setupPressureSamples() in
// OTA/update mode, so this is never allocated during an OTA. Each row holds LEARN_SAVE_COUNT samples.
static PressureLearnSaveStruct *learnData[4] = {nullptr, nullptr, nullptr, nullptr};
static SemaphoreHandle_t learnDataMutex;

// The 4 offset models (RAM only). up=false for the two dump models is set in loadSamples.
static OffsetModel offsetModels[4];

// Block until the learn-data lock is held (no-op if the mutex does not exist yet).
static void learnDataLock()
{
    if (learnDataMutex == NULL)
    {
        return;
    }
    while (xSemaphoreTake(learnDataMutex, 1) != pdTRUE)
    {
        delay(1);
    }
}

static void learnDataUnlock()
{
    if (learnDataMutex != NULL)
    {
        xSemaphoreGive(learnDataMutex);
    }
}

OffsetModel *getOffsetModel(SOLENOID_AI_INDEX index)
{
    return &offsetModels[index];
}

PressureLearnSaveStruct *getLearnData(SOLENOID_AI_INDEX index)
{
    return learnData[index];
}

int getLearnDataLength(SOLENOID_AI_INDEX index)
{
    return learnDataIndex[index];
}

static const char *getLogFileName(SOLENOID_AI_INDEX index)
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
    default:
        return nullptr; // AI_MODEL_UNDEFINED: no file to log to
    }
}

static void initDataFile(SOLENOID_AI_INDEX index)
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

// Allocate the rows if needed, read the stored samples off SPIFFS, and dump them over serial.
static void loadSamples()
{
    for (int i = 0; i < 4; i++)
    {
        if (learnData[i] == nullptr)
        {
            learnData[i] = (PressureLearnSaveStruct *)malloc(LEARN_SAVE_COUNT * sizeof(PressureLearnSaveStruct));
            if (learnData[i] == nullptr)
            {
                Serial.print("FATAL: failed to allocate learnData row ");
                Serial.println(i);
                learnDataIndex[i] = 0;
                continue;
            }
        }
        learnDataIndex[i] = readBytes(getLogFileName((SOLENOID_AI_INDEX)i), learnData[i], LEARN_SAVE_COUNT * sizeof(PressureLearnSaveStruct)) / sizeof(PressureLearnSaveStruct);
        // Weights are not persisted; they start at zero and the training task re-fits them from these
        // samples (the fade uses the constant default until there are enough). See AI_TRAINING.md.
    }

    offsetModels[SOLENOID_AI_INDEX::AI_MODEL_DOWN_FRONT].up = false;
    offsetModels[SOLENOID_AI_INDEX::AI_MODEL_DOWN_REAR].up = false;

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

void setupPressureSamples()
{
    loadSamples();

    learnDataMutex = xSemaphoreCreateMutex();

    // One AI version: the on-disk sample layout. A change wipes samples (weights are never persisted;
    // they are re-fit from the samples each boot). See "Schema migration" in AI_TRAINING.md.
    _SaveData.mlSampleRecord.load("mlSampleRec", 0);
    if (_SaveData.mlSampleRecord.get().i != ML_SAMPLE_RECORD_VERSION)
    {
        Serial.print("AI sample record layout changed to ");
        Serial.print(ML_SAMPLE_RECORD_VERSION);
        Serial.println(", clearing stored samples");
        clearPressureData();
        _SaveData.mlSampleRecord.set(ML_SAMPLE_RECORD_VERSION);
    }
}

void clearPressureData()
{
    // Called from the BLE task while wheel tasks may be inside recordLearnSample, so hold the lock
    // across the wipe or a sample can land after the index reset and desync it from the file.
    learnDataLock();
    for (int i = 0; i < 4; i++)
    {
        deleteFile(getLogFileName((SOLENOID_AI_INDEX)i));
        learnDataIndex[i] = 0;
        for (int c = 0; c < ML_NUM_COEFF; c++)
        {
            offsetModels[i].w[c] = 0;
        }
    }
    AIPercentage = 0;
    learnDataUnlock();

    loadSamples(); // re-read (now empty) + dump over serial to confirm the wipe
}

// Once a model's file is full we simply stop collecting (the fit from LEARN_SAVE_COUNT samples is plenty).
void recordLearnSample(SOLENOID_AI_INDEX aiIndex, uint8_t raw_bag, uint8_t settled_bag, uint8_t raw_tank)
{
    int *size = &learnDataIndex[aiIndex];

    if (*size >= LEARN_SAVE_COUNT)
    {
        return;
    }

    learnDataLock();

    // Both front wheels share AI_MODEL_UP_FRONT (and both rears share AI_MODEL_UP_REAR), so two wheel tasks can land here at the same time. This is the size check that actually matters since it is inside the semaphore now and safe
    if (*size < LEARN_SAVE_COUNT)
    {
        PressureLearnSaveStruct *pls = getLearnData(aiIndex);
        if (pls == nullptr)
        {
            learnDataUnlock();
            return; // row failed to allocate at boot; skip logging rather than dereference null
        }
        // De-dup: skip a sample within SAMPLE_DEDUP_PSI of the previous stored one (a preset move closes
        // the valve many times, which would log long runs of near-identical samples). See AI_TRAINING.md.
        if (*size > 0 &&
            abs((int)raw_bag - (int)pls[*size - 1].raw_bag) <= SAMPLE_DEDUP_PSI &&
            abs((int)settled_bag - (int)pls[*size - 1].settled_bag) <= SAMPLE_DEDUP_PSI)
        {
            learnDataUnlock();
            return; // near-duplicate of the last stored sample -> don't log
        }
        pls[*size].raw_bag = raw_bag;
        pls[*size].settled_bag = settled_bag;
        pls[*size].raw_tank = raw_tank;

        writeBytes(getLogFileName(aiIndex), &pls[*size], sizeof(PressureLearnSaveStruct), "a");

        *size = *size + 1;
    }

    learnDataUnlock();

    updateAIPercentage();
}

#pragma endregion

#pragma region training

uint8_t AIPercentage = 0;

// Re-fit one model's weights from its stored samples (closed-form ridge least squares, ~1 ms). This is
// the ONLY training path: no online learner, nothing persisted — more samples just means another refit.
void refitModel(SOLENOID_AI_INDEX index)
{
    OffsetModel *m = getOffsetModel(index);
    int len = getLearnDataLength(index);
    int used = m->refit(getLearnData(index), len);
    Serial.printf("Refit model %i: %i samples, used %i  w=[%.4f %.4f %.4f]\n",
                  (int)index, len, used, m->w[0], m->w[1], m->w[2]);
}

// Refit any model whose stored sample count changed since the last call. Called once at boot (refits all,
// since lastCount seeds to -1) and then every 100 ms from task_trainAI.
void trainOffsetModels()
{
    static int lastCount[4] = {-1, -1, -1, -1};
    for (int i = 0; i < 4; i++)
    {
        int c = getLearnDataLength((SOLENOID_AI_INDEX)i);
        if (c != lastCount[i])
        {
            refitModel((SOLENOID_AI_INDEX)i);
            lastCount[i] = c;
        }
    }
    updateAIPercentage();
}

void updateAIPercentage()
{
    // Progress toward full AI weighting (100% == every model at AI_LEARN_RATIO_NUM samples, i.e.
    // AI fully driving), not toward the LEARN_SAVE_COUNT collection ceiling. See AI_TRAINING.md.
    int totalLen = 0;
    for (int i = 0; i < 4; i++)
    {
        int len = getLearnDataLength((SOLENOID_AI_INDEX)i);
        totalLen += len < AI_LEARN_RATIO_NUM ? len : AI_LEARN_RATIO_NUM;
    }
    AIPercentage = ((float)totalLen / ((float)AI_LEARN_RATIO_NUM * 4)) * 100;
}

// Fraction of the trained model to trust, ramping in with sample count: 0 below OFFSET_FADE_MIN, linearly
// up to 1 at AI_LEARN_RATIO_NUM. Below the min, the flat constant default is used (-/+ OFFSET_DEFAULT_PSI
// for up/down). Raw flowing readings alone are wrong by ~10 psi, so an offset is always applied.
static double getPredictionOffset(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank)
{
    OffsetModel *m = getOffsetModel(aiIndex);
    double def = m->up ? -(double)OFFSET_DEFAULT_PSI : (double)OFFSET_DEFAULT_PSI;
    int count = getLearnDataLength(aiIndex);
    if (count < OFFSET_FADE_MIN)
    {
        return def;
    }
    double trained = m->predict(raw_bag, raw_tank);
    double w = (double)(count - OFFSET_FADE_MIN) / (double)(AI_LEARN_RATIO_NUM - OFFSET_FADE_MIN);
    if (w > 1.0)
    {
        w = 1.0;
    }
    return def * (1.0 - w) + trained * w;
}

// Predicted true bag pressure recovered from the live flowing readings while a valve is open. The
// closed-loop controller stops when this reaches goal. See AI_TRAINING.md.
double getPredictedBagPressure(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank)
{
    return raw_bag + getPredictionOffset(aiIndex, raw_bag, raw_tank);
}

// Height-mode counterpart to getPredictedBagPressure. The level sensor reads the true ride height even
// while a valve is open, so this is an identity stub — it exists so the closed-loop controller can treat
// both modes through one predictor seam. (A future valve-close-lag model could live here; see the
// "Future improvements" note in AI_TRAINING.md.)
double getPredictedBagHeight(double raw_level)
{
    return raw_level;
}

#pragma endregion
