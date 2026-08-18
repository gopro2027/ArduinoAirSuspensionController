# AI Pressure Correction — How It Works

> Full control-loop design, decision history, and tuning guide:
> [docs/pressure-goal-routine.md](docs/pressure-goal-routine.md). This file covers the ML layer.

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

**The same 4 models and 4 files serve both pressure and height-sensor mode.** In pressure mode the samples
hold psi; in height mode they hold height %. This keeps the ML layer unchanged at the cost of one rule:
stored AI data is only valid for the mode it was collected in, since mixed units train a meaningless model.
**`setheightSensorMode()` enforces this — it calls `clearPressureData()` whenever the mode actually
changes** (hand-written rather than `createSaveFuncInt` for exactly this reason; boot loads the preference
directly, so a reboot never wipes). The untrained default also follows the active mode
(`OFFSET_DEFAULT_PSI` vs `OFFSET_DEFAULT_LEVEL`).

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

There is **no physics-default formula**. Before a model is trained, `getPredictionOffset` returns a flat
constant, negative on air-up and positive on air-out: `OFFSET_DEFAULT_PSI` (5) in pressure mode,
`OFFSET_DEFAULT_LEVEL` (2) in height mode. As samples accumulate it fades the trained model in:

    w = clamp((count − OFFSET_FADE_MIN) / (AI_LEARN_RATIO_NUM − OFFSET_FADE_MIN), 0, 1)
    offset = default × (1 − w) + trained × w

Below `OFFSET_FADE_MIN` (25) → pure default; at `AI_LEARN_RATIO_NUM` (150) → pure trained. When a real
trained model exists the constant is fully faded out.

### Sample collection

Every valve close logs one sample: the last **flowing** readings while the valve was open, paired with the
**settled** bag reading after close — `{ raw_bag, settled_bag, raw_tank }` (offset =
`settled_bag − raw_bag`). Two triggers, one sink (`recordLearnSample` → SPIFFS + RAM):

