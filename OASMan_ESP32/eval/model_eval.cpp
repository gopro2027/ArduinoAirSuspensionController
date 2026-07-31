// PC evaluation harness for the AI valve-timing models. Never compiled into firmware.
//
// Compares, on the real recorded datasets (extracted from src/pressureMath.cpp by extract_datasets.py):
//   A) old model (v1 features) trained with the old 10k-epoch SGD   <- what shipped before
//   B) old model (v1 features) fit with exact least squares         <- isolates the training method
//   C) new model (v2 physics features) fit with exact least squares <- isolates the features
//   D) C + streaming the tail of the data through RLS online updates, clean and with corrupted samples
//
// Build & run (from this directory):
//   g++ -O2 -o model_eval model_eval.cpp && ./model_eval

#define test_run
#include "../src/pressureMath.cpp" // production model code, compiled for PC
#include "datasets.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// The pre-v2 production model, reproduced verbatim for comparison
// ---------------------------------------------------------------------------

struct OldModel
{
    double learning_rate = 0.00075;
    double w1 = 0.1, w2 = 0.1, b = 0.0;
    bool up = true;

    static double normalize(double x, double min, double max) { return (x - min) / (max - min); }
    static double denormalize(double x, double min, double max) { return x * (max - min) + min; }

    bool isSampleValid(double s, double e, double t)
    {
        if (up)
            return (t > 0) && (t > e);
        return (e > 0) && (s > 0);
    }

    double predict(double s, double e, double t)
    {
        if (!isSampleValid(s, e, t))
            return 0;
        s = normalize(s, 0, 200);
        e = normalize(e, 0, 200);
        t = normalize(t, 0, 200);
        double result;
        if (up)
        {
            double x = log(t / (t - e));
            result = w1 * x + w2 * (e - s) + b;
        }
        else
        {
            result = w1 * log(s / e) + b;
        }
        return result;
    }

    double predictDeNormalized(double s, double e, double t)
    {
        if (up == false)
        {
            if (s < 1)
                s = 1;
            if (e < 1)
                e = 1;
        }
        return denormalize(predict(s, e, t), 0, 5000);
    }

    void train(double s, double e, double t, double actual_time)
    {
        if (!isSampleValid(s, e, t))
            return;
        double pred = predict(s, e, t);
        double error = pred - normalize(actual_time, 0, 5000);
        double sn = normalize(s, 0, 200), en = normalize(e, 0, 200), tn = normalize(t, 0, 200);
        if (up)
        {
            double x = log(tn / (tn - en));
            w1 -= learning_rate * error * x;
            w2 -= learning_rate * error * (en - sn);
            b -= learning_rate * error;
        }
        else
        {
            w1 -= learning_rate * error * log(sn / en);
            b -= learning_rate * error;
        }
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static uint64_t rngState = 0x9E3779B97F4A7C15ull;
static uint64_t nextRand()
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 7;
    rngState ^= rngState << 17;
    return rngState;
}

// The recording-path filter from wheel.cpp / manifoldSaveData.cpp: correct direction,
// moved more than 3 psi, valve open longer than 10 ms.
static std::vector<EvalSample> filterLikeFirmware(const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> out;
    for (int i = 0; i < len; i++)
    {
        const EvalSample &x = ds[i];
        bool dirOk = up ? (x.e - x.s >= 0) : (x.s - x.e >= 0);
        if (dirOk && fabs(x.s - x.e) > 3 && x.ms > 10)
            out.push_back(x);
    }
    return out;
}

struct Metrics
{
    double mae, rmse, bias; // bias = mean signed error (positive: predictions too long)
};

template <typename PredFn>
static Metrics score(const std::vector<EvalSample> &set, PredFn pred)
{
    double sumAbs = 0, sumSq = 0, sum = 0;
    int n = 0;
    for (const EvalSample &x : set)
    {
        double p = pred(x);
        double d = p - x.ms;
        sumAbs += fabs(d);
        sumSq += d * d;
        sum += d;
        n++;
    }
    Metrics m;
    m.mae = n ? sumAbs / n : 0;
    m.rmse = n ? sqrt(sumSq / n) : 0;
    m.bias = n ? sum / n : 0;
    return m;
}

// ---------------------------------------------------------------------------------------------
// Compatibility shims for the experiments below, which all predate others_flowing.
//
// The production model now takes a contention count. Forwarding zero reproduces exactly what those
// experiments originally measured: with an all-zero third column the normal equations reduce to the
// old three-parameter system (ridge alone occupies that diagonal, so its weight solves to zero).
// New work should use AIModel/AIFitter directly and pass the real count - see evalContention.
// ---------------------------------------------------------------------------------------------
struct AIModelNC : AIModel
{
    using AIModel::computeFeatures;
    using AIModel::predictDeNormalized;
    double predictDeNormalized(double s, double e, double tank)
    {
        return AIModel::predictDeNormalized(s, e, tank, 0);
    }
    void computeFeatures(double s, double e, double tank, double f[ML_NUM_COEFF])
    {
        AIModel::computeFeatures(s, e, tank, 0, f);
    }
    using AIModel::trainOnline;
    bool trainOnline(double s, double e, double tank, double ms)
    {
        return AIModel::trainOnline(s, e, tank, 0, ms);
    }
    using AIModel::loadWeights;
    void loadWeights(double a, double b_, double c) { AIModel::loadWeights(a, b_, 0, c); }
};

struct AIFitterNC : AIFitter
{
    using AIFitter::add;
    void add(AIModelNC &m, double s, double e, double tank, double ms)
    {
        AIFitter::add(m, s, e, tank, 0, ms);
    }
};

// Production dropped its 3x3 helpers when the model widened to four coefficients; the historical
// hand-rolled fits below still need one.
static bool solve3(const double M[9], const double r[3], double x[3])
{
    double d = M[0] * (M[4] * M[8] - M[5] * M[7]) -
               M[1] * (M[3] * M[8] - M[5] * M[6]) +
               M[2] * (M[3] * M[7] - M[4] * M[6]);
    if (!isfinite(d) || fabs(d) < 1e-12)
        return false;
    double T[9];
    for (int col = 0; col < 3; col++)
    {
        for (int i = 0; i < 9; i++)
            T[i] = M[i];
        T[col] = r[0];
        T[col + 3] = r[1];
        T[col + 6] = r[2];
        double dt = T[0] * (T[4] * T[8] - T[5] * T[7]) -
                    T[1] * (T[3] * T[8] - T[5] * T[6]) +
                    T[2] * (T[3] * T[7] - T[4] * T[6]);
        x[col] = dt / d;
    }
    return isfinite(x[0]) && isfinite(x[1]) && isfinite(x[2]);
}

// Scores a fixed set of shipped (old-firmware) weights against a full dataset, and a fresh v2
// batch fit on the same data for reference. Used to check the user's in-car observation that the
// old model's predictions ran too long.
static void evalShippedWeights(const char *name, const EvalSample *ds, int len, bool up,
                               double w1, double w2, double b)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    OldModel shipped;
    shipped.up = up;
    shipped.w1 = w1;
    shipped.w2 = w2;
    shipped.b = b;
    Metrics s = score(all, [&](const EvalSample &x) { return shipped.predictDeNormalized(x.s, x.e, x.tank); });

