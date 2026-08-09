#ifndef aiPressureUtil_h
#define aiPressureUtil_h

#include <Arduino.h>
#include <user_defines.h>

#include "pressureMath.h"

// The offset-model AI: the sample store (SPIFFS + RAM mirror), the refit that derives the model weights
// from those samples, and the prediction that recovers the true bag value from a live flowing sensor
// reading. Full design: AI_TRAINING.md + docs/pressure-goal-routine.md.

// One pressure-offset sample: the flowing sensor readings captured just before a valve closed, plus the
// settled bag reading after it closed. The model learns offset = settled_bag - raw_bag.
struct PressureLearnSaveStruct
{
    uint8_t raw_bag;     // flowing bag reading captured just before the valve closed
    uint8_t settled_bag; // settled bag reading after close (label)
    uint8_t raw_tank;    // flowing tank reading at capture
    void print()
    {
        Serial.print("{");
        Serial.print((int)raw_bag);
        Serial.print(", ");
        Serial.print((int)settled_bag);
        Serial.print(", ");
        Serial.print((int)raw_tank);
        Serial.print("}");
    }
};

// Allocate + load the sample store and apply the on-disk layout migration. Called once from
// beginSaveData (after the preferences load, and never in OTA/update mode so no memory is used there).
void setupPressureSamples();

PressureLearnSaveStruct *getLearnData(SOLENOID_AI_INDEX aiIndex);
int getLearnDataLength(SOLENOID_AI_INDEX aiIndex);
// Append one offset sample to the model's SPIFFS file + RAM mirror (the training task re-fits from it).
void recordLearnSample(SOLENOID_AI_INDEX aiIndex, uint8_t raw_bag, uint8_t settled_bag, uint8_t raw_tank);
void clearPressureData();

// The 4 offset models are RAM-only, re-fit from the stored samples every boot (nothing persisted).
OffsetModel *getOffsetModel(SOLENOID_AI_INDEX aiIndex);

// Progress toward fully-trained (0-100%), reported over BLE.
extern uint8_t AIPercentage;
void updateAIPercentage();

// Refit any offset model whose sample count changed. Called at boot and periodically from task_trainAI.
void trainOffsetModels();
// Predicted true bag pressure from live flowing readings: rawBag + faded (default -> trained) offset.
double getPredictedBagPressure(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank);
double getPredictedBagHeight(double raw_level);

#endif
