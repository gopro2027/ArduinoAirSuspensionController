# Wheel goal-routine thread synchronization

How the four wheel tasks stay in step while airing to a target, defined in
[`OASMan_ESP32/src/components/wheel.cpp`](../src/components/wheel.cpp).

## Why this exists

Each corner runs `Wheel::goalRoutine()` on its **own FreeRTOS task** (four tasks,
created in [`tasks.cpp`](../src/tasks/tasks.cpp), `task_wheel` → `Wheel::loop()` →
`goalRoutine()`). Every routine independently pulses its IN/OUT solenoid toward a
pressure/height target. Without coordination the corners drift apart: one finishes a
short pulse and immediately starts the next while another is still mid-pulse, so the
solenoids fire at different times and the car moves unevenly.

The goal-sync barrier keeps the **active** wheels loosely locked together: after each
iteration a wheel waits for the others before starting the next round of valve opens,
so the opens happen together.

## The primitive

Three functions over three shared, mutex-protected counters
(`goalSyncClubSize`, `goalSyncWaiting`, `goalSyncGeneration`; all access is inside
`wheelThreadLock()`/`wheelThreadUnlock()`, a 1-tick spin-with-yield mutex).

The set of wheels currently participating is the **club**. A wheel joins when it starts
its routine and leaves when it finishes, so the barrier automatically targets only the
corners that actually need to move.

### `goalSyncJoin()`
Called once at the top of `goalRoutine()` (after a 100 ms settle). Increments
`goalSyncClubSize`. The wheel is now a club member and everyone else's barrier will
wait for it.

### `goalSyncLeave()`
Called once at the very end of `goalRoutine()`. Decrements `goalSyncClubSize`, then —
critically — if wheels are already waiting and the reduced club size is now satisfied
(`goalSyncWaiting >= goalSyncClubSize`, or the club is empty), it releases them by
bumping `goalSyncGeneration`. This is what prevents a wheel that finishes early from
stranding the others at a barrier that can never reach the old count.

### `goalSyncBarrier(timeoutMs)`
A sense-reversing (generation-based) barrier:

1. Capture the current `goalSyncGeneration`, increment `goalSyncWaiting`.
2. If this wheel is the last to arrive (`goalSyncWaiting >= goalSyncClubSize`), reset
   `goalSyncWaiting`, bump `goalSyncGeneration`, and return immediately — this releases
   everyone.
3. Otherwise spin (`delay(1)`) until **any** of:
   - the generation changed (someone else released the group), or
   - the club fully gathered, or
   - the club emptied, or
   - `timeoutMs` elapsed (backstop).

Using the captured generation to detect release makes the barrier safe against a fast
wheel "lapping" — running its next iteration and re-entering the barrier before a slow
wheel has woken from the previous one. `goalSyncWaiting` is always reset to 0 by
whoever triggers a release, so counts never accumulate stale.

## Invariants (why it can't deadlock or leak)

- **Every `goalSyncJoin()` is matched by exactly one `goalSyncLeave()`.** All `break`
  paths out of the goal loop fall through to the final `goalSyncBarrier()` +
  `goalSyncLeave()`, and the routine only clears its start flag after leaving. Club
  membership can't leak.
- **`goalSyncClubSize >= 1` whenever a wheel is at a barrier**, because a wheel's own
  join keeps it counted until after its last barrier. The `clubSize == 0` branches are
  defensive.
- **No permanent stall.** Normal release comes from the club gathering or a wheel
  leaving; the `timeoutMs` backstop guarantees forward progress even if a wheel is
  starved, and the whole routine is independently capped at `ROUTINE_TIMEOUT_MS` (10 s).
- **No nested locking.** Join/Leave/Barrier each take and release the mutex in balanced,
  non-nested pairs.

## The mode-aware timeout (the fix)

The barrier timeout is **not** a single constant — the safe value depends on the valve
state *while a wheel waits*:

| Mode | Valve state at barrier | Iteration length | Timeout | Constant |
|------|------------------------|------------------|---------|----------|
| **Pressure** | Closed (open + settle already done) | ~250 ms – 5.5 s | **6000 ms** | `GOAL_SYNC_BARRIER_TIMEOUT_PRESSURE_MS` |
| **Height**   | Open (pulsing, closed next round) | ~1 ms | **100 ms** | `GOAL_SYNC_BARRIER_TIMEOUT_HEIGHT_MS` |

`goalSyncBarrierTimeoutMs()` picks the right one via `getheightSensorMode()`.

### What was wrong before

The barrier previously used **one 100 ms timeout for both modes**. In pressure mode a
single iteration is `open + delay(valveTime) + close + delay(250 settle)`, and
`valveTime ≈ 10 × remainingPSI` (up to ~5 s under AI prediction or a smooth air-out).
So two corners whose remaining delta differed by more than ~10 psi arrived at the
barrier more than 100 ms apart — the early one timed out and ran ahead **before the
slow one arrived**. During the bulk of a preset change (when deltas differ most) the
barrier timed out nearly every iteration and provided almost no coupling: the exact
"solenoids sound out of sync" symptom.

### Why the new values are safe

- **Pressure mode:** valves are *closed* at the barrier, so waiting longer just idles —
  no over-inflation risk. 6000 ms exceeds the longest possible single iteration
  (~5.5 s), so the fast wheel actually waits for the slow one and sync holds; the
  timeout only fires on a genuine stall (and the 10 s routine cap still bounds that).
- **Height mode:** the valve is *open* while waiting, so a long wait would keep moving
  that corner — the timeout stays short. Iterations are ~1 ms here anyway, so 100 ms is
  ample and rarely fires.

## Known residual looseness (not addressed by this change)

- **First iteration isn't tightly aligned.** The four wheel tasks poll on independent
  `delay(100)` cycles and pick up their start flags (set sequentially from the BLE
  task) up to ~100 ms apart, so joins — and the *first* round of valve opens — are
  staggered. Only subsequent iterations are barrier-aligned.
- **Late joiners / new goals mid-move** raise `goalSyncClubSize` while others are
  waiting, causing a one-timeout hiccup that self-heals.

## Verifying on-device

Build and flash the relevant env (e.g. `manifold_v4_dev`), then listen: the per-round
solenoid clicks across corners should land together during a preset change instead of
scattering. To get hard data, temporarily log `thisWheelNum`, `millis() - startMs`, and
whether the barrier exited via generation-change vs. timeout at the timeout check in
`goalSyncBarrier()`; the timeout branch should now be rare in pressure mode.
