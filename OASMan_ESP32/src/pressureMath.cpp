#include "pressureMath.h"

#ifdef test_run
#include <iostream>
#include <iomanip>
#endif

// ---------------------------------------------------------------------------
// 3x3 linear algebra helpers (row-major)
// ---------------------------------------------------------------------------

static double det3(const double M[9])
{
    return M[0] * (M[4] * M[8] - M[5] * M[7]) -
           M[1] * (M[3] * M[8] - M[5] * M[6]) +
           M[2] * (M[3] * M[7] - M[4] * M[6]);
}

// Solve M * x = r by Cramer's rule. Returns false when M is (near) singular.
static bool solve3(const double M[9], const double r[3], double x[3])
{
    double d = det3(M);
    if (!isfinite(d) || fabs(d) < 1e-12)
    {
        return false;
    }
    double T[9];
    for (int col = 0; col < 3; col++)
    {
        for (int i = 0; i < 9; i++)
        {
            T[i] = M[i];
        }
        T[col] = r[0];
        T[col + 3] = r[1];
        T[col + 6] = r[2];
        x[col] = det3(T) / d;
    }
    return isfinite(x[0]) && isfinite(x[1]) && isfinite(x[2]);
}

// Invert M via the adjugate. Returns false when M is (near) singular.
static bool invert3(const double M[9], double out[9])
{
    double d = det3(M);
    if (!isfinite(d) || fabs(d) < 1e-12)
    {
        return false;
    }
    out[0] = (M[4] * M[8] - M[5] * M[7]) / d;
    out[1] = (M[2] * M[7] - M[1] * M[8]) / d;
    out[2] = (M[1] * M[5] - M[2] * M[4]) / d;
    out[3] = (M[5] * M[6] - M[3] * M[8]) / d;
    out[4] = (M[0] * M[8] - M[2] * M[6]) / d;
    out[5] = (M[2] * M[3] - M[0] * M[5]) / d;
    out[6] = (M[3] * M[7] - M[4] * M[6]) / d;
    out[7] = (M[1] * M[6] - M[0] * M[7]) / d;
    out[8] = (M[0] * M[4] - M[1] * M[3]) / d;
    for (int i = 0; i < 9; i++)
    {
        if (!isfinite(out[i]))
        {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// AIModel
// ---------------------------------------------------------------------------

AIModel::AIModel()
{
    onlineInitDefault();
}

void AIModel::loadWeights(double _w1, double _w2, double _b)
{
    w1 = _w1;
    w2 = _w2;
    b = _b;
}

bool AIModel::isSampleValid(double start_pressure, double end_pressure, double tank_pressure)
{
    if (this->up)
    {
        // tank must be above both bag pressures or the subsonic log blows up to inf/nan
        return (tank_pressure > start_pressure) && (tank_pressure > end_pressure);
    }
    // venting features are finite for any non-negative gauge pressures
    return (start_pressure >= 0) && (end_pressure >= 0);
}

void AIModel::computeFeatures(double start_pressure, double end_pressure, double tank_pressure, double f[3])
{
    if (this->up)
    {
        // Subsonic regime: bag charges toward tank pressure like an RC circuit,
        // t = tau * ln((Pt-Ps)/(Pt-Pe)). Gauge/absolute offsets cancel in the differences.
        f[0] = log((tank_pressure - start_pressure) / (tank_pressure - end_pressure));
        // Choked regime: flow through the orifice is sonic while the bag is far below tank
        // pressure, so fill rate is constant and proportional to absolute tank pressure.
        f[1] = (end_pressure - start_pressure) / (tank_pressure + ML_PSI_ATMOSPHERE);
    }
    else
    {
        // Choked venting (bag above ~13 psi gauge): mass flow scales with absolute bag pressure,
        // so absolute pressure decays exponentially, t = tau * ln(Ps_abs/Pe_abs).
        f[0] = log((start_pressure + ML_PSI_ATMOSPHERE) / (end_pressure + ML_PSI_ATMOSPHERE));
        // Subsonic tail (below ~13 psi gauge): decay toward atmosphere, i.e. toward 0 gauge.
        // Clamp at 1 psi since the pure gauge log diverges at exactly atmospheric.
        double sg = start_pressure < 1 ? 1 : start_pressure;
        double eg = end_pressure < 1 ? 1 : end_pressure;
        f[1] = log(sg / eg);
    }
    f[2] = 1.0;
}

double AIModel::predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure)
{
    if (!isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        // 0 gets rejected by the aiPredict > 0 check in wheel.cpp, so the corner falls back to table timing
        return 0;
    }
    double f[3];
    computeFeatures(start_pressure, end_pressure, tank_pressure, f);
    return (w1 * f[0] + w2 * f[1] + b) * ML_TIME_NORM_MS;
}

void AIModel::onlineInitDefault()
{
    for (int i = 0; i < 9; i++)
    {
        P[i] = 0;
    }
    P[0] = ML_RLS_DEFAULT_PRIOR;
    P[4] = ML_RLS_DEFAULT_PRIOR;
    P[8] = ML_RLS_DEFAULT_PRIOR;
    errEma = 0;
    errCount = 0;
}

void AIModel::onlineSeedGate(double meanSquaredNormError)
{
    // floor keeps a suspiciously perfect fit from arming a gate that would reject everything
    const double floorSq = 1e-5; // ~16 ms rms in normalized units
    errEma = meanSquaredNormError > floorSq ? meanSquaredNormError : floorSq;
    errCount = ML_OUTLIER_GATE_WARMUP;
}

bool AIModel::trainOnline(double start_pressure, double end_pressure, double tank_pressure, double actual_time_ms)
{
    if (!isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        return false;
    }
    double f[3];
    computeFeatures(start_pressure, end_pressure, tank_pressure, f);
    double y = actual_time_ms / ML_TIME_NORM_MS;
    double err = y - (w1 * f[0] + w2 * f[1] + b);
    if (!isfinite(err))
    {
        // an inf/nan reaching the weights would be unrecoverable once saveWeights() commits it to nvs
        return false;
    }

    // Outlier gate: one bump/sensor glitch shouldn't yank the weights. Track a running mean of
    // squared error; once warmed up, skip samples ~3 sigma out. The ema contribution of a gated
    // sample is clipped so a burst of garbage can't talk its way past the gate.
    double e2 = err * err;
    bool gated = (errCount >= ML_OUTLIER_GATE_WARMUP) && (e2 > ML_OUTLIER_GATE_FACTOR * errEma);
    double e2Clipped = gated ? (ML_OUTLIER_GATE_FACTOR * errEma) : e2;
    errEma = (errCount == 0) ? e2Clipped : (0.95 * errEma + 0.05 * e2Clipped);
    errCount++;
    if (gated)
    {
        return false;
    }

    // Standard RLS with forgetting factor: exact least squares over an exponentially weighted window
    double Pf[3];
    for (int r = 0; r < 3; r++)
    {
        Pf[r] = P[r * 3] * f[0] + P[r * 3 + 1] * f[1] + P[r * 3 + 2] * f[2];
    }
    double denom = ML_RLS_FORGETTING + f[0] * Pf[0] + f[1] * Pf[1] + f[2] * Pf[2];
    if (!isfinite(denom) || denom < 1e-9)
    {
        return false;
    }
    double k[3] = {Pf[0] / denom, Pf[1] / denom, Pf[2] / denom};

    w1 += k[0] * err;
    w2 += k[1] * err;
    b += k[2] * err;

    // P = (P - k * Pf^T) / lambda. k is Pf scaled, so the update is a symmetric outer product
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            P[r * 3 + c] = (P[r * 3 + c] - k[r] * Pf[c]) / ML_RLS_FORGETTING;
        }
    }
    return true;
}

