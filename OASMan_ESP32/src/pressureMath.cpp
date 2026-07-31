#include "pressureMath.h"

#ifdef test_run
#include <iostream>
#include <iomanip>
#endif

// ---------------------------------------------------------------------------
// N x N linear algebra helpers (row-major, N = ML_NUM_COEFF)
//
// Gauss-Jordan with partial pivoting rather than Cramer's rule: at 4x4 the determinant expansion
// is both slower and worse conditioned, and pivoting is what keeps the near-collinear feature
// columns (f0 and f1 correlate around +0.95) from producing garbage weights.
// ---------------------------------------------------------------------------

#define ML_N ML_NUM_COEFF

// Solve M * x = r. Inputs are copied, so the caller's matrices survive.
static bool solveN(const double *Min, const double *rin, double *x)
{
    double M[ML_N * ML_N];
    double r[ML_N];
    for (int i = 0; i < ML_N * ML_N; i++)
    {
        M[i] = Min[i];
    }
    for (int i = 0; i < ML_N; i++)
    {
        r[i] = rin[i];
    }

    for (int col = 0; col < ML_N; col++)
    {
        int piv = col;
        for (int row = col + 1; row < ML_N; row++)
        {
            if (fabs(M[row * ML_N + col]) > fabs(M[piv * ML_N + col]))
            {
                piv = row;
            }
        }
        if (!isfinite(M[piv * ML_N + col]) || fabs(M[piv * ML_N + col]) < 1e-12)
        {
            return false;
        }
        if (piv != col)
        {
            for (int c = 0; c < ML_N; c++)
            {
                double t = M[col * ML_N + c];
                M[col * ML_N + c] = M[piv * ML_N + c];
                M[piv * ML_N + c] = t;
            }
            double t = r[col];
            r[col] = r[piv];
            r[piv] = t;
        }
        for (int row = col + 1; row < ML_N; row++)
        {
            double factor = M[row * ML_N + col] / M[col * ML_N + col];
            for (int c = col; c < ML_N; c++)
            {
                M[row * ML_N + c] -= factor * M[col * ML_N + c];
            }
            r[row] -= factor * r[col];
        }
    }

    for (int row = ML_N - 1; row >= 0; row--)
    {
        double sum = r[row];
        for (int c = row + 1; c < ML_N; c++)
        {
            sum -= M[row * ML_N + c] * x[c];
        }
        x[row] = sum / M[row * ML_N + row];
    }
    for (int i = 0; i < ML_N; i++)
    {
        if (!isfinite(x[i]))
        {
            return false;
        }
    }
    return true;
}

