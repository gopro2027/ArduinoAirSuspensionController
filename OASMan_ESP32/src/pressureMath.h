
#ifndef pressureMath_h
#define pressureMath_h
// test_run is defined by the PC eval harness (eval/model_eval.cpp) to compile this file off-target
#ifdef test_run
#include <math.h>
#include <stdint.h>
#else
#include <Arduino.h>
#endif

// On-disk PressureLearnSaveStruct layout version. Bump only when the sample struct changes (forces a wipe).
// There is no separate model/feature version: weights are never persisted, they are re-fit from the
// samples every boot, so a feature change just takes effect on the next refit. See AI_TRAINING.md.
#define ML_SAMPLE_RECORD_VERSION 3

#define ML_OFFSET_NORM 100.0   // label/prediction scale, keeps weights O(0.1)
#define ML_FIT_RIDGE 0.001     // ridge added to the normal-equations diagonal (per sample)
#define ML_FIT_MIN_SAMPLES 25  // minimum valid samples for a stable 4-coeff fit
#define ML_NUM_COEFF 4         // features: differential/100, (differential/100)^2, others_open, bias

struct PressureLearnSaveStruct; // defined in manifoldSaveData.h

// Per axle-direction linear model of the pressure-sensor OFFSET during flow (settled - flowing), in psi.
// Weights are re-derived from all stored samples by closed-form least squares (refit) — there is no
// online learner and nothing is persisted. actualBag = rawBag + predict(...). See AI_TRAINING.md.
class OffsetModel
{
public:
    double w[ML_NUM_COEFF] = {0, 0, 0, 0};
    bool up = true; // fill (differential = tank - bag) vs dump (differential = bag - atmosphere ~ bag)

    void computeFeatures(double raw_bag, double raw_tank, double others_open, double f[ML_NUM_COEFF]);
    double predict(double raw_bag, double raw_tank, double others_open);
    // Ridge least-squares fit over the samples. Returns the valid-sample count used (0 = too few /
    // singular, in which case the weights are left unchanged).
    int refit(const PressureLearnSaveStruct *samples, int count);
};

#endif
