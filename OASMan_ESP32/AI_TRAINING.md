# AI Pressure Correction — How It Works

The manifold controls bag pressure **closed-loop**: it opens a valve and holds it open, re-reading the
sensor each tick, and closes when the bag reaches the goal — exactly how height-sensor mode works. The
problem this has to solve: **you cannot read a bag's true pressure while its valve is open.** Air flowing
past the manifold-mounted sensor makes the bag read high on fill / low on dump (and the tank read low);
the reading only settles to the truth once the valve closes.

So the learned model is no longer a valve-*time* predictor. It is a **sensor-offset corrector**:

$$\text{actualBag} = \text{rawBag} + \text{offset},\qquad \text{offset} = f(\text{flowDifferential},\ \text{othersOpen})$$

The controller feeds the live flowing readings through `f` to recover the true pressure and stops at goal.

Relevant code:

| File | Role |
|---|---|
| `src/pressureMath.h/.cpp` | The offset model (`AIModel`), batch fitter (`AIFitter`), online learner (RLS), constants |
| `src/airSuspensionUtil.cpp` | Batch/online training, `getActualBagPressure()`, ADS data-rate setup |
| `src/manifoldSaveData.cpp` | Offset-sample store (SPIFFS + RAM), online queues, weight persistence (NVS), migration |
| `src/components/wheel.cpp` (`goalRoutine`) | The closed-loop controller; logs offset samples on close |
| `src/tasks/tasks.cpp` (`task_trainAI`) | Batch-trains at boot, then loops the online learner |

There are **4 models per vehicle** (up-front, up-rear, down-front, down-rear; both corners of an axle share).

---

## The offset model

Each model is a 4-parameter linear regression predicting the offset in psi. The bag sensor sits on the
manifold, upstream of the bag; during flow it reads a pressure drop that scales with flow, and flow scales
with the differential driving it.

- **Fill (up):** differential = `rawTank − rawBag`; the sensor over-reads, so the offset is negative.
- **Dump (down):** differential = `rawBag` (bag → atmosphere ≈ 0); the sensor under-reads, offset positive.

Features (`computeFeatures`, scaled by 100 so the squared term stays well-conditioned):
`f = [ differential/100, (differential/100)², othersOpen, 1 ]`. Predictions/labels are scaled by
`ML_OFFSET_NORM` (100) to keep weights O(0.1).

Both corrections vanish as flow stops (`rawBag → rawTank` on fill, `rawBag → 0` on dump), so the estimate
converges to the true pressure exactly where the controller needs to stop. Because the model is trained on
**flowing-reading-before-close → settled-reading-after-close**, it inherently absorbs the valve-close /
in-flight overshoot — no separate close-early margin is needed.

**Physics-default seed weights** (`loadAILearnedDataPreferences`): fill models seed `w1 = −0.12`, dump
models `w1 = +0.12`, everything else 0 (≈10 psi at a big differential, a few psi near goal). Control works
from first boot on these defaults; learning refines them per corner.

### Contention (`othersOpen`)

All corners share one tank and one exhaust, so a same-direction move by other corners changes the flow and
therefore the offset. Unlike the old time model, this is a **live measurement**, not a prediction: each tick
the controller counts how many other corners currently have a same-direction valve open
(`countOthersOpenSameDirection`, via `Solenoid::isOpen()`). No intent-sync, barriers, or guessing.

---

## The closed-loop controller (`Wheel::goalRoutine`)

Per corner, once a goal is set:

1. Commit a direction from the raw reading (far from goal the ~10 psi offset is small vs the distance;
   near goal the deadband stops the move).
2. Each tick: `readInputs()`, read the flowing tank, count `othersOpen`, compute
   `actual = getActualBagPressure(...)`, and hold the valve open.