    AIModelNC v2;
    v2.up = up;
    AIFitterNC fitter;
    for (const EvalSample &x : all)
        fitter.add(v2, x.s, x.e, x.tank, x.ms);
    if (!fitter.solveInto(v2))
        return;
    Metrics n = score(all, [&](const EvalSample &x) { return v2.predictDeNormalized(x.s, x.e, x.tank); });

    // overshoot shows up on small trim moves, so bucket the bias by move size
    std::vector<EvalSample> small, large;
    for (const EvalSample &x : all)
        (fabs(x.e - x.s) <= 10 ? small : large).push_back(x);
    Metrics sSmall = score(small, [&](const EvalSample &x) { return shipped.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics sLarge = score(large, [&](const EvalSample &x) { return shipped.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics nSmall = score(small, [&](const EvalSample &x) { return v2.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics nLarge = score(large, [&](const EvalSample &x) { return v2.predictDeNormalized(x.s, x.e, x.tank); });

    printf("%-24s shipped 7/19 weights: MAE %6.1f  RMSE %6.1f  bias %+7.1f ms  (<=10psi %+7.1f | >10psi %+7.1f)\n",
           name, s.mae, s.rmse, s.bias, sSmall.bias, sLarge.bias);
    printf("%-24s v2 fit on same data:  MAE %6.1f  RMSE %6.1f  bias %+7.1f ms  (<=10psi %+7.1f | >10psi %+7.1f)\n",
           "", n.mae, n.rmse, n.bias, nSmall.bias, nLarge.bias);
}

// ---------------------------------------------------------------------------
// Per-dataset evaluation
// ---------------------------------------------------------------------------

static void evalDataset(const char *name, const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
    {
        printf("%-24s skipped (%d usable samples after firmware filter)\n", name, (int)all.size());
        return;
    }

    // deterministic shuffle then split: 60% batch / 20% online stream / 20% holdout
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    int nStream = (int)all.size() / 5;
    int nBatch = (int)all.size() - nHold - nStream;
    std::vector<EvalSample> batch(all.begin(), all.begin() + nBatch);
    std::vector<EvalSample> stream(all.begin() + nBatch, all.begin() + nBatch + nStream);
    std::vector<EvalSample> hold(all.begin() + nBatch + nStream, all.end());
    std::vector<EvalSample> trainAll = batch;
    trainAll.insert(trainAll.end(), stream.begin(), stream.end()); // batch+stream = the 80% train split for A/B/C

    printf("%-24s %4d samples (train %d / holdout %d)  [times %d..%d ms]\n",
           name, (int)all.size(), (int)trainAll.size(), (int)hold.size(),
           (int)[&] { double m = 1e9; for (auto &x : all) m = m < x.ms ? m : x.ms; return m; }(),
           (int)[&] { double m = 0; for (auto &x : all) m = m > x.ms ? m : x.ms; return m; }());

    // A) old model, old SGD training (10k epochs, exactly like trainSingleAIModel did)
    OldModel oldSgd;
    oldSgd.up = up;
    for (int epoch = 0; epoch < 1000 * 10; epoch++)
        for (const EvalSample &x : trainAll)
            oldSgd.train(x.s, x.e, x.tank, x.ms);
    Metrics a = score(hold, [&](const EvalSample &x) { return oldSgd.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics aTrain = score(trainAll, [&](const EvalSample &x) { return oldSgd.predictDeNormalized(x.s, x.e, x.tank); });

    // B) old features, exact least squares (isolates training method from features)
    {
        double A[9] = {0}, v[3] = {0};
        OldModel ref;
        ref.up = up;
        for (const EvalSample &x : trainAll)
        {
            if (!ref.isSampleValid(x.s, x.e, x.tank))
                continue;
            double sn = x.s / 200.0, en = x.e / 200.0, tn = x.tank / 200.0;
            double f[3];
            if (up)
            {
                f[0] = log(tn / (tn - en));
                f[1] = en - sn;
            }
            else
            {
                f[0] = log(sn / en);
                f[1] = 0;
            }
            f[2] = 1;
            double y = x.ms / 5000.0;
            for (int r = 0; r < 3; r++)
            {
                for (int c = 0; c < 3; c++)
                    A[r * 3 + c] += f[r] * f[c];
                v[r] += f[r] * y;
            }
        }
        double ridge = ML_FIT_RIDGE * trainAll.size();
        A[0] += ridge;
        A[4] += ridge;
        A[8] += ridge;
        double w[3];
        if (solve3(A, v, w))
        {
            OldModel oldLs;
            oldLs.up = up;
            oldLs.w1 = w[0];
            oldLs.w2 = w[1];
            oldLs.b = w[2];
            Metrics bM = score(hold, [&](const EvalSample &x) { return oldLs.predictDeNormalized(x.s, x.e, x.tank); });
            printf("  A old feats + SGD 10k     holdout MAE %6.1f  RMSE %6.1f   (train RMSE %6.1f)\n", a.mae, a.rmse, aTrain.rmse);
            printf("  B old feats + exact LS    holdout MAE %6.1f  RMSE %6.1f\n", bM.mae, bM.rmse);
        }
    }

    // C) new physics features, exact least squares (the production v2 path)
    AIModelNC newModel;
    newModel.up = up;
    AIFitterNC fitter;
    for (const EvalSample &x : trainAll)
        fitter.add(newModel, x.s, x.e, x.tank, x.ms);
    if (!fitter.solveInto(newModel))
    {
        printf("  C new feats: solve FAILED (%d valid samples)\n", fitter.sampleCount());
        return;
    }
    Metrics c = score(hold, [&](const EvalSample &x) { return newModel.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics cTrain = score(trainAll, [&](const EvalSample &x) { return newModel.predictDeNormalized(x.s, x.e, x.tank); });
    printf("  C new feats + exact LS    holdout MAE %6.1f  RMSE %6.1f   (train RMSE %6.1f)  w=(%.4f, %.4f, %.4f)\n",
           c.mae, c.rmse, cTrain.rmse, newModel.w1, newModel.w2, newModel.b);

    // C2) single-feature variant (f1 forced to 0) - checks whether the second feature earns its keep
    AIModelNC single;
    single.up = up;
    {
        double A[9] = {0}, v[3] = {0};
        int ns = 0;
        for (const EvalSample &x : trainAll)
        {
            if (!single.isSampleValid(x.s, x.e, x.tank))
                continue;
            double f[3];
            single.computeFeatures(x.s, x.e, x.tank, f);
            f[1] = 0;
            double y = x.ms / 5000.0;
            for (int r = 0; r < 3; r++)
            {
                for (int cc = 0; cc < 3; cc++)
                    A[r * 3 + cc] += f[r] * f[cc];
                v[r] += f[r] * y;
            }
            ns++;
        }
        double ridge = ML_FIT_RIDGE * ns;
        A[0] += ridge;
        A[4] += ridge;
        A[8] += ridge;
        double w[3];
        if (solve3(A, v, w))
        {
            single.w1 = w[0];
            single.w2 = 0;
            single.b = w[2];
            Metrics c2 = score(hold, [&](const EvalSample &x) { return single.predictDeNormalized(x.s, x.e, x.tank); });
            printf("  C2 f0-only + exact LS     holdout MAE %6.1f  RMSE %6.1f   w=(%.4f, 0, %.4f)\n", c2.mae, c2.rmse, single.w1, single.b);
        }
    }

    // extrapolation sanity probes (predictions outside the training distribution)
    if (!up)
    {
        printf("  extrapolation (down):  100->10   50->5   30->5   (ms)\n");
        printf("    old SGD:            %7.0f %7.0f %7.0f\n",
               oldSgd.predictDeNormalized(100, 10, 150), oldSgd.predictDeNormalized(50, 5, 150), oldSgd.predictDeNormalized(30, 5, 150));
        printf("    new full:           %7.0f %7.0f %7.0f\n",
               newModel.predictDeNormalized(100, 10, 150), newModel.predictDeNormalized(50, 5, 150), newModel.predictDeNormalized(30, 5, 150));
        printf("    new f0-only:        %7.0f %7.0f %7.0f\n",
               single.predictDeNormalized(100, 10, 150), single.predictDeNormalized(50, 5, 150), single.predictDeNormalized(30, 5, 150));
    }
    else
    {
        printf("  extrapolation (up):    20->110@180   20->110@120   90->95@100   (ms)\n");
        printf("    old SGD:            %10.0f %13.0f %12.0f\n",
               oldSgd.predictDeNormalized(20, 110, 180), oldSgd.predictDeNormalized(20, 110, 120), oldSgd.predictDeNormalized(90, 95, 100));
        printf("    new full:           %10.0f %13.0f %12.0f\n",
               newModel.predictDeNormalized(20, 110, 180), newModel.predictDeNormalized(20, 110, 120), newModel.predictDeNormalized(90, 95, 100));
        printf("    new f0-only:        %10.0f %13.0f %12.0f\n",
               single.predictDeNormalized(20, 110, 180), single.predictDeNormalized(20, 110, 120), single.predictDeNormalized(90, 95, 100));
    }

    // D) production two-phase flow: batch fit on 60%, then stream 20% through RLS
    AIModelNC rlsModel;
    rlsModel.up = up;
    AIFitterNC batchFitter;
    for (const EvalSample &x : batch)
        batchFitter.add(rlsModel, x.s, x.e, x.tank, x.ms);
    if (batchFitter.solveInto(rlsModel))
    {
        batchFitter.initOnline(rlsModel);
        // seed the outlier gate from the batch residuals, exactly like trainSingleAIModel will
        double sumSqNorm = 0;
        int nRes = 0;
        for (const EvalSample &x : batch)
        {
            if (!rlsModel.isSampleValid(x.s, x.e, x.tank))
                continue;
            double d = (rlsModel.predictDeNormalized(x.s, x.e, x.tank) - x.ms) / ML_TIME_NORM_MS;
            sumSqNorm += d * d;
            nRes++;
        }
        double meanSqNorm = nRes ? sumSqNorm / nRes : 0;
        rlsModel.onlineSeedGate(meanSqNorm);
        Metrics before = score(hold, [&](const EvalSample &x) { return rlsModel.predictDeNormalized(x.s, x.e, x.tank); });
        int applied = 0;
        for (const EvalSample &x : stream)
            applied += rlsModel.trainOnline(x.s, x.e, x.tank, x.ms) ? 1 : 0;
        Metrics after = score(hold, [&](const EvalSample &x) { return rlsModel.predictDeNormalized(x.s, x.e, x.tank); });
        printf("  D batch60+RLS stream      holdout RMSE %6.1f -> %6.1f  (%d/%d updates applied)\n",
               before.rmse, after.rmse, applied, (int)stream.size());

        // corruption test: same stream but every 4th sample's time is multiplied by 5
        AIModelNC dirty;
        dirty.up = up;
        AIFitterNC dirtyFitter;
        for (const EvalSample &x : batch)
            dirtyFitter.add(dirty, x.s, x.e, x.tank, x.ms);
        dirtyFitter.solveInto(dirty);
        dirtyFitter.initOnline(dirty);
        dirty.onlineSeedGate(meanSqNorm);
        int i = 0, gatedCount = 0;
        for (const EvalSample &x : stream)
        {
            double ms = (i++ % 4 == 3) ? x.ms * 5 : x.ms;
            if (!dirty.trainOnline(x.s, x.e, x.tank, ms))
                gatedCount++;
        }
        Metrics corrupt = score(hold, [&](const EvalSample &x) { return dirty.predictDeNormalized(x.s, x.e, x.tank); });
        printf("  D' same + 25%% corrupted   holdout RMSE %6.1f  (%d samples rejected by gate)\n", corrupt.rmse, gatedCount);
    }

    // G) how many bootstrap samples does the batch fit actually need? Fit on the first N of the
    // (shuffled) train split and score the same holdout each time.
    {
        printf("  G batch fit size sweep    holdout RMSE:");
        int sizes[] = {50, 75, 100, 150, (int)trainAll.size()};
        for (int si = 0; si < 5; si++)
        {
            int N = sizes[si];
            if (N > (int)trainAll.size())
                continue;
            AIModelNC sub;
            sub.up = up;
            AIFitterNC subFit;
            for (int j = 0; j < N; j++)
                subFit.add(sub, trainAll[j].s, trainAll[j].e, trainAll[j].tank, trainAll[j].ms);
            if (!subFit.solveInto(sub))
                continue;
            Metrics m = score(hold, [&](const EvalSample &x) { return sub.predictDeNormalized(x.s, x.e, x.tank); });
            printf("  @%d %6.1f", N, m.rmse);
        }
        printf("\n");
    }

    // E) cold-start pure online: no batch fit, no stored pool - stream every train sample through
    // RLS from default weights and watch holdout RMSE converge
    {
        AIModelNC cold;
        cold.up = up;
        printf("  E cold-start RLS          holdout RMSE:");
        int seen = 0;
        for (const EvalSample &x : trainAll)
        {
            cold.trainOnline(x.s, x.e, x.tank, x.ms);
            seen++;
            if (seen == 10 || seen == 25 || seen == 50 || seen == 100 || seen == (int)trainAll.size())
            {
                Metrics m = score(hold, [&](const EvalSample &x2) { return cold.predictDeNormalized(x2.s, x2.e, x2.tank); });
                printf("  @%d %6.1f", seen, m.rmse);
            }
        }
        printf("\n");

        // F) forgetting test: after converging, feed only small trim moves (delta <= 10 psi) for a
        // simulated long stretch, with NO anchor replay, then re-score the big moves (delta >= 25 psi)
        std::vector<EvalSample> trims, bigHold;
        for (const EvalSample &x : trainAll)
            if (fabs(x.e - x.s) <= 10)
                trims.push_back(x);
        for (const EvalSample &x : hold)
            if (fabs(x.e - x.s) >= 25)
                bigHold.push_back(x);
        if (!trims.empty() && (int)bigHold.size() >= 5)
        {
            Metrics bigBefore = score(bigHold, [&](const EvalSample &x) { return cold.predictDeNormalized(x.s, x.e, x.tank); });
            int fed = 0;
            while (fed < 1000)
                for (const EvalSample &x : trims)
                {
                    cold.trainOnline(x.s, x.e, x.tank, x.ms);
                    if (++fed >= 1000)
                        break;
                }
            Metrics bigAfter = score(bigHold, [&](const EvalSample &x) { return cold.predictDeNormalized(x.s, x.e, x.tank); });
            printf("  F 1000 trim-only, no anchors: big-move (>=25psi) holdout RMSE %6.1f -> %6.1f  (%d big holdout samples)\n",
                   bigBefore.rmse, bigAfter.rmse, (int)bigHold.size());
        }
    }
    printf("\n");
}

// ---------------------------------------------------------------------------
// Relative-error weighting experiment
//
// The production fit minimizes squared error in *milliseconds*. Recorded times span 20..3000 ms,
// so a 200 ms miss on a 2500 ms move (8%) costs the same as a 200 ms miss on a 150 ms move (133%).
// The fit therefore buys accuracy on long moves by wrecking short ones. Weighting each sample by
// 1/t minimizes *relative* error instead, which is what the valve actually cares about.
// ---------------------------------------------------------------------------

// Fits w1/w2/b by weighted least squares. weightByTime scales each row by 1/t.
// forceZeroBias drops the intercept (physically, a zero-size move should take zero time).
// Gaussian elimination with partial pivoting for a k x k system, k <= 3. M and rhs are destroyed.
static bool solveK(double *M, double *rhs, double *out, int k)
{
    for (int col = 0; col < k; col++)
    {
        int piv = col;
        for (int r = col + 1; r < k; r++)
            if (fabs(M[r * k + col]) > fabs(M[piv * k + col]))
                piv = r;
        if (fabs(M[piv * k + col]) < 1e-12)
            return false;
        if (piv != col)
        {
            for (int c = 0; c < k; c++)
            {
                double t = M[col * k + c];
                M[col * k + c] = M[piv * k + c];
                M[piv * k + c] = t;
            }
            double t = rhs[col];
            rhs[col] = rhs[piv];
            rhs[piv] = t;
        }
        for (int r = col + 1; r < k; r++)
        {
            double factor = M[r * k + col] / M[col * k + col];
            for (int c = col; c < k; c++)
                M[r * k + c] -= factor * M[col * k + c];
            rhs[r] -= factor * rhs[col];
        }
    }
    for (int r = k - 1; r >= 0; r--)
    {
        double sum = rhs[r];
        for (int c = r + 1; c < k; c++)
            sum -= M[r * k + c] * out[c];
        out[r] = sum / M[r * k + r];
    }
    return true;
}

// Exact non-negative least squares for the 3-coefficient problem. At the constrained optimum the
// non-zero coefficients satisfy the unconstrained normal equations restricted to their own support,
// so enumerating all 8 supports and keeping the feasible one with the lowest residual is exact.
static void solveNonNegative(const double *A, const double *v, double *w)
{
    double bestObj = 0; // the all-zero solution is always feasible
    w[0] = w[1] = w[2] = 0;
    for (int mask = 1; mask < 8; mask++)
    {
        int idx[3], k = 0;
        for (int i = 0; i < 3; i++)
            if (mask & (1 << i))
                idx[k++] = i;
        double M[9], rhs[3], sol[3] = {0, 0, 0};
        for (int r = 0; r < k; r++)
        {
            rhs[r] = v[idx[r]];
            for (int c = 0; c < k; c++)
                M[r * k + c] = A[idx[r] * 3 + idx[c]];
        }
        if (!solveK(M, rhs, sol, k))
            continue;
        bool feasible = true;
        for (int r = 0; r < k; r++)
            if (sol[r] < 0)
                feasible = false;
        if (!feasible)
            continue;
        double cand[3] = {0, 0, 0};
        for (int r = 0; r < k; r++)
            cand[idx[r]] = sol[r];
        double obj = 0; // w'Aw - 2v'w, dropping the constant y'y
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
                obj += cand[r] * A[r * 3 + c] * cand[c];
            obj -= 2 * v[r] * cand[r];
        }
        if (obj < bestObj)
        {
            bestObj = obj;
            w[0] = cand[0];
            w[1] = cand[1];
            w[2] = cand[2];
        }
    }
}

static bool fitWeighted(AIModelNC &m, const std::vector<EvalSample> &set, bool weightByTime, bool forceZeroBias,
                        bool nonNeg = false, double dither = 0)
{
    double A[9] = {0}, v[3] = {0};
    int n = 0;
    for (const EvalSample &x : set)
    {
        double s = x.s, e = x.e;
        if (dither > 0)
        {
            // simulate the fractional psi the uint8_t learn file threw away
            s += dither * (2.0 * (nextRand() % 10000) / 10000.0 - 1.0);
            e += dither * (2.0 * (nextRand() % 10000) / 10000.0 - 1.0);
        }
        if (!m.isSampleValid(s, e, x.tank))
            continue;
        double f[3];
        m.computeFeatures(s, e, x.tank, f);
        double y = x.ms / ML_TIME_NORM_MS;
        if (forceZeroBias)
            f[2] = 0;
        if (weightByTime)
        {
            double w = ML_TIME_NORM_MS / (x.ms > 20 ? x.ms : 20);
            f[0] *= w;
            f[1] *= w;
            f[2] *= w;
            y *= w;
        }
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
                A[r * 3 + c] += f[r] * f[c];
            v[r] += f[r] * y;
        }
        n++;
    }
    if (n < ML_FIT_MIN_SAMPLES)
        return false;
    double ridge = ML_FIT_RIDGE * n;
    A[0] += ridge;
    A[4] += ridge;
    A[8] += ridge;
    double w[3];
    if (nonNeg)
    {
        solveNonNegative(A, v, w);
    }
    else if (!solve3(A, v, w))
    {
        return false;
    }
    m.w1 = w[0];
    m.w2 = w[1];
    m.b = forceZeroBias ? 0 : w[2];
    return true;
}

// Mean absolute percentage error - the metric that matches "did the valve open roughly the right
// fraction of the needed time", which is what causes visible overshoot/undershoot in the car.
static double mape(const std::vector<EvalSample> &set, AIModelNC &m)
{
    double sum = 0;
    int n = 0;
    for (const EvalSample &x : set)
    {
        double p = m.predictDeNormalized(x.s, x.e, x.tank);
        sum += fabs(p - x.ms) / (x.ms > 20 ? x.ms : 20);
        n++;
    }
    return n ? 100.0 * sum / n : 0;
}

static void reportVariant(const char *label, AIModelNC &m, const std::vector<EvalSample> &hold)
{
    std::vector<EvalSample> small, large;
    for (const EvalSample &x : hold)
        (fabs(x.e - x.s) <= 15 ? small : large).push_back(x);
    Metrics all = score(hold, [&](const EvalSample &x) { return m.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics sm = score(small, [&](const EvalSample &x) { return m.predictDeNormalized(x.s, x.e, x.tank); });
    Metrics lg = score(large, [&](const EvalSample &x) { return m.predictDeNormalized(x.s, x.e, x.tank); });
    printf("    %-22s RMSE %6.1f  MAPE %5.0f%%   bias <=15psi %+7.1f | >15psi %+7.1f   w=(%.4f, %.4f, %.4f)\n",
           label, all.rmse, mape(hold, m), sm.bias, lg.bias, m.w1, m.w2, m.b);
}

static void evalWeighting(const char *name, const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
        return;
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    std::vector<EvalSample> train(all.begin(), all.end() - nHold);
    std::vector<EvalSample> hold(all.end() - nHold, all.end());

    printf("  %s\n", name);
    AIModelNC a, b, c, d, e;
    a.up = b.up = c.up = d.up = e.up = up;
    if (fitWeighted(a, train, false, false))
        reportVariant("absolute error (now)", a, hold);
    if (fitWeighted(b, train, true, false))
        reportVariant("relative (1/t)", b, hold);
    if (fitWeighted(c, train, true, true))
        reportVariant("relative + no bias", c, hold);
    if (fitWeighted(d, train, false, false, true))
        reportVariant("absolute + nonneg", d, hold);
    if (fitWeighted(e, train, true, false, true))
        reportVariant("relative + nonneg", e, hold);
}

// Every logged pressure is a uint8_t, so the fractional psi is gone. Re-fit repeatedly with the
// lost fraction dithered back in and report how far the weights and a representative prediction
// wander - that spread is the accuracy the integer learn file is costing us.
static void evalQuantization(const char *name, const EvalSample *ds, int len, bool up, double ps, double pe)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
        return;
    const int trials = 40;
    double preds[trials], sum = 0;
    for (int t = 0; t < trials; t++)
    {
        rngState = 0x1234567 + t * 7919ull;
        AIModelNC m;
        m.up = up;
        if (!fitWeighted(m, all, false, false, false, 0.5))
        {
            return;
        }
        preds[t] = m.predictDeNormalized(ps, pe, 130);
        sum += preds[t];
    }
    double mean = sum / trials, var = 0;
    for (int t = 0; t < trials; t++)
        var += (preds[t] - mean) * (preds[t] - mean);
    double sd = sqrt(var / trials);
    printf("  %-22s %2.0f->%2.0f psi: prediction %6.0f ms, spread from lost fraction +-%.0f ms (%.1f%%)\n",
           name, ps, pe, mean, sd, mean > 1 ? 100.0 * sd / mean : 0);
}

// Runs the user's reported in-car moves through the weights actually on their device.
static void evalCarScenarios()
{
    struct Scenario
    {
        const char *label;
        bool up;
        double s, e, tank;
    };
    // corner weights read off the device serial log, in SOLENOID_AI_INDEX order
    const double W[4][3] = {
        {0.29317, 0.07423, 0.02297},   // up front
        {0.58444, -0.45446, -0.01635}, // up rear
        {0.35850, -0.22567, 0.10627},  // down front
        {0.29808, -0.09550, 0.01919},  // down rear
    };
    const Scenario scenarios[] = {
        {"air up   20 -> 75", true, 20, 75, 130},
        {"trim down 75 -> 55", false, 75, 55, 130},
        {"air out  75 -> 20", false, 75, 20, 130},
        {"tiny trim 75 -> 70", false, 75, 70, 130},
    };
    for (const Scenario &sc : scenarios)
    {
        printf("  %-20s", sc.label);
        for (int i = 0; i < 4; i++)
        {
            if ((i < 2) != sc.up)
                continue;
            AIModelNC m;
            m.up = sc.up;
            m.loadWeights(W[i][0], W[i][1], W[i][2]);
            printf("  model%d %6.0f ms", i, m.predictDeNormalized(sc.s, sc.e, sc.tank));
        }
        printf("\n");
    }
}

// Air leaving the tank to fill a bag drops the tank by (V_bag / V_tank) times the bag's rise, so the
// tank pressure actually driving flow across a pulse is lower than the value sampled before it, and
// the shortfall grows with move size - which is exactly a large-move under-prediction. k is that
// volume ratio. It is unknown per vehicle, so scan for it rather than assuming a number.
static bool featuresDroop(double s, double e, double tank, double k, double f[3])
{
    double tEff = tank - k * (e - s);
    if (!(tEff > s) || !(tEff > e))
        return false;
    f[0] = log((tEff - s) / (tEff - e));
    f[1] = (e - s) / (tEff + ML_PSI_ATMOSPHERE);
    f[2] = 1.0;
    return true;
}

static bool fitDroop(const std::vector<EvalSample> &set, double k, double *w)
{
    double A[9] = {0}, v[3] = {0};
    int n = 0;
    for (const EvalSample &x : set)
    {
        double f[3];
        if (!featuresDroop(x.s, x.e, x.tank, k, f))
            continue;
        double y = x.ms / ML_TIME_NORM_MS;
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
                A[r * 3 + c] += f[r] * f[c];
            v[r] += f[r] * y;
        }
        n++;
    }
    if (n < ML_FIT_MIN_SAMPLES)
        return false;
    double ridge = ML_FIT_RIDGE * n;
    A[0] += ridge;
    A[4] += ridge;
    A[8] += ridge;
    return solve3(A, v, w);
}

static void droopMetrics(const std::vector<EvalSample> &set, double k, const double *w,
                         double &rmse, double &biasSmall, double &biasLarge, int &dropped)
{
    double ss = 0, bs = 0, bl = 0;
    int n = 0, ns = 0, nl = 0;
    dropped = 0;
    for (const EvalSample &x : set)
    {
        double f[3];
        if (!featuresDroop(x.s, x.e, x.tank, k, f))
        {
            dropped++;
            continue;
        }
        double d = (w[0] * f[0] + w[1] * f[1] + w[2]) * ML_TIME_NORM_MS - x.ms;
        ss += d * d;
        n++;
        if (fabs(x.e - x.s) <= 15)
        {
            bs += d;
            ns++;
        }
        else
        {
            bl += d;
            nl++;
        }
    }
    rmse = n ? sqrt(ss / n) : 0;
    biasSmall = ns ? bs / ns : 0;
    biasLarge = nl ? bl / nl : 0;
}

static void evalTankDroop(const char *name, const EvalSample *ds, int len)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, true);
    if ((int)all.size() < 50)
        return;
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    std::vector<EvalSample> train(all.begin(), all.end() - nHold);
    std::vector<EvalSample> hold(all.end() - nHold, all.end());

    double bestK = 0, bestTrain = 1e18, bestW[3] = {0, 0, 0};
    for (int i = 0; i <= 100; i++)
    {
        double k = i * 0.005;
        double w[3];
        if (!fitDroop(train, k, w))
            continue;
        double rmse, bsm, blg;
        int dropped;
        droopMetrics(train, k, w, rmse, bsm, blg, dropped);
        // a k that only "wins" by making hard samples invalid is not a win
        if (dropped > (int)train.size() / 50)
            continue;
        if (rmse < bestTrain)
        {
            bestTrain = rmse;
            bestK = k;
            bestW[0] = w[0];
            bestW[1] = w[1];
            bestW[2] = w[2];
        }
    }

    double w0[3];
    if (!fitDroop(train, 0, w0))
        return;
    double r0, s0, l0, rk, sk, lk;
    int d0, dk;
    droopMetrics(hold, 0, w0, r0, s0, l0, d0);
    droopMetrics(hold, bestK, bestW, rk, sk, lk, dk);
    printf("  %-22s k=0.000  holdout RMSE %6.1f  bias <=15psi %+7.1f | >15psi %+7.1f\n",
           name, r0, s0, l0);
    printf("  %-22s k=%.3f  holdout RMSE %6.1f  bias <=15psi %+7.1f | >15psi %+7.1f  (dropped %d)\n",
           "", bestK, rk, sk, lk, dk);
}

// ---------------------------------------------------------------------------------------------
// Regime-split features.
//
// The shipped features both span the whole move: for a dump, f0 is the choked-flow log and f1 is
// the subsonic log, and BOTH are evaluated from start to end. That makes them near-duplicates of
// each other (they are both monotone in the same pressure ratio), so the fit can trade one against
// the other almost freely - which is where weights like (+0.81, -0.38) come from and why the model
// ends up flat.
//
// Physically the move is not two overlapping effects, it is two consecutive SEGMENTS. A dump is
// choked while the bag is above ~14.7 psi gauge (absolute above 2 atm) and subsonic below it. A fill
// is choked while the bag is below ~0.528x the tank's absolute pressure and subsonic above it. Split
// the move at that crossover and give each segment its own term and the two features stop
// overlapping: a 75->55 dump is pure choked, a 12->0 dump is pure subsonic.
// ---------------------------------------------------------------------------------------------
static const double PATM = ML_PSI_ATMOSPHERE;
static const double CHOKED_RATIO = 0.528; // downstream/upstream absolute ratio where flow unchokes

static bool featuresSplit(double s, double e, double tank, bool up, double f[3])
{
    f[2] = 1.0;
    if (up)
    {
        if (!(tank > s) || !(tank > e))
            return false;
        double crit = CHOKED_RATIO * (tank + PATM) - PATM; // bag gauge pressure where the fill unchokes
        // choked segment: constant mass flow set by the tank, so time is proportional to the
        // pressure covered divided by the tank's absolute pressure
        double chokedEnd = e < crit ? e : crit;
        f[0] = (s < crit) ? (chokedEnd - s) / (tank + PATM) : 0.0;
        // subsonic segment: exponential approach to tank pressure
        double subStart = s > crit ? s : crit;
        f[1] = (e > crit) ? log((tank - subStart) / (tank - e)) : 0.0;
    }
    else
    {
        const double crit = PATM; // gauge pressure where venting unchokes
        // choked segment: absolute pressure decays exponentially
        double chokedEnd = e > crit ? e : crit;
        f[0] = (s > crit) ? log((s + PATM) / (chokedEnd + PATM)) : 0.0;
        // subsonic tail: gauge pressure decays exponentially toward atmosphere
        double subStart = s < crit ? s : crit;
        double a = subStart < 1 ? 1 : subStart;
        double b = e < 1 ? 1 : e;
        f[1] = (e < crit) ? log(a / b) : 0.0;
    }
    return isfinite(f[0]) && isfinite(f[1]);
}

typedef bool (*FeatFn)(double, double, double, bool, double *);

static bool featuresShipped(double s, double e, double tank, bool up, double f[3])
{
    AIModelNC m;
    m.up = up;
    if (!m.isSampleValid(s, e, tank))
        return false;
    m.computeFeatures(s, e, tank, f);
    return true;
}

static bool fitFeat(const std::vector<EvalSample> &set, bool up, FeatFn ff, double *w)
{
    double A[9] = {0}, v[3] = {0};
    int n = 0;
    for (const EvalSample &x : set)
    {
        double f[3];
        if (!ff(x.s, x.e, x.tank, up, f))
            continue;
        double y = x.ms / ML_TIME_NORM_MS;
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
                A[r * 3 + c] += f[r] * f[c];
            v[r] += f[r] * y;
        }
        n++;
    }
    if (n < ML_FIT_MIN_SAMPLES)
        return false;
    double ridge = ML_FIT_RIDGE * n;
    A[0] += ridge;
    A[4] += ridge;
    A[8] += ridge;
    return solve3(A, v, w);
}

// Correlation between the two features - the collinearity that destabilises the fit.
static double featCorr(const std::vector<EvalSample> &set, bool up, FeatFn ff)
{
    double s0 = 0, s1 = 0, s00 = 0, s11 = 0, s01 = 0;
    int n = 0;
    for (const EvalSample &x : set)
    {
        double f[3];
        if (!ff(x.s, x.e, x.tank, up, f))
            continue;
        s0 += f[0];
        s1 += f[1];
        s00 += f[0] * f[0];
        s11 += f[1] * f[1];
        s01 += f[0] * f[1];
        n++;
    }
    if (n < 2)
        return 0;
    double c = s01 / n - (s0 / n) * (s1 / n);
    double v0 = s00 / n - (s0 / n) * (s0 / n);
    double v1 = s11 / n - (s1 / n) * (s1 / n);
    return (v0 > 1e-12 && v1 > 1e-12) ? c / sqrt(v0 * v1) : 0;
}

static void reportFeat(const char *label, const std::vector<EvalSample> &train,
                       const std::vector<EvalSample> &hold, bool up, FeatFn ff)
{
    double w[3];
    if (!fitFeat(train, up, ff, w))
        return;
    double ss = 0, bs = 0, bl = 0;
    int n = 0, ns = 0, nl = 0;
    for (const EvalSample &x : hold)
    {
        double f[3];
        if (!ff(x.s, x.e, x.tank, up, f))
            continue;
        double d = (w[0] * f[0] + w[1] * f[1] + w[2]) * ML_TIME_NORM_MS - x.ms;
        ss += d * d;
        n++;
        if (fabs(x.e - x.s) <= 15)
        {
            bs += d;
            ns++;
        }
        else
        {
            bl += d;
            nl++;
        }
    }
    printf("    %-16s RMSE %6.1f  bias <=15psi %+7.1f | >15psi %+7.1f  featcorr %+.3f  w=(%.4f, %.4f, %.4f)\n",
           label, n ? sqrt(ss / n) : 0, ns ? bs / ns : 0, nl ? bl / nl : 0,
           featCorr(train, up, ff), w[0], w[1], w[2]);
}

static void evalSplit(const char *name, const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
        return;
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    std::vector<EvalSample> train(all.begin(), all.end() - nHold);
    std::vector<EvalSample> hold(all.end() - nHold, all.end());
    printf("  %s\n", name);
    reportFeat("shipped", train, hold, up, featuresShipped);
    reportFeat("regime-split", train, hold, up, featuresSplit);
}

// ---------------------------------------------------------------------------------------------
// Does contention explain what the model is missing?
//
// Two tests. First: fit the shipped 3-parameter model, then group the holdout residuals by how many
// other corners were flowing. If the model is blind to a real contention effect, the residuals will
// trend with the count - pulses that ran alone finishing sooner than predicted, pulses that ran
// against three others finishing later. Second: refit with others_flowing as an actual fourth
// input and see whether holdout error drops enough to justify a 4x4 fitter and RLS covariance.
// ---------------------------------------------------------------------------------------------
static bool fitWithOthers(const std::vector<EvalSample> &set, bool up, double *w)
{
    double A[16] = {0}, v[4] = {0};
    int n = 0;
    for (const EvalSample &x : set)
    {
        double f3[3];
        if (!featuresShipped(x.s, x.e, x.tank, up, f3))
            continue;
        double f[4] = {f3[0], f3[1], x.others, 1.0};
        double y = x.ms / ML_TIME_NORM_MS;
        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < 4; c++)
                A[r * 4 + c] += f[r] * f[c];
            v[r] += f[r] * y;
        }
        n++;
    }
    if (n < ML_FIT_MIN_SAMPLES)
        return false;
    double ridge = ML_FIT_RIDGE * n;
    for (int i = 0; i < 4; i++)
        A[i * 4 + i] += ridge;
    double M[16], rhs[4];
    for (int i = 0; i < 16; i++)
        M[i] = A[i];
    for (int i = 0; i < 4; i++)
        rhs[i] = v[i];
    return solveK(M, rhs, w, 4);
}

static void evalContention(const char *name, const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
        return;
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    std::vector<EvalSample> train(all.begin(), all.end() - nHold);
    std::vector<EvalSample> hold(all.end() - nHold, all.end());

    double w3[3];
    if (!fitFeat(train, up, featuresShipped, w3))
        return;

    printf("  %s\n", name);
    // residual by contention bucket, over everything (more samples per bucket than holdout alone)
    for (int k = 0; k <= 3; k++)
    {
        double sum = 0;
        int n = 0;
        for (const EvalSample &x : all)
        {
            if ((int)x.others != k)
                continue;
            double f[3];
            if (!featuresShipped(x.s, x.e, x.tank, up, f))
                continue;
            sum += (w3[0] * f[0] + w3[1] * f[1] + w3[2]) * ML_TIME_NORM_MS - x.ms;
            n++;
        }
        if (n >= 5)
            printf("      others=%d  n=%3d  mean residual %+8.1f ms\n", k, n, sum / n);
    }

    double rmse3 = 0, rmse4 = 0;
    int n3 = 0, n4 = 0;
    for (const EvalSample &x : hold)
    {
        double f[3];
        if (!featuresShipped(x.s, x.e, x.tank, up, f))
            continue;
        double d = (w3[0] * f[0] + w3[1] * f[1] + w3[2]) * ML_TIME_NORM_MS - x.ms;
        rmse3 += d * d;
        n3++;
    }
    double w4[4];
    if (fitWithOthers(train, up, w4))
    {
        for (const EvalSample &x : hold)
        {
            double f[3];
            if (!featuresShipped(x.s, x.e, x.tank, up, f))
                continue;
            double d = (w4[0] * f[0] + w4[1] * f[1] + w4[2] * x.others + w4[3]) * ML_TIME_NORM_MS - x.ms;
            rmse4 += d * d;
            n4++;
        }
        printf("      holdout RMSE  3-param %6.1f   4-param (with others) %6.1f   others weight %+.4f\n",
               n3 ? sqrt(rmse3 / n3) : 0, n4 ? sqrt(rmse4 / n4) : 0, w4[2]);
    }
}

// ---------------------------------------------------------------------------------------------
// End-to-end check of the shipping code path with the real contention count: AIFitter -> solveInto
// -> initOnline -> onlineSeedGate -> trainOnline, exactly as trainSingleAIModel and
// processLearnSampleQueues run it. Holdout RMSE here should land on the 4-param numbers that
// evalContention derives independently; if it doesn't, solveN/invertN are wrong.
// ---------------------------------------------------------------------------------------------
static void evalProduction(const char *name, const EvalSample *ds, int len, bool up)
{
    std::vector<EvalSample> all = filterLikeFirmware(ds, len, up);
    if ((int)all.size() < 50)
        return;
    rngState = 0x9E3779B97F4A7C15ull;
    for (int i = (int)all.size() - 1; i > 0; i--)
    {
        int j = (int)(nextRand() % (uint64_t)(i + 1));
        EvalSample tmp = all[i];
        all[i] = all[j];
        all[j] = tmp;
    }
    int nHold = (int)all.size() / 5;
    std::vector<EvalSample> train(all.begin(), all.end() - nHold);
    std::vector<EvalSample> hold(all.end() - nHold, all.end());

    AIModel m;
    m.up = up;
    AIFitter fitter;
    for (const EvalSample &x : train)
        fitter.add(m, x.s, x.e, x.tank, x.others, x.ms);
    if (!fitter.solveInto(m))
    {
        printf("  %-16s solve FAILED (%d valid samples)\n", name, fitter.sampleCount());
        return;
    }

    // the rmse gate trainSingleAIModel applies to its own training data
    double sumSq = 0;
    int n = 0;
    for (const EvalSample &x : train)
    {
        if (!m.isSampleValid(x.s, x.e, x.tank))
            continue;
        double d = m.predictDeNormalized(x.s, x.e, x.tank, x.others) - x.ms;
        sumSq += d * d;
        n++;
    }
    double trainRmse = n ? sqrt(sumSq / n) : 0;
    Metrics h = score(hold, [&](const EvalSample &x) { return m.predictDeNormalized(x.s, x.e, x.tank, x.others); });

    bool covOk = fitter.initOnline(m);
    m.onlineSeedGate((sumSq / n) / (ML_TIME_NORM_MS * ML_TIME_NORM_MS));
    int applied = 0;
    for (const EvalSample &x : train)
        if (m.trainOnline(x.s, x.e, x.tank, x.others, x.ms))
            applied++;
    Metrics after = score(hold, [&](const EvalSample &x) { return m.predictDeNormalized(x.s, x.e, x.tank, x.others); });

    printf("  %-16s train RMSE %6.1f %s   holdout RMSE %6.1f  bias %+7.1f   after RLS %6.1f (%d/%d applied, cov %s)\n",
           name, trainRmse, trainRmse <= ML_BATCH_RMSE_GATE_MS ? "PASS" : "FAIL",
           h.rmse, h.bias, after.rmse, applied, (int)train.size(), covOk ? "ok" : "default");
    printf("  %-16s w1=%.5f w2=%.5f w3=%.5f b=%.5f   (contention costs %.0f ms per corner)\n",
           "", m.w1, m.w2, m.w3, m.b, m.w3 * ML_TIME_NORM_MS);
}

int main()
{
    printf("=== UP (fill) datasets ===\n");
    evalDataset("up_corvette_v1", ds_up_corvette_v1, ds_up_corvette_v1_len, true);
    evalDataset("up_front_v2", ds_up_front_v2, ds_up_front_v2_len, true);
    evalDataset("up_rear_v2", ds_up_rear_v2, ds_up_rear_v2_len, true);
    evalDataset("up_front_corvette_v3", ds_up_front_corvette_v3, ds_up_front_corvette_v3_len, true);
    evalDataset("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true);
    evalDataset("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true);

    printf("=== DOWN (dump) datasets ===\n");
    evalDataset("down_corvette_v1", ds_down_corvette_v1, ds_down_corvette_v1_len, false);
    evalDataset("down_rear_earl", ds_down_rear_earl, ds_down_rear_earl_len, false);
    evalDataset("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false);
    evalDataset("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false);

    // weights from 'ai weights for corvette 7.19.2026.txt', in SOLENOID_AI_INDEX order.
    // In-car report: predictions often ran too long -> expect positive bias here.
    printf("=== shipped 7/19/2026 weights vs the data they were trained on ===\n");
    evalShippedWeights("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true, 0.06000, 1.24298, 0.04448);
    evalShippedWeights("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true, 0.13275, 0.80219, -0.05428);
    evalShippedWeights("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false, 0.15206, 0.10000, 0.05793);
    evalShippedWeights("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false, 0.08166, 0.10000, 0.04473);

    printf("\n=== predictions from the weights currently on the device ===\n");
    evalCarScenarios();

    printf("\n=== absolute vs relative error weighting ===\n");
    evalWeighting("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true);
    evalWeighting("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true);
    evalWeighting("up_front_corvette_v3", ds_up_front_corvette_v3, ds_up_front_corvette_v3_len, true);
    evalWeighting("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false);
    evalWeighting("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false);
    evalWeighting("down_corvette_v1", ds_down_corvette_v1, ds_down_corvette_v1_len, false);

    printf("\n=== cost of storing pressures as whole psi (uint8_t) ===\n");
    evalQuantization("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true, 20, 75);
    evalQuantization("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true, 70, 75);
    evalQuantization("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true, 20, 75);
    evalQuantization("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true, 70, 75);
    evalQuantization("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false, 75, 20);
    evalQuantization("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false, 75, 70);
    evalQuantization("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false, 75, 20);
    evalQuantization("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false, 75, 70);

    printf("\n=== tank droop during the pulse (up models only) ===\n");
    evalTankDroop("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len);
    evalTankDroop("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len);
    evalTankDroop("up_front_corvette_v3", ds_up_front_corvette_v3, ds_up_front_corvette_v3_len);
    evalTankDroop("up_corvette_v1", ds_up_corvette_v1, ds_up_corvette_v1_len);
    evalTankDroop("up_front_v2", ds_up_front_v2, ds_up_front_v2_len);
    evalTankDroop("up_rear_v2", ds_up_rear_v2, ds_up_rear_v2_len);

    printf("\n=== overlapping vs regime-split features ===\n");
    evalSplit("down_front_car2026", ds_down_front_car2026, ds_down_front_car2026_len, false);
    evalSplit("down_rear_car2026", ds_down_rear_car2026, ds_down_rear_car2026_len, false);
    evalSplit("down_corvette_v1", ds_down_corvette_v1, ds_down_corvette_v1_len, false);
    evalSplit("down_rear_earl", ds_down_rear_earl, ds_down_rear_earl_len, false);
    evalSplit("up_front_car2026", ds_up_front_car2026, ds_up_front_car2026_len, true);
    evalSplit("up_rear_car2026", ds_up_rear_car2026, ds_up_rear_car2026_len, true);
    evalSplit("up_front_corvette_v3", ds_up_front_corvette_v3, ds_up_front_corvette_v3_len, true);
    evalSplit("up_corvette_v1", ds_up_corvette_v1, ds_up_corvette_v1_len, true);
    evalSplit("up_front_v2", ds_up_front_v2, ds_up_front_v2_len, true);
    evalSplit("up_rear_v2", ds_up_rear_v2, ds_up_rear_v2_len, true);

    printf("\n=== schema 3 data (7/30 test 2) vs the schema 2 data it replaced ===\n");
    evalWeighting("up_front OLD", ds_up_front_car2026, ds_up_front_car2026_len, true);
    evalWeighting("up_front NEW", ds_up_front_s3, ds_up_front_s3_len, true);
    evalWeighting("up_rear OLD", ds_up_rear_car2026, ds_up_rear_car2026_len, true);
    evalWeighting("up_rear NEW", ds_up_rear_s3, ds_up_rear_s3_len, true);
    evalWeighting("down_front OLD", ds_down_front_car2026, ds_down_front_car2026_len, false);
    evalWeighting("down_front NEW", ds_down_front_s3, ds_down_front_s3_len, false);
    evalWeighting("down_rear OLD", ds_down_rear_car2026, ds_down_rear_car2026_len, false);
    evalWeighting("down_rear NEW", ds_down_rear_s3, ds_down_rear_s3_len, false);

    printf("\n=== does others_flowing explain the residual? ===\n");
    evalContention("up_front NEW", ds_up_front_s3, ds_up_front_s3_len, true);
    evalContention("up_rear NEW", ds_up_rear_s3, ds_up_rear_s3_len, true);
    evalContention("down_front NEW", ds_down_front_s3, ds_down_front_s3_len, false);
    evalContention("down_rear NEW", ds_down_rear_s3, ds_down_rear_s3_len, false);

    printf("\n=== shipping code path, 4 coefficients, real contention counts ===\n");
    evalProduction("up_front", ds_up_front_s3, ds_up_front_s3_len, true);
    evalProduction("up_rear", ds_up_rear_s3, ds_up_rear_s3_len, true);
    evalProduction("down_front", ds_down_front_s3, ds_down_front_s3_len, false);
    evalProduction("down_rear", ds_down_rear_s3, ds_down_rear_s3_len, false);

    printf("\n=== regime-split on the new data ===\n");
    evalSplit("up_front NEW", ds_up_front_s3, ds_up_front_s3_len, true);
    evalSplit("up_rear NEW", ds_up_rear_s3, ds_up_rear_s3_len, true);
    evalSplit("down_front NEW", ds_down_front_s3, ds_down_front_s3_len, false);
    evalSplit("down_rear NEW", ds_down_rear_s3, ds_down_rear_s3_len, false);
    return 0;
}
