# Pressure Goal / Presets — Design & Rationale

How OAS-Man drives each corner to a target pressure (or height) when a preset is loaded, why the
control loop is shaped the way it is, and the measured data behind every design decision. This is the
reference for future improvements to this code.

Companion docs:

- **[AI_TRAINING.md](../AI_TRAINING.md)** — the offset-model learning layer (samples → refit → predict → fade).
- **[docs/goal-sync-barrier.md](goal-sync-barrier.md)** — the `goalSync*` thread-rendezvous mechanism.

Code lives in `src/components/wheel.cpp` (`goalRoutine`, `achieveFineGoal`, `waitForStableReading`,
`openValveForMs`, `initPressureGoal`) with the model layer in `src/pressureMath.*`,
`src/airSuspensionUtil.cpp` (predictors, refit), and `src/manifoldSaveData.cpp` (sample storage).
Tunables in `ESP32_SHARED_LIBS/src/user_defines.h`.

---

## 1. The core problem: you cannot read a bag's pressure while its valve is open

Each corner has one pressure sensor plumbed where air flows between the tank and the bag. While a
valve is open, air rushing past the sensor skews the reading (Bernoulli/flow effects):

- **Filling (air-up):** the sensor reads **high** — it sees tank-side pressure bleeding through.
- **Venting (air-out):** the sensor reads **low** — it sees the pressure collapsing toward atmosphere.

Only after the valve closes and the air settles does the reading converge to the truth.

Measured on the Corvette (offset = settled − flowing, at the moment of valve close):

| Direction | Typical offset | Predictability |
|---|---|---|
| Air-up | ≈ −6 to −10 psi | Good — model RMSE ≈ 2.5–3 psi |
| Air-out | ≈ +15 to +20 psi | Poor — model RMSE ≈ 4.5–6 psi |

Worse, air-out has a measured **blind spot**: the flowing reading saturates toward a low plateau that
barely depends on true bag pressure. Holding the flowing reading fixed at ~15–30 psi mid-vent, the
*true* settled pressure ranged **25→59 psi (std ≈ 7–9 psi)**. That information is simply not in the
sensor during venting — no model or feature can recover it. This single fact drives most of the
design: the flowing reading is good enough to *move fast toward* goal, but only the **valve-closed
settled reading** can be trusted to *land* on goal.

---

## 2. Control flow overview

```
Load preset → initPressureGoal(target) per corner → flag set → wheel task runs goalRoutine:

  ┌─ COARSE loop ──────────────────────────────────────────────────────────┐
  │ read true (valve-closed) value                                         │
  │   at goal?            → done                                           │
  │   within 5 psi?       → achieveFineGoal() → done                       │
  │   else commit dir, hold valve open, stop when PREDICTED value = goal,  │
  │   close, barrier-rendezvous, settle-to-stable, log sample, re-decide   │
  └────────────────────────────────────────────────────────────────────────┘
  then: all corners rendezvous idle
  ┌─ FINAL RE-CHECK (×FINAL_RECHECK_ROUNDS) ──────────────────────────────┐
  │ settle-to-stable read → off goal? achieveFineGoal() → barrier         │
  └────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Entry — `initPressureGoal(newPressure, onlyAirUp, onComplete)`

- Rejects a goal above `getbagMaxPressure()` (pressure mode) or ~103% of `getHeightSensorMax()`.
- Pressure mode: refuses to start an air-up the tank can't deliver (tank pressure below current bag).
- Sets `pressureGoal`, `routineStartTime`, and the per-corner start flag; the four wheel FreeRTOS
  tasks each pick it up in `Wheel::loop()` → `goalRoutine()`.
- `onlyAirUp` (used by maintain-pressure) forbids venting anywhere in the routine — an overshoot is
  accepted rather than corrected downward.

### 2.2 Coarse phase (`goalRoutine`)

One loop shared by pressure and height modes; the only per-mode difference is the **predictor seam**:

```cpp
actual = heightMode ? getPredictedBagHeight(raw)            // identity stub — level sensor reads true during flow
                    : getPredictedBagPressure(aiIndex, raw, rawTank); // raw + learned/default offset