- **Preset / maintain moves** — `goalRoutine` logs on close (after the direction's settle time).
- **Manual moves** (BLE valveControlBittset / gamepad) — `Wheel::captureManualOffsetSample` watches this
  corner's valve state each `Wheel::loop` tick, caching the flowing readings while open and logging after
  the settle on close. Always on; only skipped while a goal routine is running (it collects its own).

Closes happen near goal, so samples concentrate where stopping accuracy matters. The
`BEGIN/END IMPORTANT DATA FOR PRO` serial dump prints every stored sample for offline analysis.

### Training

`task_trainAI` calls `trainOffsetModels()` at boot (refits all 4 from their loaded samples) then every
100 ms (refits only a model whose sample count changed). Fast conversions help: `initializeADS()` sets the
ADS1115 to `RATE_ADS1115_860SPS`.

## The closed-loop controller (`Wheel::goalRoutine`)

Decoupled into small pieces: the `goalRoutine` **coarse** loop, plus three helpers —
`waitForStableReading` (settled read), `achieveFineGoal` (**fine** phase), and `openValveForMs` (a burst).
Pressure and height mode run the **same** loop, predictor, models, fine phase and re-check. The only
per-mode differences are a `ModeTuning` struct (deadband / settle band / fine constants), the untrained
default constant, and the pressure-only bag-max ceiling.

**Coarse phase (`goalRoutine`).** Commit a direction from the true (valve-closed) reading; each tick read the
live value and compute `actual = getPredictedBagPressure(aiIndex, raw, rawTank)` (raw + learned/default
offset — psi or height % depending on mode), hold the valve open; when `actual` reaches goal, close,
rendezvous at the barrier (valve closed), then `waitForStableReading` for the true settled value, log the
flowing→settled sample, and re-decide against it (**bidirectional**: continue, reverse, or hand off to fine).

**Settled read (`waitForStableReading`).** Instead of a fixed settle wait, block until the reading holds
within a band (`SETTLE_STABLE_BAND_PSI` / `_LEVEL`) for `SETTLE_STABLE_MS` (100 ms), with a
`SETTLE_MAX_WAIT_MS` backstop. Air-out (which rises back to true slowly) waits exactly as long as it needs.

**Fine phase (`achieveFineGoal`) — runs in BOTH modes.** In pressure mode the flowing reading is a blind,
low-saturating proxy during air-out (measured: the true settled pressure scatters ±7–15 psi for a fixed
flowing reading in the mid range), so the coarse prediction cannot land precisely and tends to hunt near
goal. Height mode has no such blind spot, but it still can't stop on an exact percentage from a moving
chassis — the fine phase gives it the same precision, and it needs **no model** to do so because it works
purely off accurate valve-closed readings. Once the true reading is within the mode's `FINE_PULSE_THRESHOLD_*`
of goal, `goalRoutine` calls `achieveFineGoal`, which hones in with short **bursts** off
`waitForStableReading`. The burst starts sized to the remaining error (`clamp(err × msPerUnit, minMs, maxMs)`
from `ModeTuning`); **each time the reading crosses the goal it shrinks the burst by
`FINE_PULSE_OVERSHOOT_SHRINK`** (anti-oscillation, mirroring the old logic). It exits on: landing exactly on
goal (success); the burst shrinking below 1 ms while still crossing (hardware can't resolve finer — give up);
the reading not moving for `FINE_PULSE_MAX_TRIES` bursts (stuck: tank/bag exhausted); or the routine timeout.
Fine bursts are **not** logged (they'd pollute the model with blind near-goal samples).

**Final cross-corner re-check — runs in BOTH modes.** Corners finish at slightly different times, and a
sibling still moving can nudge an already-done corner (through the shared manifold in pressure mode, through
the chassis in height mode). After the main loop all corners rendezvous idle, then run
`FINAL_RECHECK_ROUNDS` synchronized rounds: re-read the settled value and, if off, re-correct with
`achieveFineGoal`. Each round is bracketed by a barrier so the next read happens with all corners idle again,
catching a correction that disturbed a neighbor. `getheightSensorMode()` is global, so every corner runs the
same barrier count (no club mismatch).

**Sync / safety.** `achieveFineGoal` takes no barriers (the valve is only open during a burst, never
waiting), so the never-wait-with-valve-open invariant holds and a fine corner rendezvouses only at
`goalRoutine`'s exit barrier. Pressure mode enforces the in-loop `getbagMaxPressure()` /
`MAX_PRESSURE_SAFETY` ceiling (coarse fill); height mode has no in-loop pressure ceiling. Other hard stops:
the 10 s `ROUTINE_TIMEOUT_MS`, or an `onlyAirUp` block (which accepts an overshoot rather than venting).

**Sample logging / de-dup.** A preset move closes the valve many times (each coarse close logs a
flowing→settled sample), so `recordLearnSample` drops any sample within `SAMPLE_DEDUP_PSI` of the
previous stored one for that model — killing long runs of near-identical samples while keeping distinct
pressures. The four models are independent, so an uneven up/down sample count is harmless.

## Persistence / migration

Only the **sample files** are stored (SPIFFS); weights are never persisted. One version,
`ML_SAMPLE_RECORD_VERSION` (`pressureMath.h`): if the stored `mlSampleRec` differs, `clearPressureData`
wipes the sample files (`beginSaveData`). No model/feature version — a feature change just takes effect on
the next refit.

## Tuning constants

`pressureMath.h`: `ML_OFFSET_NORM` (100), `ML_FIT_RIDGE`, `ML_FIT_MIN_SAMPLES` (25), `ML_NUM_COEFF` (3),
`ML_SAMPLE_RECORD_VERSION`. `user_defines.h`: `LEARN_SAVE_COUNT` (300), `OFFSET_DEFAULT_PSI` (5),
`OFFSET_FADE_MIN` (25), `AI_LEARN_RATIO_NUM` (150), `SAMPLE_DEDUP_PSI` (1), `FINAL_RECHECK_ROUNDS` (4),
`SETTLE_STABLE_MS` (100), `SETTLE_MAX_WAIT_MS` (1500), `FINE_PULSE_OVERSHOOT_SHRINK` (0.5),
`FINE_PULSE_MAX_TRIES` (8). `OFFSET_SAMPLE_SETTLE_MS`/`_DOWN_MS` are now only for the manual-move capture.

The control loop's mode-specific knobs come in pairs — pick the one for the mode you're tuning:

| | Pressure mode | Height mode |
|---|---|---|
| deadband | `PRESSURE_DEADBAND_PSI` (0) | `LEVEL_DEADBAND_PERCENTAGE` (0) |
| settle band | `SETTLE_STABLE_BAND_PSI` (1) | `SETTLE_STABLE_BAND_LEVEL` (1) |
| fine threshold | `FINE_PULSE_THRESHOLD_PSI` (5) | `FINE_PULSE_THRESHOLD_LEVEL` (5) |
| fine ms per unit | `FINE_PULSE_MS_PER_PSI` (5) | `FINE_PULSE_MS_PER_LEVEL` (20) |
| fine burst min/max | `FINE_PULSE_MIN_MS` (5) / `FINE_PULSE_MAX_MS` (100) | `FINE_PULSE_MIN_MS_LEVEL` (5) / `FINE_PULSE_MAX_MS_LEVEL` (150) |

The finest achievable step is governed by the mode's `FINE_PULSE_MIN_MS*` together with
`FINE_PULSE_OVERSHOOT_SHRINK`. With a 0 deadband the settle band must stay tight, or a "settled" reading can
wobble wider than the controller will accept and it hunts.

## Future improvements

- **Better features for height mode.** In height mode the model learns valve-close *overshoot* (the level
  sensor already reads true during flow), but it currently reuses the pressure feature — the tank/bag
  differential — because that keeps the sample layout and `pressureMath` untouched. The physically causal
  input is the **measured rate of travel** at the moment of reading, since overshoot is displacement after
  the read (`≈ velocity × lag`). Worth revisiting if height accuracy plateaus; it would need a rate tracker
  and a per-mode feature branch.
- **Separate storage per mode.** Today both modes share the same 4 files, so switching modes wipes the
  training data. A second file set would remove that at the cost of ~3.6 KB heap and a wider model index.
- Average the flowing reading over the last few pre-close ticks to denoise the model feature.
- The manual-move capture still uses fixed `OFFSET_SAMPLE_SETTLE_*` waits rather than `waitForStableReading`.

Note: `eval/model_eval.cpp` targeted the old online-learning model and is stale until ported.
