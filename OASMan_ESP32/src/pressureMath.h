
#ifndef pressureMath_h
#define pressureMath_h
// test_run is defined by the PC eval harness (eval/model_eval.cpp) to compile this file off-target
#ifdef test_run
#include <math.h>
#include <stdint.h>
#else
#include <Arduino.h>
#endif

// Schema version of the model feature set. Bump whenever AIModel's features change meaning:
// manifoldSaveData checks it at boot and resets stored weights + ready flags so weights trained
// on old features are never used with new math. Bootstrap samples are kept, so the model retrains
// instantly from SPIFFS on the next boot.
#define ML_MODEL_SCHEMA_VERSION 2

// Valve time normalization scale in ms (model trains/predicts time / this)
#define ML_TIME_NORM_MS 5000.0

// psi offset from gauge to absolute pressure
#define ML_PSI_ATMOSPHERE 14.7

// ---- batch fit (closed-form least squares) ----
#define ML_FIT_RIDGE 0.001    // per-sample Tikhonov regularization, keeps the 3x3 solve well conditioned
#define ML_FIT_MIN_SAMPLES 25 // don't trust a fit with fewer valid samples than this

// ---- online learning (RLS) ----
#define ML_RLS_FORGETTING 0.995     // exponential memory of roughly 1/(1-lambda) = 200 samples
#define ML_RLS_DEFAULT_PRIOR 10.0   // P diagonal when there is no bootstrap data to rebuild P from
#define ML_OUTLIER_GATE_FACTOR 9.0  // skip a sample when err^2 > factor * running mean err^2 (~3 sigma)
#define ML_OUTLIER_GATE_WARMUP 20   // samples seen before the outlier gate arms
#define ML_ONLINE_ANCHOR_INTERVAL 5 // every Nth online sample, replay one retained bootstrap sample

// ---- usage guards (see trainSingleAIModel / wheel.cpp) ----
#define ML_BATCH_RMSE_GATE_MS 400  // batch fit must beat this RMSE on its own data before setReady

class AIFitter;

class AIModel
{
    friend class AIFitter;

    // RLS state (RAM only; rebuilt at boot from the retained bootstrap samples)
    double P[9]; // 3x3 covariance, row major, kept symmetric
    double errEma;
    uint32_t errCount;

public:
    AIModel();

    // Weights for each input
    double w1 = 0.1, w2 = 0.1, b = 0.0;
    bool up = true;

    /** True when computeFeatures() will produce finite values for this sample. Written with
     * positive comparisons so nan inputs also come back invalid. */
    bool isSampleValid(double start_pressure, double end_pressure, double tank_pressure);

    /** Physics features, computed from raw gauge psi. f[2] is the bias term (always 1).
     * Up:   f0 = ln((Pt-Ps)/(Pt-Pe))  subsonic charge toward tank pressure
     *       f1 = (Pe-Ps)/(Pt+atm)     choked/sonic regime: fill rate scales with absolute tank pressure
     * Down: f0 = ln((Ps+atm)/(Pe+atm)) choked venting: absolute pressure decays toward zero
     *       f1 = ln(max(Ps,1)/max(Pe,1)) subsonic tail: gauge pressure decays toward atmosphere */
    void computeFeatures(double start_pressure, double end_pressure, double tank_pressure, double f[3]);

    void loadWeights(double _w1, double _w2, double _b);

    /** Predicted valve open time in ms. Returns 0 for invalid inputs so callers fall back to table timing. */
    double predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure);

    /** Reset RLS covariance/gate to defaults (used when no bootstrap data exists to rebuild from). */
    void onlineInitDefault();

    /** Arm the outlier gate immediately using the batch fit's mean squared (normalized) error,
     * instead of waiting for ML_OUTLIER_GATE_WARMUP online samples to learn the noise floor. */
    void onlineSeedGate(double meanSquaredNormError);

    /** One recursive-least-squares step on a single sample, with outlier gating.
     * Returns true when the weights were actually updated. */
    bool trainOnline(double start_pressure, double end_pressure, double tank_pressure, double actual_time_ms);

    void print_weights();
};

/** Accumulates samples and solves the exact least-squares weights for an AIModel (the model is
 * linear in w1/w2/b, so the normal equations give the true optimum - no epochs, no learning rate). */
class AIFitter
{
    double A[9]; // X^T X
    double v[3]; // X^T y
    int n;

public:
    AIFitter();
    void reset();
    /** Accumulate one sample (silently skips samples that fail isSampleValid). */
    void add(AIModel &m, double start_pressure, double end_pressure, double tank_pressure, double actual_time_ms);
    int sampleCount();
    /** Solve and store the weights into the model. False when there are too few samples or the system is singular. */
    bool solveInto(AIModel &m);
    /** Initialize the model's RLS covariance from this fit (P = inv(A + ridge)). Falls back to
     * onlineInitDefault() and returns false when the matrix can't be inverted. */
    bool initOnline(AIModel &m);
};

#endif