```

Cycle: with the valve closed the reading is true, so decide there — finish (within deadband), hand
off to fine (within `FINE_PULSE_THRESHOLD_PSI`), or commit a direction. Then hold the valve open,
re-computing `actual` from the flowing reading each ~1 ms tick, and close when `actual` reaches goal.
After closing: rendezvous at the barrier (valve closed), `waitForStableReading` for the true settled
value, log the flowing→settled sample, drop the committed direction, and re-decide from the top.

The re-decide is **bidirectional** — it continues, reverses, or hands off to fine based on the true
settled reading. (Earlier versions only corrected undershoot and accepted overshoot; the data showed
air-out overshoots low half the time, so monotonic verify systematically landed low.)

Deadbands: `PRESSURE_DEADBAND_PSI` = **0** (exact psi; safe only because the fine phase exists) and
`LEVEL_DEADBAND_PERCENTAGE` = 1 for height.

### 2.3 Settled read — `waitForStableReading(band)`

Every "true" reading uses this instead of a fixed wait: block until the reading holds within `band`
(`SETTLE_STABLE_BAND_PSI` 1 / `SETTLE_STABLE_BAND_LEVEL` 2) for `SETTLE_STABLE_MS` (100 ms), with a
`SETTLE_MAX_WAIT_MS` (1500 ms) backstop. Air-out settles much slower than air-up (the bag reading has
to *rise* ~15–20 psi back to truth after venting), so a stability gate waits exactly as long as
physically needed — fixed waits were either too short for air-out (corrupting both verify reads and
training labels) or wastefully long for air-up.

### 2.4 Fine phase — `achieveFineGoal()` (pressure only)

Because of the §1 blind spot, the coarse prediction cannot land precisely near goal — it stops early,
creeps in small steps, or oscillates. So within `FINE_PULSE_THRESHOLD_PSI` (5) of goal, stop trusting
the flowing prediction entirely and hone in on the **true** reading with short bursts:

1. `waitForStableReading` → error = goal − true. Within deadband → success, return.
2. Burst length: starts at `clamp(err × FINE_PULSE_MS_PER_PSI, FINE_PULSE_MIN_MS, FINE_PULSE_MAX_MS)`.
3. **Anti-oscillation:** each time the reading *crosses* the goal (direction flips), the burst is
   multiplied by `FINE_PULSE_OVERSHOOT_SHRINK` (0.5). Successive approximation: bursts shrink until
   it lands exactly, or the burst falls below 1 ms (hardware can't resolve finer — give up).
4. **Stuck detector:** if the reading doesn't move at all for `FINE_PULSE_MAX_TRIES` (8) bursts
   (tank/bag exhausted), give up rather than pulse until the routine timeout.
5. `onlyAirUp` + need-to-vent → accept where we are. Routine timeout also exits.

This mirrors the pre-ML main-branch anti-oscillation logic (shrink valve time on each goal crossing).
Height mode never runs fine — its sensor reads true during flow, so coarse already lands accurately.

`getMinValveOpenPSI()`/`FINE_PULSE_MIN_MS` note: the finest achievable step is governed by
`FINE_PULSE_MIN_MS` together with the shrink factor; those two are the knobs for "lands exactly."

### 2.5 Final cross-corner re-check

Observed on-car: a corner that finishes early can be nudged a few psi off by a sibling corner still
correcting (shared manifold/tank coupling — and a sibling's flow can also disturb the early corner's
"stable" reading). So after **all** corners rendezvous idle, every corner runs
`FINAL_RECHECK_ROUNDS` (4) synchronized rounds: settle-to-stable read → if off goal,
`achieveFineGoal()` → close valves → barrier. The per-round barrier means each round's *read* happens
with all corners idle (clean), and a round-N correction that disturbed a neighbor gets caught in
round N+1.

Chosen deliberately as the **simple concurrent** design. Its known residual: corrections in the very
last round are never themselves re-verified, so a tiny leftover can survive (that's why the round
count was raised 2→4). The robust alternative, if ever needed: loop **until a round makes zero
corrections** (capped), or re-check corners **sequentially** (one corrects while three hold still) —
sequential fully eliminates mutual disturbance at the cost of complexity/time.

Height mode skips the re-check (no manifold-pressure coupling of its level reading). This is safe for
barrier accounting because `heightSensorMode` is global — all corners run the same barrier count.

### 2.6 Thread sync (summary — see goal-sync-barrier.md)

Four wheel tasks, one dynamic "club" (`goalSyncJoin/Leave/Barrier`). **INVARIANT: a corner only ever
waits at a barrier with its valves closed** — a flowing corner simply hasn't reached the barrier call
yet, so a waiting corner is always harmless no matter how long it waits. `achieveFineGoal` takes no
barriers at all (bursts are quick and local); a fine corner rendezvouses at the exit/re-check
barriers only. The barrier timeout (6000 ms) is a backstop, sized above the longest single fill.

### 2.7 Safety

- **In-loop ceiling (coarse fill, pressure mode):** close + abort immediately if the predicted value
  reaches `getbagMaxPressure()` (≤ `MAX_PRESSURE_SAFETY` 200). No settle/verify on this path.
- **`ROUTINE_TIMEOUT_MS` (10 s):** bounds the whole routine (empty tank, stuck sensor, hunting).
- **Exit path always closes both valves** before the final rendezvous.
- Known gap (accepted for now): height mode has no in-loop bag-pressure ceiling while chasing a
  height target (TODO in code).

---

## 3. The learning layer (summary — see AI_TRAINING.md for full detail)

`actualBag = rawBag + offset`, `offset = f(flowDifferential)` — a per-axle-per-direction 3-coefficient
linear model (`[d, d², 1]`, d = differential/100; differential = tank−bag on fill, bag on dump), fit
by closed-form ridge least squares over all stored samples (~1 ms), re-fit from scratch whenever the
sample count changes. **Nothing is persisted but the samples** (SPIFFS, 4 files); weights re-derive
every boot; one schema version (`ML_SAMPLE_RECORD_VERSION`) whose mismatch wipes samples.

Before training data exists, the offset is a flat constant (−5 fill / +5 dump, `OFFSET_DEFAULT_PSI`),
faded linearly into the trained model between `OFFSET_FADE_MIN` (25) and `AI_LEARN_RATIO_NUM` (150)
samples.

**Sample collection:** every *coarse* close logs `{flowing bag, settled bag, flowing tank}` (the
settled label comes from the stability-gated read). Manual up/down moves also log (via
`captureManualOffsetSample`). **Fine bursts are deliberately NOT logged** — near-goal air-out samples
are exactly the blind-spot data (±9 psi label scatter) that poisons the fit. `SAMPLE_DEDUP_PSI` (1)
drops a sample within 1 psi (flowing AND settled) of the previous stored one, killing the long
runs of near-identical samples a single preset move produces (measured: up to 18 in a row, 15–25% of
the file). The four models are independent, so uneven up/down counts are harmless.

**`othersOpen` was removed as a feature:** tank sag from other open valves already shows up in
`rawTank` (which the differential captures); measured contribution ≈ 0. Contention needs no separate
input.

---

## 4. Design history — what was tried, what the data said

Understanding *why* the current shape won matters for future changes:

1. **Valve-time prediction (rejected).** The original ML predicted *how long* to open the valve from
   (start, goal, tank). It failed for structural reasons: open-loop (no mid-move correction), a
   self-reinforcing data-poisoning loop (its own bad moves generated its training data), and poor
   extrapolation across pressure ranges. Accuracy requirement (±3 psi) was unreachable.
2. **Closed-loop on the raw flowing reading (rejected).** Mirrors the height-sensor branch, but the
   raw reading is wrong by the §1 offsets — stops ~10 psi early on fill, ~20 late on dump.
3. **Closed-loop on a learned offset correction (current coarse phase).** Turns the ML from "predict
   the future" into "correct a sensor" — a far easier, self-labelling problem (every valve close
   yields a perfect training pair automatically).
4. **Verify-after-close** (monotonic → **bidirectional**): the settled reading is re-checked and the
   loop continues/reverses. Monotonic verify accepted overshoot and biased air-out low → made
   bidirectional.
5. **Deadband journey 2 → 1 → 0:** 2 left ±2 psi errors; 0 alone caused hunting (valves overshoot the
   exact integer between ticks — a tick can move 1–3 psi); 1 was a stopgap. The **fine-pulse phase**
   is what made 0 viable.
6. **Fine pulse + anti-oscillation crossing-shrink** (§2.4): resolves finer than a normal control
   tick; immune to the blind spot because it reads valve-closed only.
7. **Final concurrent re-check** (§2.5): fixes the cross-corner nudge; simple-concurrent chosen over
   sequential, rounds raised 2→4 from on-car results.

---

## 5. Tuning guide

All in `user_defines.h`. What to touch for which symptom:

| Symptom | Knob(s) |
|---|---|
| Lands 1–2 psi off, load-preset-again fixes it | `FINAL_RECHECK_ROUNDS` up (or implement loop-until-clean, §2.5) |
| Fine phase nudges past exact repeatedly | `FINE_PULSE_MIN_MS` down (finest step), `FINE_PULSE_OVERSHOOT_SHRINK` toward 0.5–0.7 |
| Fine phase too slow / too many bursts | `FINE_PULSE_MS_PER_PSI` up (aim: one burst ≈ half the gap) |
| Coarse hands off too early/late | `FINE_PULSE_THRESHOLD_PSI` |
| Verify reads look wrong on air-out | `SETTLE_STABLE_MS` up / `SETTLE_STABLE_BAND_PSI` down (`SETTLE_MAX_WAIT_MS` is the ceiling) |
| Samples accumulate too slowly | `SAMPLE_DEDUP_PSI` down (min 0 = off), `LOG_MANUAL_OFFSET_SAMPLES` |
| Model trusted too soon/late | `OFFSET_FADE_MIN`, `AI_LEARN_RATIO_NUM` |
| Untrained behavior off | `OFFSET_DEFAULT_PSI` (flat ±psi before training) |

Bench/serial diagnostics: the boot `BEGIN/END IMPORTANT DATA FOR PRO` dump prints every stored sample
(3-tuples) for offline analysis; `Refit model N: ...` lines show live weights; `task_trainAI` prints
its stack high-water mark at boot.

---

## 6. Known limitations & future improvements

- **Last-round re-check residual** (§2.5) — replace fixed rounds with loop-until-no-corrections, or
  sequential per-corner re-check.
- **Air-out blind spot is physical** — no feature will fix it; only the fine phase / settled reads
  address it. Don't spend effort on fancier flowing-reading models for dump.
- **Height mode valve-close-lag model** — `getPredictedBagHeight` is an identity stub; a small model
  could close slightly early to account for valve-close travel (needs data).
- **Height mode in-loop pressure ceiling** — TODO in `goalRoutine`; heavy load or a stuck level
  sensor could over-pressure a bag while chasing height.
- **Manual-move capture** still uses fixed settle waits (`OFFSET_SAMPLE_SETTLE_MS`/`_DOWN_MS`) rather
  than `waitForStableReading`.
- **Flowing-feature denoise** — average the last few pre-close ticks instead of one instantaneous
  reading.
- `eval/model_eval.cpp` targets the deleted online-learning model; stale until ported.
