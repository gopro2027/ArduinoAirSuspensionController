#ifndef aiPressureUtil_h
#define aiPressureUtil_h

#include "manifoldSaveData.h"

// The offset-model AI layer: refit the models from the logged samples, and predict the true bag value
// from a live (flowing) sensor reading. Samples themselves are stored by recordLearnSample in
// manifoldSaveData.cpp. Full design: AI_TRAINING.md + docs/pressure-goal-routine.md.

// Progress toward fully-trained (0-100%), reported over BLE.
extern uint8_t AIPercentage;
void updateAIPercentage();

// Refit any offset model whose sample count changed. Called at boot and periodically from task_trainAI.
void trainOffsetModels();
// Predicted true bag pressure from live flowing readings: rawBag + faded (default -> trained) offset.
double getPredictedBagPressure(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank);
double getPredictedBagHeight(double raw_level);

#endif