void AIModel::print_weights()
{
#ifdef test_run
    std::cout << std::setprecision(5) << "Weights: w1=" << w1 << ", w2=" << w2 << ", b=" << b << std::endl;
#else
    Serial.print("Weights: ");
    Serial.print("w1=");
    Serial.print(w1, 5);
    Serial.print(" w2=");
    Serial.print(w2, 5);
    Serial.print(" b=");
    Serial.print(b, 5);
    Serial.println();

    Serial.print("loadWeights(");
    Serial.print(w1, 5);
    Serial.print(",");
    Serial.print(w2, 5);
    Serial.print(",");
    Serial.print(b, 5);
    Serial.println(");");
#endif
}

// ---------------------------------------------------------------------------
// AIFitter
// ---------------------------------------------------------------------------

AIFitter::AIFitter()
{
    reset();
}

void AIFitter::reset()
{
    for (int i = 0; i < 9; i++)
    {
        A[i] = 0;
    }
    v[0] = v[1] = v[2] = 0;
    n = 0;
}

void AIFitter::add(AIModel &m, double start_pressure, double end_pressure, double tank_pressure, double actual_time_ms)
{
    if (!m.isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        return;
    }
    double f[3];
    m.computeFeatures(start_pressure, end_pressure, tank_pressure, f);
    double y = actual_time_ms / ML_TIME_NORM_MS;
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            A[r * 3 + c] += f[r] * f[c];
        }
        v[r] += f[r] * y;
    }
    n++;
}

int AIFitter::sampleCount()
{
    return n;
}

bool AIFitter::solveInto(AIModel &m)
{
    if (n < ML_FIT_MIN_SAMPLES)
    {
        return false;
    }
    double M[9];
    double ridge = ML_FIT_RIDGE * n;
    for (int i = 0; i < 9; i++)
    {
        M[i] = A[i];
    }
    M[0] += ridge;
    M[4] += ridge;
    M[8] += ridge;

    double w[3];
    if (!solve3(M, v, w))
    {
        return false;
    }
    m.w1 = w[0];
    m.w2 = w[1];
    m.b = w[2];
    return true;
}

bool AIFitter::initOnline(AIModel &m)
{
    m.onlineInitDefault();
    if (n < ML_FIT_MIN_SAMPLES)
    {
        return false;
    }
    double M[9];
    double ridge = ML_FIT_RIDGE * n;
    for (int i = 0; i < 9; i++)
    {
        M[i] = A[i];
    }
    M[0] += ridge;
    M[4] += ridge;
    M[8] += ridge;

    double Pinv[9];
    if (!invert3(M, Pinv))
    {
        return false;
    }
    for (int i = 0; i < 9; i++)
    {
        m.P[i] = Pinv[i];
    }
    return true;
}
