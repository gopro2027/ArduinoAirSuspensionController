#include "pressureMath.h"
#include "manifoldSaveData.h" // PressureLearnSaveStruct (for refit)

#ifdef test_run
#include <iostream>
#include <iomanip>
#endif

#define ML_N ML_NUM_COEFF

// Solve M * x = r (Gaussian elimination with partial pivoting). Copies inputs; false on singular/non-finite.
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
        double s = r[row];
        for (int c = row + 1; c < ML_N; c++)
        {
            s -= M[row * ML_N + c] * x[c];
        }
        x[row] = s / M[row * ML_N + row];
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

// Feature 0 is the flow-driving differential / 100 (scaled so the squared term stays O(1) and the fit is
// well conditioned): fill is driven by tank->bag, dump is driven by bag->atmosphere(~0). See AI_TRAINING.md.
void OffsetModel::computeFeatures(double raw_bag, double raw_tank, double others_open, double f[ML_NUM_COEFF])
{
    (void)others_open; // dropped as a feature: redundant with raw_tank via the differential, contributed ~0 to the fit
    double differential = this->up ? (raw_tank - raw_bag) : raw_bag;
    double d = differential / 100.0;
    f[0] = d;
    f[1] = d * d;
    f[2] = 1.0; // bias
}

// Predicted sensor offset in psi (settled - flowing). Fill reads high (negative offset), dump reads low.
double OffsetModel::predict(double raw_bag, double raw_tank, double others_open)
{
    if (!isfinite(raw_bag) || !isfinite(raw_tank) || raw_bag < 0 || raw_tank < 0)
    {
        return 0;
    }
    double f[ML_NUM_COEFF];
    computeFeatures(raw_bag, raw_tank, others_open, f);
    return (w[0] * f[0] + w[1] * f[1] + w[2] * f[2]) * ML_OFFSET_NORM;
}

int OffsetModel::refit(const PressureLearnSaveStruct *samples, int count)
{
    double A[ML_NUM_COEFF * ML_NUM_COEFF] = {0};
    double v[ML_NUM_COEFF] = {0};
    int n = 0;

    for (int i = 0; i < count; i++)
    {
        double raw = samples[i].raw_bag;
        double tank = samples[i].raw_tank;
        double others = samples[i].others_open;
        double f[ML_NUM_COEFF];
        computeFeatures(raw, tank, others, f);
        double y = ((double)samples[i].settled_bag - raw) / ML_OFFSET_NORM;
        for (int r = 0; r < ML_NUM_COEFF; r++)
        {
            for (int c = 0; c < ML_NUM_COEFF; c++)
            {
                A[r * ML_NUM_COEFF + c] += f[r] * f[c];
            }
            v[r] += f[r] * y;
        }
        n++;
    }

    if (n < ML_FIT_MIN_SAMPLES)
    {
        return 0; // not enough to fit; leave weights (the fade uses the constant default until then)
    }
    for (int i = 0; i < ML_NUM_COEFF; i++)
    {
        A[i * ML_NUM_COEFF + i] += ML_FIT_RIDGE * n;
    }
    double sol[ML_NUM_COEFF];
    if (!solveN(A, v, sol))
    {
        return 0;
    }
    for (int i = 0; i < ML_NUM_COEFF; i++)
    {
        w[i] = sol[i];
    }
    return n;
}
