# AI Valve Timing — How It Works

The manifold learns, per vehicle, how long to open each solenoid to move a bag from one pressure
to another. This document describes the model (schema v2), the two-phase training pipeline,
the safety rails around it, and the PC evaluation harness used to validate changes offline.

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

Each model is a 3-parameter linear regression on physics-derived features. Inputs are raw gauge
psi; the output is valve-open time normalized by `ML_TIME_NORM_MS` (5000 ms).

### Fill (up) model

$$t = w_1 \ln\frac{P_{tank}-P_{start}}{P_{tank}-P_{end}} + w_2\frac{P_{end}-P_{start}}{P_{tank}+14.7} + b$$

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

$$t = w_1 \ln\frac{P_{start}+14.7}{P_{end}+14.7} + w_2 \ln\frac{\max(P_{start},1)}{\max(P_{end},1)} + b$$

Venting to atmosphere also has two regimes:

- **Choked** (bag above ~13 psi gauge): mass flow scales with absolute bag pressure, so absolute
  pressure decays exponentially toward zero absolute — the first (absolute-ratio) log.
- **Subsonic tail** (below ~13 psi gauge): decay is toward atmosphere (0 gauge) — the second
  (gauge-ratio) log, clamped at 1 psi since it diverges at exactly atmospheric.

Note: fitted data tends to give the tail a *negative* weight. That is not a bug — bag volume
shrinks as the suspension drops, so low-pressure dumps run faster than a fixed-volume model
predicts, and the fit uses the tail term to express that.

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

Each sample also records `others_flowing`: how many of the other three corners had a same-direction
valve open at the midpoint of the pulse. All four corners share one tank filling and one exhaust
dumping, so the same pressure move runs at a different rate alone than it does with three others
competing. **This field is logged but is not a model input** — it exists so we can measure whether
contention explains the residual bias (predictions long on moves ≤15 psi, short on moves >15 psi)
that survives every feature parameterization tried so far. Using it means widening the fitter and the
RLS covariance from 3×3 to 4×4, which is deliberately deferred until the data justifies it.

`recordLearnSample()` routes each sample by model state:

- **Not ready (bootstrap):** appended to that model's SPIFFS file and RAM array
  (`learnData[4][LEARN_SAVE_COUNT]`, 150 samples per model).
- **Ready (online):** pushed into a small ring queue (`ML_IMMEDIATE_TRAIN_SAMPLE_QUE` = 20 deep,
  drops oldest when full) consumed by the training task.

The bootstrap samples are **kept forever** after training — they are reused to rebuild online-learning
state at every boot and as anchor-replay material. They are only discarded when a fit fails its
quality gate, when the app sends a reset, or on a schema change.

---

## Phase 1 — batch training (closed-form least squares)

When a not-ready model has 150 samples, `trainSingleAIModel()` runs once (from `task_trainAI`):

1. `AIFitter` accumulates the normal equations ($X^TX$, $X^Ty$) over all valid samples in one pass.
2. A ridge-regularized 3×3 solve (Cramer's rule, `ML_FIT_RIDGE`) produces the **exact**
   least-squares weights. The model is linear in $w_1, w_2, b$, so no epochs and no learning rate
   are needed — this replaced the old 10,000-epoch SGD loop (~45 s → ~1 ms) and lands on the true
   optimum instead of orbiting it.
3. **Quality gate:** RMSE of the fit over its own training data must be ≤ `ML_BATCH_RMSE_GATE_MS`
   (400 ms). A model that can't fit its own data never gets valve control. On failure (or a
   singular/underdetermined solve) the model is simply left untrained and the corner keeps using
   lookup-table timing; the collected samples are kept. Training never deletes logged data — a
   sample good enough to log is good enough to keep. Fitting happens in a temp model so a rejected
   fit never leaves half-written weights in the live one.
4. On success: weights are copied to the live model and saved to NVS, the RLS covariance is
   initialized from the batch fit ($P = (X^TX + ridge)^{-1}$), the outlier gate is seeded with the
   batch residual noise floor, and `isReadyToUse` is set.

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
  slowly wash out what the bootstrap taught about big lifts (catastrophic forgetting, 3-parameter
  edition). Every `ML_ONLINE_ANCHOR_INTERVAL` (5th) online sample, one retained bootstrap sample is
  replayed through the same RLS update, cycling through the stored 150.
- Weights are saved to NVS once per queue drain (not per sample); `Preferencable::setDouble` also
  skips writes when the value hasn't changed, limiting flash wear.

The RLS state (`P`, gate EMA) is RAM-only. At boot, `trainAIModels()` rebuilds it for already-ready
models from the retained bootstrap samples (`initOnlineStateFromLearnData`); with no samples it
falls back to a conservative default covariance.

---

## Prediction and safety rails

`Wheel::goalRoutine()` asks the model for a pulse time via `getAiPredictionTime()` when
`canUseAiPrediction()` (AI enabled + model ready). The lookup-table timing is only a bootstrap
fallback; an accepted AI prediction replaces it directly. Two checks reject bad predictions:

1. **Validity:** invalid inputs (e.g. tank at/below goal pressure) return 0, rejected by the
   `aiPredict > 0` check → the corner falls back to the lookup-table timing.
   Predictions at or above 5000 ms are also rejected.
2. **NaN hygiene:** validity checks reject NaN inputs by construction, `trainOnline` refuses
   non-finite errors, and the solvers check determinants and finiteness — a non-finite value can
   never reach the weights (which would be unrecoverable once committed to NVS).

None of this touches the hard safety layer (`MAX_PRESSURE_SAFETY`, goal checks, timeouts) — the AI
only ever chooses *how long* to open a valve the routine already decided to open, and the routine
re-measures and re-decides every iteration.

---

## Schema migration (fielded devices)

`ML_MODEL_SCHEMA_VERSION` (in `pressureMath.h`) identifies both the feature set the stored weights
were trained with and the on-disk layout of `PressureLearnSaveStruct` — bump it for either. At boot,
`beginSaveData()` compares it against the `mlModelSchema` NVS value (devices
from before this mechanism default to 1). On mismatch it calls `clearPressureData()` — wiping
weights, ready flags, bootstrap sample files, and the reported AI percentage — then stores the new
version. The vehicle re-collects `LEARN_SAVE_COUNT` samples per model and falls back to lookup-table
timing until it does.

The check lives in `beginSaveData()` rather than in `loadAILearnedDataPreferences()` because
`clearPressureData()` calls the latter itself, which would recurse.

**Bump the schema version whenever `computeFeatures()` changes meaning.** Weights are not
transferable between feature sets.

---

## Tuning constants (all in `src/pressureMath.h`)

| Constant | Value | Meaning |
|---|---|---|
| `ML_MODEL_SCHEMA_VERSION` | 2 | Feature-set version stamped into NVS |
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

`LEARN_SAVE_COUNT` (150, bootstrap size) and `ML_IMMEDIATE_TRAIN_SAMPLE_QUE` (20, online queue
depth) live in `ESP32_SHARED_LIBS/src/user_defines.h`.

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
