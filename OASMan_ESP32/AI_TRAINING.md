# AI Valve Timing — How It Works

The manifold learns, per vehicle, how long to open each solenoid to move a bag from one pressure
to another. This document describes the model, the two-phase training pipeline, how the AI is
blended into valve timing as it trains, the safety rails around it, and the PC evaluation harness
used to validate changes offline.

Relevant code:

| File | Role |
|---|---|
| `src/pressureMath.h/.cpp` | The model (`AIModel`), batch fitter (`AIFitter`), online learner (RLS), all tuning constants |
| `src/airSuspensionUtil.cpp` (`#pragma region training`) | Batch training driver, quality gate, online queue processing |
| `src/manifoldSaveData.cpp` | Sample storage (SPIFFS + RAM), online sample queues, weight persistence (NVS), schema migration |
| `src/components/wheel.cpp` (`goalRoutine`) | Where samples are recorded and predictions are consumed |
| `src/tasks/tasks.cpp` (`task_trainAI`) | The task that runs batch training once, then loops the online learner |
| `eval/` | PC evaluation harness + historical datasets |

There are **4 models per vehicle**: up-front, up-rear, down-front, down-rear
(both corners of an axle share a model, since the bags match).

---

## The model

Each model is a 4-parameter linear regression (`ML_NUM_COEFF`) on physics-derived features. Inputs
are raw gauge psi plus a contention count; the output is valve-open time normalized by
`ML_TIME_NORM_MS` (5000 ms).

### Fill (up) model

$$t = w_1 \ln\frac{P_{tank}-P_{start}}{P_{tank}-P_{end}} + w_2\frac{P_{end}-P_{start}}{P_{tank}+14.7} + w_3 N_{others} + b$$

Filling a bag from a tank through an orifice has two regimes, and each feature covers one:

- **Subsonic** (bag close to tank pressure): flow is driven by the pressure difference, so the bag
  charges like an RC circuit toward tank pressure — time goes as the log-ratio of the remaining
  gaps. Gauge/absolute offsets cancel in the differences, so gauge psi is fine here.
- **Choked (sonic)** (bag below ~53% of absolute tank pressure): flow through the valve orifice is
  sonic and depends *only* on tank pressure, so the bag rises at a constant rate proportional to
  absolute tank pressure. Time goes as $\Delta p / P_{tank,abs}$. With a 165 psi tank most of a big
  lift happens in this regime.

Validity: requires $P_{tank} > P_{start}$ and $P_{tank} > P_{end}$, otherwise the log is inf/nan.
Invalid inputs make `predictDeNormalized()` return 0, which the caller treats as "no prediction"
(see Safety below). The checks are written with positive comparisons so NaN inputs also fail them.

### Dump (down) model

$$t = w_1 \ln\frac{P_{start}+14.7}{P_{end}+14.7} + w_2 \ln\frac{\max(P_{start},1)}{\max(P_{end},1)} + w_3 N_{others} + b$$

Venting to atmosphere also has two regimes:

- **Choked** (bag above ~13 psi gauge): mass flow scales with absolute bag pressure, so absolute
  pressure decays exponentially toward zero absolute — the first (absolute-ratio) log.
- **Subsonic tail** (below ~13 psi gauge): decay is toward atmosphere (0 gauge) — the second
  (gauge-ratio) log, clamped at 1 psi since it diverges at exactly atmospheric.

Note: fitted data tends to give the tail a *negative* weight. That is not a bug — bag volume
shrinks as the suspension drops, so low-pressure dumps run faster than a fixed-volume model
predicts, and the fit uses the tail term to express that.

### Contention ($N_{others}$, both models)

All four corners draw from one tank and vent through one exhaust, so an identical pressure move
takes materially longer when other corners are moving with it. $N_{others}$ is how many of the other
three are expected to have a same-direction valve open during this pulse.

This was the single largest source of error in the model. Grouping the residuals of the previous
three-parameter fit by contention gave a clean monotonic ladder — the model ran ~330 ms long on
front fills that happened alone and ~170 ms short on the same fills against three others. Adding
the term cut holdout RMSE on the 7/30 dataset by 12% (up front), 32% (up rear), 31% (down front)
and 49% (down rear), and collapsed the small-move/large-move bias split that had survived every
other feature parameterization tried. Fitted cost is roughly 240/305/155/110 ms per competing
corner for up front / up rear / down front / down rear.

