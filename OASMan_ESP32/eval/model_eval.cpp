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

    AIModel v2;
    v2.up = up;
    AIFitter fitter;
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
    AIModel newModel;
    newModel.up = up;
    AIFitter fitter;
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
    AIModel single;
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
    AIModel rlsModel;
    rlsModel.up = up;
    AIFitter batchFitter;
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
        AIModel dirty;
        dirty.up = up;
        AIFitter dirtyFitter;
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
            AIModel sub;
            sub.up = up;
            AIFitter subFit;
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
        AIModel cold;
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
    return 0;
}
