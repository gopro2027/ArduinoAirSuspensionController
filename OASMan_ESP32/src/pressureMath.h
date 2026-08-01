
#ifndef pressureMath_h
#define pressureMath_h
// test_run is defined by the PC eval harness (eval/model_eval.cpp) to compile this file off-target
#ifdef test_run
#include <math.h>
#include <stdint.h>
#else
#include <Arduino.h>
#endif

// Feature-set version. Bump when computeFeatures / coefficient meaning changes.
// Boot clears weights only and refits from samples — see AI_TRAINING.md.
#define ML_MODEL_SCHEMA_VERSION 6 // ; was: 5

// On-disk PressureLearnSaveStruct layout. Bump only when size/field order changes (forces sample wipe).
#define ML_SAMPLE_RECORD_VERSION 3 // ; was: 2

// The model now predicts the pressure-sensor OFFSET during flow (settled - flowing) so the closed-loop
// controller can recover the true bag pressure while a valve is open. offset is in psi; labels/predictions
// are scaled by ML_OFFSET_NORM to keep weights O(0.1) and the RLS/gate tuning valid. See AI_TRAINING.md.
#define ML_OFFSET_NORM 100.0
#define ML_PSI_ATMOSPHERE 14.7
#define ML_FIT_RIDGE 0.001
#define ML_FIT_MIN_SAMPLES 25
#define ML_NUM_COEFF 4 // f0 (differential/100), f1 (differential/100 squared), others_open, bias

#define ML_RLS_FORGETTING 0.995
#define ML_RLS_DEFAULT_PRIOR 10.0
#define ML_OUTLIER_GATE_FACTOR 9.0
#define ML_OUTLIER_GATE_WARMUP 20
#define ML_ONLINE_ANCHOR_INTERVAL 5
#define ML_BATCH_RMSE_GATE_PSI 8 // reject a batch fit whose self-RMSE exceeds this many psi

class AIFitter;

class AIModel
{
    friend class AIFitter;

    double P[ML_NUM_COEFF * ML_NUM_COEFF]; // RLS covariance (RAM only; rebuilt at boot)
    double errEma;
    uint32_t errCount;

public:
    AIModel();

    double w1 = 0.0, w2 = 0.0, w3 = 0.0, b = 0.0;
    bool up = true; // fill (differential = tank-bag) vs dump (differential = bag-atmosphere)

    bool isSampleValid(double raw_bag, double raw_tank, double others_open);
    void computeFeatures(double raw_bag, double raw_tank, double others_open, double f[ML_NUM_COEFF]);
    void loadWeights(double _w1, double _w2, double _w3, double _b);
    // Predicted sensor offset in psi (settled - flowing). actualBag = raw_bag + predictOffset(...).
    double predictOffset(double raw_bag, double raw_tank, double others_open);
    void onlineInitDefault();
    void onlineSeedGate(double meanSquaredNormError);
    bool trainOnline(double raw_bag, double raw_tank, double others_open, double offset_psi);
    void print_weights();
};

// Closed-form least squares accumulator; see AI_TRAINING.md.
class AIFitter
{
    double A[ML_NUM_COEFF * ML_NUM_COEFF];
    double v[ML_NUM_COEFF];
    int n;

    void ridged(double *out);

public:
    AIFitter();
    void reset();
    void add(AIModel &m, double raw_bag, double raw_tank, double others_open, double offset_psi);
    int sampleCount();
    bool solveInto(AIModel &m);
    bool initOnline(AIModel &m);
};

#endif