**The count is an estimate of intent, not a measurement of state,** and it has to be: the model
predicts before opening its valve, and at that moment every thread is parked at the barrier with its
valve shut, so counting open valves would always return zero.

In pressure mode, each wheel thread decides its own pulse intent (`none` / `up` / `down`) from its
own reading, writes it to a shared slot, then hits one barrier. After the barrier every thread reads
the same table and counts how many *other* slots match its direction — that is `others_flowing`.
Corners that are done, timed out, or `onlyAirUp` and need to dump still publish `none` and take that
barrier before exiting, so mid-loop waiters cannot deadlock. Height-sensor mode skips this path
(AI is unused there).

A corner whose pulse turns out much shorter than ours stops competing partway through, so the
estimate can run high — but it is the *same* estimate at training time and at prediction time, which
is what actually matters. Later vetoes (`valveTime` driven to 0 by oscillation dividing, six-valve
`canOpen`) are intentionally not mirrored; they are rare and per-thread.

### Why these replaced the v1 features

The v1 fill feature was $\ln(P_{tank}/(P_{tank}-P_{end}))$ — i.e. the correct subsonic formula with
the start pressure hardwired to 0. It produced the same log value for a 70→90 fill and a 20→90
fill, and could predict *negative* times for near-tank fills. The v1 dump feature used gauge
ratios only (wrong regime for most real dumps) and needed a 1 psi clamp hack in prediction.

---

## Sample collection

`Wheel::goalRoutine()` records a sample after a valve pulse: start pressure, measured end pressure
(read after a 250 ms settle), pre-pulse tank pressure, and the commanded open time. Every pulse in a
goal routine is recorded, subject to pulses longer than 10 ms, moves bigger than 3 psi, and the
correct direction for the model (`manifoldSaveData.cpp` re-checks direction and a minimum 1 psi
change).

The tank pressure is read at the top of each routine iteration, 50 ms after the barrier releases all
four wheel threads together. That is the only point in a routine where no valve anywhere is open, so
it is the only place a tank reading reflects the tank rather than whatever was flowing out of it.
`Compressor::getTankPressure()` is deliberately *not* used here — it is a 5-sample mean that refreshes
only every 500 ms and is sampled regardless of valve state.

Each sample also records `others_flowing` from the shared intent table after the sync barrier —
see *Contention* above. It is the model's third input.

`recordLearnSample()` routes each sample by **stored sample count**, not by a readiness flag:

- **Below `LEARN_SAVE_COUNT` (bootstrap):** appended to that model's SPIFFS file and RAM array
  (`learnData[4]`, each row `LEARN_SAVE_COUNT` = 300 samples, heap-allocated).
- **At/above `LEARN_SAVE_COUNT` (online):** pushed into a small ring queue
  (`ML_IMMEDIATE_TRAIN_SAMPLE_QUE` = 20 deep, drops oldest when full) consumed by the training task.

The bootstrap samples are **kept** — they are reused to rebuild online-learning
state at every boot and as anchor-replay material. They are discarded when the app sends a reset, on a
sample-record change, or when a batch fit that had enough samples rejects them as unusable (see the
quality gate below).

The RAM array is heap-allocated the first time samples are loaded. Update/OTA mode returns from
`beginSaveData()` before that load, so the ~14 KB is never held during an OTA.

---

## Phase 1 — batch training (closed-form least squares)

Every model is batch-fit **at boot** from whatever samples it has stored (`trainAIModels()` in
`task_trainAI`), not only once a full bootstrap set is collected — the closed-form solve is ~1 ms, so
there is no reason to wait. This lets the AI phase into valve timing measurably long before the model
is "done" (see *Blended prediction* below). Two exceptions preserve accumulated online learning: a
model that has already reached `LEARN_SAVE_COUNT` samples **and** trained once is in online mode, so
boot keeps its NVS weights (which carry the RLS drift) and only rebuilds the RAM-only RLS state. A
model still collecting is (re)fit from its current samples on every boot — no online drift exists below
`LEARN_SAVE_COUNT`, so nothing is lost.