3. Stop and close when `actual` is within `PRESSURE_DEADBAND_PSI` (2) of goal — or immediately if it hits the
   in-loop `getbagMaxPressure()` / `MAX_PRESSURE_SAFETY` ceiling (fill), or the 10 s `ROUTINE_TIMEOUT_MS`
   (covers tank-empty / stuck-sensor stalls), or `onlyAirUp` forbids a needed air-down.

The four wheel threads run concurrently and rendezvous on the existing generation barrier
(`custom_barrier_wait`) each tick. Height-sensor mode uses the same loop with the level sensor and no offset.

**Reads are fast enough** because `initializeADS()` raises the ADS1115 data rate to `RATE_ADS1115_860SPS`
(~1.16 ms/conversion vs the ~7.8 ms default); a bag+tank read per tick under 4-corner load stays ≤ ~1 psi of
control lag.

---

## Sample collection (self-collecting)

On every valve close, `goalRoutine` logs one offset sample: the last **flowing** readings while the valve was
open, paired with the **settled** bag reading taken `OFFSET_SAMPLE_SETTLE_MS` (250) after close:

```
{ raw_bag, settled_bag, raw_tank, others_open }   // label offset = settled_bag - raw_bag
```

Closes happen near goal, so samples concentrate exactly where stopping accuracy matters. `recordLearnSample`
appends to the model's SPIFFS file (`learnData[4]`, heap-allocated, up to `LEARN_SAVE_COUNT`=300) until full,
then feeds the online ring queue. (Manual-move offset capture is disabled for now —
`USE_MANUAL_SAMPLE_LOGGING false` — and is a clean follow-up driven from `Wheel::loop` by valve state.)

---

## Training (reused least-squares pipeline)

- **Batch:** at boot `trainSingleAIModel` fits the offset with `AIFitter` (ridge normal equations, Gaussian
  elimination) over stored samples. A self-fit RMSE over `ML_BATCH_RMSE_GATE_PSI` (8 psi) or a
  singular/degenerate solve with ≥ `ML_FIT_MIN_SAMPLES` (25) samples is treated as unusable data and **wiped**;
  fewer than 25 samples are kept and the physics-default weights keep driving control meanwhile.
- **Online:** once trained, each new sample updates the model via RLS (forgetting factor `ML_RLS_FORGETTING`)
  with an outlier gate and anchor replay, in `processLearnSampleQueues`. RLS state is RAM-only, rebuilt at boot.
- Models still collecting (< `LEARN_SAVE_COUNT`) are re-batch-fit each boot; models past it that have trained
  keep their drifted NVS weights (only RLS state is rebuilt). `getActualBagPressure` **always** applies the
  correction (default or learned) — a raw flowing reading alone is wrong by ~10 psi.

---

## Schema migration

Two version numbers in `pressureMath.h`, checked at boot in `beginSaveData()`:

| Key | Constant | On mismatch |
|---|---|---|
| `mlModelSchema` | `ML_MODEL_SCHEMA_VERSION` (6) | `clearAIWeightsOnly()` — drop weights, keep samples, refit |
| `mlSampleRec` | `ML_SAMPLE_RECORD_VERSION` (3) | `clearPressureData()` — wipe samples + weights (layout changed) |

The move from time prediction to the offset model bumped both, so a fielded device wipes the old (incompatible)
time samples on first boot of this firmware and re-collects offset samples through normal use.

---

## Tuning constants

`pressureMath.h`: `ML_OFFSET_NORM` (100, label/prediction scale), `ML_FIT_RIDGE`, `ML_FIT_MIN_SAMPLES` (25),
`ML_BATCH_RMSE_GATE_PSI` (8), RLS/gate/anchor constants. `user_defines.h`: `LEARN_SAVE_COUNT` (300),
`PRESSURE_DEADBAND_PSI` (2), `OFFSET_SAMPLE_SETTLE_MS` (250), and the ADS data rate lives in `initializeADS()`.

Note: `eval/model_eval.cpp` targeted the old time model and is stale until ported to the offset model.
