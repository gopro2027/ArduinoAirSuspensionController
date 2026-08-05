# AI Pressure Correction — How It Works

The manifold controls bag pressure **closed-loop**: it holds a valve open and re-reads the sensor each
tick, closing when the bag reaches the goal — like height-sensor mode. The catch: **you can't read a
bag's true pressure while its valve is open.** Air flowing past the manifold-mounted sensor makes the bag
read high on fill / low on dump; the reading only settles to the truth once the valve closes.

So the learned model is a **sensor-offset corrector**:

    actualBag = rawBag + offset,   offset = f(flowDifferential)

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

`OffsetModel` is a 3-coefficient linear fit of the offset (psi). Features (`computeFeatures`, scaled by
100 so the squared term stays well-conditioned): `[ differential/100, (differential/100)², 1 ]`,
where differential = `rawTank − rawBag` (fill) or `rawBag` (dump). `predict` = `w·f × ML_OFFSET_NORM`.
(A prior `othersOpen`/contention feature was removed — tank sag from other open valves already shows up in
`rawTank`, which the differential captures, so it added ~0 to the fit on real data.)
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
**settled** bag reading after close — `{ raw_bag, settled_bag, raw_tank }` (offset =
`settled_bag − raw_bag`). Two triggers, one sink (`recordLearnSample` → SPIFFS + RAM):

- **Preset / maintain moves** — `goalRoutine` logs on close (after the direction's settle time).
- **Manual moves** (BLE valveControlBittset / gamepad) — `Wheel::captureManualOffsetSample`
  (`LOG_MANUAL_OFFSET_SAMPLES`) watches this corner's valve state each `Wheel::loop` tick, caching the
  flowing readings while open and logging after the settle on close.

Closes happen near goal, so samples concentrate where stopping accuracy matters. The
`BEGIN/END IMPORTANT DATA FOR PRO` serial dump prints every stored sample for offline analysis.

### Training

`task_trainAI` calls `trainOffsetModels()` at boot (refits all 4 from their loaded samples) then every
100 ms (refits only a model whose sample count changed). Fast conversions help: `initializeADS()` sets the
ADS1115 to `RATE_ADS1115_860SPS`.

## The closed-loop controller (`Wheel::goalRoutine`)

**Both modes share one loop** — the only difference is the predictor seam. Per corner: commit a direction
from the raw reading; each tick read the live value and compute `actual` (pressure mode:
`getPredictedBagPressure(...)` = raw + learned offset; height mode: `getPredictedBagHeight(...)`, an
identity stub since the level sensor reads true even during flow), hold the valve open; when `actual`
reaches goal, close and **verify**: wait for the bag to settle (`OFFSET_SAMPLE_SETTLE_MS` air-up /
`OFFSET_SAMPLE_SETTLE_DOWN_MS` air-out — venting settles slower), read the true settled value, then drop
the committed direction and re-run the top-of-loop check against that reading. This makes the verify
**bidirectional**: it finishes within the deadband of goal (pressure `PRESSURE_DEADBAND_PSI` = 1; height
`getMinValveOpenPSI()` = 0), keeps going the same way if it fell short, or reverses if it overshot — all
inside one `goalRoutine` call. While settling, the corner keeps ticking the sync barrier with its valve
closed, so it never stalls or overshoots the other three corners. Mode-specific bits: pressure mode logs a
flowing→settled sample on each close and enforces the in-loop `getbagMaxPressure()` / `MAX_PRESSURE_SAFETY`
ceiling (fill); height mode does neither (no model, and no in-loop pressure ceiling). Other hard stops: the
10 s `ROUTINE_TIMEOUT_MS`, or an `onlyAirUp` block (which accepts an overshoot rather than venting).

## Persistence / migration

Only the **sample files** are stored (SPIFFS); weights are never persisted. One version,
`ML_SAMPLE_RECORD_VERSION` (`pressureMath.h`): if the stored `mlSampleRec` differs, `clearPressureData`
wipes the sample files (`beginSaveData`). No model/feature version — a feature change just takes effect on
the next refit.

## Tuning constants

`pressureMath.h`: `ML_OFFSET_NORM` (100), `ML_FIT_RIDGE`, `ML_FIT_MIN_SAMPLES` (25), `ML_NUM_COEFF` (3),
`ML_SAMPLE_RECORD_VERSION`. `user_defines.h`: `LEARN_SAVE_COUNT` (300), `OFFSET_DEFAULT_PSI` (5),
`OFFSET_FADE_MIN` (25), `AI_LEARN_RATIO_NUM` (150), `PRESSURE_DEADBAND_PSI` (1), `OFFSET_SAMPLE_SETTLE_MS`
(250), `OFFSET_SAMPLE_SETTLE_DOWN_MS` (500), `LOG_MANUAL_OFFSET_SAMPLES`.

Note: `eval/model_eval.cpp` targeted the old online-learning model and is stale until ported.