`trainSingleAIModel()`:

1. `AIFitter` accumulates the normal equations ($X^TX$, $X^Ty$) over all valid samples in one pass.
2. A ridge-regularized 4×4 solve (Gaussian elimination with partial pivoting, `ML_FIT_RIDGE`)
   produces the **exact** least-squares weights. The model is linear in $w_1, w_2, w_3, b$, so no
   epochs and no learning rate are needed — this replaced the old 10,000-epoch SGD loop (~45 s →
   ~1 ms) and lands on the true optimum instead of orbiting it. Pivoting matters here: the two
   physics features correlate around +0.95, and Cramer's rule on that is numerically fragile.
3. **Quality gate:** RMSE of the fit over its own training data must be ≤ `ML_BATCH_RMSE_GATE_MS`
   (400 ms). A model that can't fit its own data never gets valve control. When a fit that had at
   least `ML_FIT_MIN_SAMPLES` valid samples is rejected — RMSE over the gate, or a singular/degenerate
   solve — the samples are treated as **unusable and wiped** (`clearPressureDataSingle`), and the
   corner re-collects from scratch on lookup-table timing. A solve skipped only because there are
   fewer than `ML_FIT_MIN_SAMPLES` samples is *not* bad data — those samples are kept and the model
   keeps collecting (this matters because every model is re-fit at boot, often on a partial set).
   Fitting happens in a temp model so a rejected fit never leaves half-written weights in the live
   one. (Earlier firmware kept rejected samples; that was reversed — unusable data is now discarded.)
4. On success: weights are copied to the live model and saved to NVS, the RLS covariance is
   initialized from the batch fit ($P = (X^TX + ridge)^{-1}$), the outlier gate is seeded with the
   batch residual noise floor, and the model's **`trainedSampleCount`** is set to the number of valid
   samples the fit used (persisted in NVS). A failed/rejected fit sets it to 0. This count — not a
   boolean ready flag — is what gates prediction and drives the blend below.

## Phase 2 — online learning (RLS)

Once ready, every new sample updates the model immediately. `processLearnSampleQueues()` (looped by
`task_trainAI` every 100 ms) drains each model's queue and calls `AIModel::trainOnline()` per sample:

- **Recursive least squares with forgetting factor** `ML_RLS_FORGETTING` (0.995): each update is the
  exact least-squares solution over an exponentially weighted window (~200 samples of memory), one
  ~30-flop step per sample. No learning rate to tune; update size is governed by the covariance
  matrix `P`, which starts small (confident) after a 150-sample batch fit and grows if the data
  starts disagreeing with the model. This lets the model track slow drift: temperature, load,
  small leaks.
- **Outlier gate:** a running mean of squared prediction error is kept; once armed, samples whose
  error exceeds `ML_OUTLIER_GATE_FACTOR` (9×, i.e. ~3σ) are skipped. The gate is seeded from batch
  residuals so it is armed from the very first online sample, and a gated sample's contribution to
  the running mean is clipped so a burst of garbage can't talk its way past the gate. One bump or
  sensor glitch cannot yank the weights.
- **Anchor replay:** day-to-day driving is mostly small trim moves, and the forgetting window would
  slowly wash out what the bootstrap taught about big lifts (catastrophic forgetting, small-model
  edition). Every `ML_ONLINE_ANCHOR_INTERVAL` (5th) online sample, one retained bootstrap sample is
  replayed through the same RLS update, cycling through the stored 150.
- Weights are saved to NVS once per queue drain (not per sample); `Preferencable::setDouble` also
  skips writes when the value hasn't changed, limiting flash wear.

The RLS state (`P`, gate EMA) is RAM-only. At boot, `trainAIModels()` rebuilds it for online models
(those at `LEARN_SAVE_COUNT` samples that have trained) from the retained bootstrap samples
(`initOnlineStateFromLearnData`); with no samples it falls back to a conservative default covariance.
Online RLS only runs once a model has reached `LEARN_SAVE_COUNT` — below that, samples go to the
bootstrap file and the model is re-batch-fit at each boot instead.

---

## Prediction and safety rails

