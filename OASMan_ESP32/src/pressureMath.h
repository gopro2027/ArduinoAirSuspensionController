
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
#define ML_MODEL_SCHEMA_VERSION 4

// On-disk PressureLearnSaveStruct layout. Bump only when size/field order changes (forces sample wipe).
#define ML_SAMPLE_RECORD_VERSION 2

#define ML_TIME_NORM_MS 5000.0
#define ML_PSI_ATMOSPHERE 14.7
#define ML_FIT_RIDGE 0.001
#define ML_FIT_MIN_SAMPLES 25
#define ML_NUM_COEFF 4 // f0, f1, others_flowing, bias

#define ML_RLS_FORGETTING 0.995
#define ML_RLS_DEFAULT_PRIOR 10.0
#define ML_OUTLIER_GATE_FACTOR 9.0
#define ML_OUTLIER_GATE_WARMUP 20
#define ML_ONLINE_ANCHOR_INTERVAL 5
#define ML_BATCH_RMSE_GATE_MS 400

class AIFitter;

class AIModel
{
    friend class AIFitter;

    double P[ML_NUM_COEFF * ML_NUM_COEFF]; // RLS covariance (RAM only; rebuilt at boot)
    double errEma;
    uint32_t errCount;

public:
    AIModel();

    double w1 = 0.1, w2 = 0.1, w3 = 0.0, b = 0.0;
    bool up = true;

    bool isSampleValid(double start_pressure, double end_pressure, double tank_pressure);
    void computeFeatures(double start_pressure, double end_pressure, double tank_pressure,
                         double others_flowing, double f[ML_NUM_COEFF]);
    void loadWeights(double _w1, double _w2, double _w3, double _b);
    // Predicted valve open time in ms; 0 means decline (caller falls back to table timing).
    double predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure,
                               double others_flowing);
    void onlineInitDefault();
    void onlineSeedGate(double meanSquaredNormError);
    bool trainOnline(double start_pressure, double end_pressure, double tank_pressure,
                     double others_flowing, double actual_time_ms);
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
    void add(AIModel &m, double start_pressure, double end_pressure, double tank_pressure,
             double others_flowing, double actual_time_ms);
    int sampleCount();
    bool solveInto(AIModel &m);
    bool initOnline(AIModel &m);
};

#endif