// Invert M by reducing [M | I] to [I | M^-1].
static bool invertN(const double *Min, double *out)
{
    double aug[ML_N * ML_N * 2];
    for (int row = 0; row < ML_N; row++)
    {
        for (int c = 0; c < ML_N; c++)
        {
            aug[row * ML_N * 2 + c] = Min[row * ML_N + c];
            aug[row * ML_N * 2 + ML_N + c] = (row == c) ? 1.0 : 0.0;
        }
    }

    for (int col = 0; col < ML_N; col++)
    {
        int piv = col;
        for (int row = col + 1; row < ML_N; row++)
        {
            if (fabs(aug[row * ML_N * 2 + col]) > fabs(aug[piv * ML_N * 2 + col]))
            {
                piv = row;
            }
        }
        double p = aug[piv * ML_N * 2 + col];
        if (!isfinite(p) || fabs(p) < 1e-12)
        {
            return false;
        }
        if (piv != col)
        {
            for (int c = 0; c < ML_N * 2; c++)
            {
                double t = aug[col * ML_N * 2 + c];
                aug[col * ML_N * 2 + c] = aug[piv * ML_N * 2 + c];
                aug[piv * ML_N * 2 + c] = t;
            }
            p = aug[col * ML_N * 2 + col];
        }
        for (int c = 0; c < ML_N * 2; c++)
        {
            aug[col * ML_N * 2 + c] /= p;
        }
        for (int row = 0; row < ML_N; row++)
        {
            if (row == col)
            {
                continue;
            }
            double factor = aug[row * ML_N * 2 + col];
            if (factor == 0)
            {
                continue;
            }
            for (int c = 0; c < ML_N * 2; c++)
            {
                aug[row * ML_N * 2 + c] -= factor * aug[col * ML_N * 2 + c];
            }
        }
    }

    for (int row = 0; row < ML_N; row++)
    {
        for (int c = 0; c < ML_N; c++)
        {
            double val = aug[row * ML_N * 2 + ML_N + c];
            if (!isfinite(val))
            {
                return false;
            }
            out[row * ML_N + c] = val;
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

void AIModel::loadWeights(double _w1, double _w2, double _w3, double _b)
{
    w1 = _w1;
    w2 = _w2;
    w3 = _w3;
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

void AIModel::computeFeatures(double start_pressure, double end_pressure, double tank_pressure,
                              double others_flowing, double f[ML_NUM_COEFF])
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
    // Contention: all four corners share one tank filling and one exhaust dumping, so the same
    // pressure move takes materially longer with others flowing than it does alone.
    f[2] = others_flowing;
    f[3] = 1.0;
}

double AIModel::predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure,
                                    double others_flowing)
{
    if (!isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        // 0 gets rejected by the aiPredict > 0 check in wheel.cpp, so the corner falls back to table timing
        return 0;
    }
    double f[ML_NUM_COEFF];
    computeFeatures(start_pressure, end_pressure, tank_pressure, others_flowing, f);
    return (w1 * f[0] + w2 * f[1] + w3 * f[2] + b) * ML_TIME_NORM_MS;
}

void AIModel::onlineInitDefault()
{
    for (int i = 0; i < ML_N * ML_N; i++)
    {
        P[i] = 0;
    }
    for (int i = 0; i < ML_N; i++)
    {
        P[i * ML_N + i] = ML_RLS_DEFAULT_PRIOR;
    }
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

bool AIModel::trainOnline(double start_pressure, double end_pressure, double tank_pressure,
                          double others_flowing, double actual_time_ms)
{
    if (!isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        return false;
    }
    double f[ML_NUM_COEFF];
    computeFeatures(start_pressure, end_pressure, tank_pressure, others_flowing, f);
    double y = actual_time_ms / ML_TIME_NORM_MS;
    double err = y - (w1 * f[0] + w2 * f[1] + w3 * f[2] + b);
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
    double Pf[ML_N];
    double denom = ML_RLS_FORGETTING;
    for (int r = 0; r < ML_N; r++)
    {
        double sum = 0;
        for (int c = 0; c < ML_N; c++)
        {
            sum += P[r * ML_N + c] * f[c];
        }
        Pf[r] = sum;
    }
    for (int i = 0; i < ML_N; i++)
    {
        denom += f[i] * Pf[i];
    }
    if (!isfinite(denom) || denom < 1e-9)
    {
        return false;
    }
    double k[ML_N];
    for (int i = 0; i < ML_N; i++)
    {
        k[i] = Pf[i] / denom;
    }

    w1 += k[0] * err;
    w2 += k[1] * err;
    w3 += k[2] * err;
    b += k[3] * err;

    // P = (P - k * Pf^T) / lambda. k is Pf scaled, so the update is a symmetric outer product
    for (int r = 0; r < ML_N; r++)
    {
        for (int c = 0; c < ML_N; c++)
        {
            P[r * ML_N + c] = (P[r * ML_N + c] - k[r] * Pf[c]) / ML_RLS_FORGETTING;
        }
    }
    return true;
}

void AIModel::print_weights()
{
#ifdef test_run
    std::cout << std::setprecision(5) << "Weights: w1=" << w1 << ", w2=" << w2 << ", w3=" << w3
              << ", b=" << b << std::endl;
#else
    Serial.print("Weights: ");
    Serial.print("w1=");
    Serial.print(w1, 5);
    Serial.print(" w2=");
    Serial.print(w2, 5);
    Serial.print(" w3=");
    Serial.print(w3, 5);
    Serial.print(" b=");
    Serial.print(b, 5);
    Serial.println();

    Serial.print("loadWeights(");
    Serial.print(w1, 5);
    Serial.print(",");
    Serial.print(w2, 5);
    Serial.print(",");
    Serial.print(w3, 5);
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
    for (int i = 0; i < ML_N * ML_N; i++)
    {
        A[i] = 0;
    }
    for (int i = 0; i < ML_N; i++)
    {
        v[i] = 0;
    }
    n = 0;
}

void AIFitter::add(AIModel &m, double start_pressure, double end_pressure, double tank_pressure,
                   double others_flowing, double actual_time_ms)
{
    if (!m.isSampleValid(start_pressure, end_pressure, tank_pressure))
    {
        return;
    }
    double f[ML_NUM_COEFF];
    m.computeFeatures(start_pressure, end_pressure, tank_pressure, others_flowing, f);
    double y = actual_time_ms / ML_TIME_NORM_MS;
    for (int r = 0; r < ML_N; r++)
    {
        for (int c = 0; c < ML_N; c++)
        {
            A[r * ML_N + c] += f[r] * f[c];
        }
        v[r] += f[r] * y;
    }
    n++;
}

int AIFitter::sampleCount()
{
    return n;
}

// Copy of the accumulated normal equations with ridge added to the diagonal.
void AIFitter::ridged(double *out)
{
    double ridge = ML_FIT_RIDGE * n;
    for (int i = 0; i < ML_N * ML_N; i++)
    {
        out[i] = A[i];
    }
    for (int i = 0; i < ML_N; i++)
    {
        out[i * ML_N + i] += ridge;
    }
}

bool AIFitter::solveInto(AIModel &m)
{
    if (n < ML_FIT_MIN_SAMPLES)
    {
        return false;
    }
    double M[ML_N * ML_N];
    ridged(M);

    double w[ML_N];
    if (!solveN(M, v, w))
    {
        return false;
    }
    m.w1 = w[0];
    m.w2 = w[1];
    m.w3 = w[2];
    m.b = w[3];
    return true;
}

bool AIFitter::initOnline(AIModel &m)
{
    m.onlineInitDefault();
    if (n < ML_FIT_MIN_SAMPLES)
    {
        return false;
    }
    double M[ML_N * ML_N];
    ridged(M);

    double Pinv[ML_N * ML_N];
    if (!invertN(M, Pinv))
    {
        return false;
    }
    for (int i = 0; i < ML_N * ML_N; i++)
    {
        m.P[i] = Pinv[i];
    }
    return true;
}