`Wheel::goalRoutine()` asks the model for a pulse time via `getAiPredictionTime()` when
`canUseAiPrediction()` (AI enabled + `trainedSampleCount > 0`). Two checks reject bad predictions:

1. **Validity:** invalid inputs (e.g. tank at/below goal pressure) return 0, rejected by the
   `aiPredict > 0` check → the corner falls back to the lookup-table timing.
   Predictions at or above 5000 ms are also rejected.
2. **NaN hygiene:** validity checks reject NaN inputs by construction, `trainOnline` refuses
   non-finite errors, and the solvers check determinants and finiteness — a non-finite value can
   never reach the weights (which would be unrecoverable once committed to NVS).

### Bootstrap timing (the non-AI term)

The fallback timing — used before a vehicle-specific model has trained, and as the $(1-w)$ term of the
blend below — is **not** a crude lookup table. `getValveTimingSimpleFit()` predicts from a set of
**default pre-trained physics models**: the same 4-parameter model as the learner, one per corner, with
hard-coded weights taken from a reference vehicle (`initDefaultAIModels()` /
`getDefaultModelPredictionTime()`). Only if those weights produce an out-of-range value (≤0 or ≥5000 ms,
e.g. tank at/below goal on a fill) does it fall back to the old linear estimate (~ $y = 10x$) as a
safety net. So a brand-new vehicle already gets physics-shaped timing on its very first move.

### Blended prediction

The AI does not switch on all-at-once. An accepted prediction is **blended** with the bootstrap
(default-model) time in proportion to how much data the model has been trained on, so improvement is
visible and measurable well before a model is "done":

$$t = t_{AI}\,w + t_{default}\,(1-w), \qquad w = \frac{\min(n,\ \texttt{AI\_LEARN\_RATIO\_NUM})}{\texttt{AI\_LEARN\_RATIO\_NUM}}$$

where $n$ is the model's `trainedSampleCount` (`getAiBlendWeight()`). At $n=0$ the corner is pure
default model (and `canUseAiPrediction()` is false); the AI ramps in linearly as $n$ grows; at
$n \ge$ `AI_LEARN_RATIO_NUM` (150) the table is no longer consulted. Because $w$ uses the count the
model was *trained from* (fixed until the next boot re-fit), not the live stored count, the AI is
never weighted beyond what it has actually learned. The 4-parameter fit needs `ML_FIT_MIN_SAMPLES`
(25) valid samples and must clear the RMSE gate before it trains at all, so blending begins there,
not at the very first sample.

Collection continues to `LEARN_SAVE_COUNT` (300) after the blend reaches full AI at 150 — the extra
samples keep improving the batch fit (re-run each boot) until the model transitions to online RLS.

None of this touches the hard safety layer (`MAX_PRESSURE_SAFETY`, goal checks, timeouts) — the AI
only ever chooses *how long* to open a valve the routine already decided to open, and the routine
re-measures and re-decides every iteration.

---

## Schema migration (fielded devices)

Two version numbers in `pressureMath.h`, checked at boot in `beginSaveData()`:

| Key | Constant | On mismatch |
|---|---|---|
| `mlModelSchema` | `ML_MODEL_SCHEMA_VERSION` | `clearAIWeightsOnly()` — drop weights + trained counts, **keep samples**, refit this boot |
| `mlSampleRec` | `ML_SAMPLE_RECORD_VERSION` | `clearPressureData()` — wipe samples and weights; vehicle re-collects |

Feature / algorithm OTAs bump the schema version so fielded devices retrain automatically. Users
already have a button for `clearPressureData`, but there is no “retrain weights” control — the
schema path is that control. Layout bumps (struct size / field order) bump the sample-record
version and wipe, because old files can’t be parsed.

Devices predating these keys default schema to 1. The sample-record default is seeded from the
schema (`>= 3` → record 2) so a device that already wrote the current layout is not told to wipe.

History: schema/record 3 added `others_flowing` + per-pulse tank reads; schema 4 made
`others_flowing` a model input (record stayed 2); schema 5 blended the AI into valve timing by
trained-sample count, replaced the persisted `isReadyToUse` bool with `trainedSampleCount`
(NVS key `model<i>|n`), routed collection by stored count, and raised `LEARN_SAVE_COUNT` to 300
(record stayed 2 — on-disk layout unchanged). The old `model<i>|r` bool keys are left orphaned in
NVS on upgrade; they are harmless.

