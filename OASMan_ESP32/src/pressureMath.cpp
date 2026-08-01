#include "pressureMath.h"

#ifdef test_run
#include <iostream>
#include <iomanip>
#endif

#define ML_N ML_NUM_COEFF

// Solve M * x = r (Gaussian elimination with partial pivoting). Copies inputs.
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

bool AIModel::isSampleValid(double raw_bag, double raw_tank, double others_open)
{
    return isfinite(raw_bag) && isfinite(raw_tank) && isfinite(others_open) &&
           raw_bag >= 0 && raw_tank >= 0;
}

// Feature 0 is the flow-driving differential / 100 (scaled so the squared term stays O(1) and the fit
// is well conditioned): fill is driven by tank->bag, dump is driven by bag->atmosphere(~0). See AI_TRAINING.md.
void AIModel::computeFeatures(double raw_bag, double raw_tank, double others_open, double f[ML_NUM_COEFF])
{
    double differential = this->up ? (raw_tank - raw_bag) : raw_bag;
    double d = differential / 100.0;
    f[0] = d;
    f[1] = d * d;
    f[2] = others_open;
    f[3] = 1.0;
}

// Sensor offset in psi (settled - flowing). Fill reads high (negative offset), dump reads low (positive).
double AIModel::predictOffset(double raw_bag, double raw_tank, double others_open)
{
    if (!isSampleValid(raw_bag, raw_tank, others_open))
    {
        return 0;
    }
    double f[ML_NUM_COEFF];
    computeFeatures(raw_bag, raw_tank, others_open, f);
    return (w1 * f[0] + w2 * f[1] + w3 * f[2] + b) * ML_OFFSET_NORM;
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
    const double floorSq = 1e-5;
    errEma = meanSquaredNormError > floorSq ? meanSquaredNormError : floorSq;
    errCount = ML_OUTLIER_GATE_WARMUP;
}

bool AIModel::trainOnline(double raw_bag, double raw_tank, double others_open, double offset_psi)
{
    if (!isSampleValid(raw_bag, raw_tank, others_open))
    {
        return false;
    }
    double f[ML_NUM_COEFF];
    computeFeatures(raw_bag, raw_tank, others_open, f);
    double y = offset_psi / ML_OFFSET_NORM;
    double err = y - (w1 * f[0] + w2 * f[1] + w3 * f[2] + b);
    if (!isfinite(err))
    {
        return false;
    }

    double e2 = err * err;
    bool gated = (errCount >= ML_OUTLIER_GATE_WARMUP) && (e2 > ML_OUTLIER_GATE_FACTOR * errEma);
    double e2Clipped = gated ? (ML_OUTLIER_GATE_FACTOR * errEma) : e2;
    errEma = (errCount == 0) ? e2Clipped : (0.95 * errEma + 0.05 * e2Clipped);
    errCount++;
    if (gated)
    {
        return false;
    }

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

void AIFitter::add(AIModel &m, double raw_bag, double raw_tank, double others_open, double offset_psi)
{
    if (!m.isSampleValid(raw_bag, raw_tank, others_open))
    {
        return;
    }
    double f[ML_NUM_COEFF];
    m.computeFeatures(raw_bag, raw_tank, others_open, f);
    double y = offset_psi / ML_OFFSET_NORM;
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
