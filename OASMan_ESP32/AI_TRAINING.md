# AI Pressure Correction — How It Works

The manifold controls bag pressure **closed-loop**: it holds a valve open and re-reads the sensor each
tick, closing when the bag reaches the goal — like height-sensor mode. The catch: **you can't read a
bag's true pressure while its valve is open.** Air flowing past the manifold-mounted sensor makes the bag
read high on fill / low on dump; the reading only settles to the truth once the valve closes.

So the learned model is a **sensor-offset corrector**:

    actualBag = rawBag + offset,   offset = f(flowDifferential, othersOpen)

The controller feeds the live flowing readings through `f` to recover the true pressure and stops at goal.

## The ML layer is deliberately tiny: samples → refit → predict → fade

There is **no online learner, no persisted weights, no schema/versioned model, no outlier gate, no anchor
replay**. A batch least-squares fit is ~1 ms and every sample is kept, so "train" and "keep training" are
the same operation — **re-fit** — and weights are re-derived from the samples on every boot.

| File | Role |
|---|---|
| `src/pressureMath.*` | `OffsetModel`: `computeFeatures`, `predict`, `refit` (ridge least squares) + `solveN` |
| `src/manifoldSaveData.*` | sample files on SPIFFS (`learnData`, `recordLearnSample`), the 4 RAM-only models |
| `src/airSuspensionUtil.cpp` | `refitModel`/`trainOffsetModels`, `getPredictionOffset` (fade), `getPredictedBagPressure` |
| `src/components/wheel.cpp` | closed-loop `goalRoutine`; logs samples on close (auto + manual) |
| `src/tasks/tasks.cpp` | `task_trainAI`: refit all at boot, then refit changed models every 100 ms |

There are **4 models** — up-front, up-rear, down-front, down-rear (both corners of an axle share).

### The model

`OffsetModel` is a 4-coefficient linear fit of the offset (psi). Features (`computeFeatures`, scaled by
100 so the squared term stays well-conditioned): `[ differential/100, (differential/100)², othersOpen, 1 ]`,
where differential = `rawTank − rawBag` (fill) or `rawBag` (dump). `predict` = `w·f × ML_OFFSET_NORM`.
The offset vanishes as flow stops (`rawBag → rawTank` on fill, `→ 0` on dump), so the estimate converges
to the truth right where the controller needs to stop; and because it's trained on
flowing-before-close → settled-after-close, it inherently absorbs the valve-close overshoot.

`refit` accumulates the normal equations over all samples, adds ridge, and solves (Gaussian elimination).
Below `ML_FIT_MIN_SAMPLES` (25) valid samples it leaves the weights alone.

### The fade / default

There is **no physics-default formula**. Before a model is trained, `getPredictionOffset` returns a flat constant —
**−5 psi on air-up, +5 psi on air-out** (`OFFSET_DEFAULT_PSI`). As samples accumulate it fades the trained
model in:

    w = clamp((count − OFFSET_FADE_MIN) / (AI_LEARN_RATIO_NUM − OFFSET_FADE_MIN), 0, 1)
    offset = default × (1 − w) + trained × w

Below `OFFSET_FADE_MIN` (25) → pure default; at `AI_LEARN_RATIO_NUM` (150) → pure trained. When a real
trained model exists the constant is fully faded out.

### Sample collection

Every valve close logs one sample: the last **flowing** readings while the valve was open, paired with the
**settled** bag reading after close — `{ raw_bag, settled_bag, raw_tank, others_open }` (offset =
`settled_bag − raw_bag`). Two triggers, one sink (`recordLearnSample` → SPIFFS + RAM):

- **Preset / maintain moves** — `goalRoutine` logs on close (after `OFFSET_SAMPLE_SETTLE_MS`).
- **Manual moves** (BLE valveControlBittset / gamepad) — `Wheel::captureManualOffsetSample`
  (`LOG_MANUAL_OFFSET_SAMPLES`) watches this corner's valve state each `Wheel::loop` tick, caching the
  flowing readings while open and logging on close.

`othersOpen` (contention) is a **live measurement**, not a prediction — count of other same-direction
valves currently open (`countOthersOpenSameDirection` via `Solenoid::isOpen()`). Closes happen near goal,
so samples concentrate where stopping accuracy matters. The `BEGIN/END IMPORTANT DATA FOR PRO` serial dump
prints every stored sample for offline analysis.

### Training

`task_trainAI` calls `trainOffsetModels()` at boot (refits all 4 from their loaded samples) then every
100 ms (refits only a model whose sample count changed). Fast conversions help: `initializeADS()` sets the
ADS1115 to `RATE_ADS1115_860SPS`.

## The closed-loop controller (`Wheel::goalRoutine`)

Per corner: commit a direction from the raw reading; each tick read the flowing bag + tank, count
`othersOpen`, compute `actual = getPredictedBagPressure(...)`, hold the valve open; stop and close within
`PRESSURE_DEADBAND_PSI` (2) of goal — or immediately on the in-loop `getbagMaxPressure()` /
`MAX_PRESSURE_SAFETY` ceiling (fill), the 10 s `ROUTINE_TIMEOUT_MS`, or an `onlyAirUp` block. Height-sensor
mode uses the same loop with the level sensor and no offset.

## Persistence / migration

Only the **sample files** are stored (SPIFFS); weights are never persisted. One version,
`ML_SAMPLE_RECORD_VERSION` (`pressureMath.h`): if the stored `mlSampleRec` differs, `clearPressureData`
wipes the sample files (`beginSaveData`). No model/feature version — a feature change just takes effect on
the next refit.

## Tuning constants

`pressureMath.h`: `ML_OFFSET_NORM` (100), `ML_FIT_RIDGE`, `ML_FIT_MIN_SAMPLES` (25), `ML_NUM_COEFF` (4),
`ML_SAMPLE_RECORD_VERSION`. `user_defines.h`: `LEARN_SAVE_COUNT` (300), `OFFSET_DEFAULT_PSI` (5),
`OFFSET_FADE_MIN` (25), `AI_LEARN_RATIO_NUM` (150), `PRESSURE_DEADBAND_PSI` (2), `OFFSET_SAMPLE_SETTLE_MS`
(250), `LOG_MANUAL_OFFSET_SAMPLES`.

Note: `eval/model_eval.cpp` targeted the old online-learning model and is stale until ported.