---

## Tuning constants (all in `src/pressureMath.h`)

| Constant | Value | Meaning |
|---|---|---|
| `ML_MODEL_SCHEMA_VERSION` | 5 | Feature-set version (weights-only clear + refit) |
| `ML_SAMPLE_RECORD_VERSION` | 2 | Sample-file layout version (full wipe) |
| `ML_TIME_NORM_MS` | 5000 | Time normalization scale |
| `ML_PSI_ATMOSPHERE` | 14.7 | Gauge→absolute offset |
| `ML_FIT_RIDGE` | 0.001 | Per-sample ridge regularization for the batch solve |
| `ML_FIT_MIN_SAMPLES` | 25 | Minimum valid samples for a trustworthy fit |
| `ML_RLS_FORGETTING` | 0.995 | RLS forgetting factor (~200-sample memory) |
| `ML_RLS_DEFAULT_PRIOR` | 10.0 | Default P diagonal when no bootstrap data exists |
| `ML_OUTLIER_GATE_FACTOR` | 9.0 | Reject samples with err² > 9× running mean (~3σ) |
| `ML_OUTLIER_GATE_WARMUP` | 20 | Gate arming threshold when not seeded from a batch fit |
| `ML_ONLINE_ANCHOR_INTERVAL` | 5 | Replay one bootstrap sample per N online samples |
| `ML_BATCH_RMSE_GATE_MS` | 400 | Max self-fit RMSE allowed before `setReady` |

`LEARN_SAVE_COUNT` (300, bootstrap size / online-switch threshold), `AI_LEARN_RATIO_NUM` (150,
sample count at which the AI is trusted 100% and the blend reaches pure AI), and
`ML_IMMEDIATE_TRAIN_SAMPLE_QUE` (20, online queue depth) live in
`ESP32_SHARED_LIBS/src/user_defines.h`.

If a noisy vehicle ever loops on collect → gate-reject → clear, `ML_BATCH_RMSE_GATE_MS` is the
knob to loosen (the worst observed real-vehicle fit was ~250 ms RMSE).

---

## PC evaluation harness (`eval/`)

Model changes are validated **offline** against real recorded data before anything is flashed:

```bash
cd OASMan_ESP32/eval
g++ -O2 -o model_eval model_eval.cpp && ./model_eval
```

`model_eval.cpp` defines `test_run` and compiles the production `src/pressureMath.cpp` directly, so
it always tests the shipping math. `datasets.h` holds six historical datasets (~1,970 samples from
Corvette front/rear up/down and a tester's down data), regenerated by `extract_datasets.py`. For
each dataset the harness applies the firmware's recording filter, splits 60% batch / 20% online
stream / 20% holdout, and reports:

- **A** old (v1) features + the old 10k-epoch SGD — the pre-v2 baseline
- **B** old features + exact least squares — isolates the training method
- **C** new features + exact least squares — the production batch path (plus a single-feature
  variant and out-of-distribution extrapolation probes)
- **D / D'** batch fit + RLS streaming, clean and with 25% corrupted samples — verifies online
  stability and the outlier gate

Results that justified schema v2 (holdout RMSE, ms):

| Dataset | A: v1 + SGD | C: v2 + exact LS |
|---|---|---|
| up_front_corvette_v3 | 55.4 | **43.7** |
| up_rear_v2 | 63.1 | **63.0** (MAE −7%) |
| up_corvette_v1 | 120.2 | **114.6** |
| down_corvette_v1 | 265.4 | **226.4** |

Extrapolation probes: v1 predicted **−149 ms** for a 90→95 psi fill on a 100 psi tank; v2 predicts
456 ms. With 25% of the online stream corrupted (times ×5), the seeded gate rejects the bad samples
and holdout RMSE stays flat (e.g. 43.7 → 43.7 on v3 data) where an ungated learner degraded to 145.

When adding new recorded datasets, drop them into `datasets.h` (or extend the extractor) and add an
`evalDataset(...)` call in `main()` — more vehicles' data makes the go/no-go more trustworthy.
