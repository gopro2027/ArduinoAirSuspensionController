# Wheel goal-routine thread synchronization

How the four wheel tasks stay in step while airing to a target, defined in
[`OASMan_ESP32/src/components/wheel.cpp`](../src/components/wheel.cpp).

## Why this exists

Each corner runs `Wheel::goalRoutine()` on its **own FreeRTOS task** (four tasks,
created in [`tasks.cpp`](../src/tasks/tasks.cpp): `task_wheel` → `Wheel::loop()` →
`goalRoutine()`). Every routine independently drives its IN/OUT solenoid toward a
pressure/height target. Without coordination the corners finish and re-evaluate at
unrelated times, so the car moves unevenly and the solenoids fire raggedly.

The goal-sync barrier couples the **active** wheels: a corner that reaches its target
early waits for the slower corners before the routine ends, so the group starts together
and finishes together.

## The control model it runs on top of

`goalRoutine()` is **closed-loop / continuous-flow**, not pulse-based:

- It commits a direction from the true (valve-closed) reading, opens the valve, and
  **leaves it open across iterations**, running each live (flowing) reading through a
  predictor to recover the true bag value.
- The valve **closes once the predicted value reaches goal**, then a non-blocking
  *settle* window elapses and the settled value is re-verified (correcting over/undershoot).
- Each loop iteration is short (~1 ms `delay(1)` plus the ADC reads) in both pressure and
  height mode.

So during a move the valve is **open** for the whole fill/dump and **closed** during
settle and after completion.

## The primitive

Three functions over three shared, mutex-protected counters (`goalSyncClubSize`,
`goalSyncWaiting`, `goalSyncGeneration`; all access is inside
`wheelThreadLock()`/`wheelThreadUnlock()`, a 1-tick spin-with-yield mutex).

The set of wheels currently participating is the **club**. A wheel joins when it starts
its routine and leaves when it finishes, so the barrier automatically targets only the
corners that actually need to move.

### `goalSyncJoin()`
Called once at the top of `goalRoutine()`. Increments `goalSyncClubSize`. The wheel is
now a club member and everyone else's barrier will wait for it.

### `goalSyncLeave()`
Called once at the very end of `goalRoutine()`. Decrements `goalSyncClubSize`, then — if
wheels are already waiting and the reduced club size is now satisfied
(`goalSyncWaiting >= goalSyncClubSize`, or the club is empty) — releases them by bumping
`goalSyncGeneration`. This prevents a wheel that finishes early from stranding the others
at a count that can never be reached.

### `goalSyncBarrier(timeoutMs)`
A sense-reversing (generation-based) barrier:

1. Capture the current `goalSyncGeneration`, increment `goalSyncWaiting`.
2. If this wheel is the last to arrive (`goalSyncWaiting >= goalSyncClubSize`), reset
   `goalSyncWaiting`, bump `goalSyncGeneration`, and return immediately — releasing everyone.
3. Otherwise spin (`delay(1)`) until **any** of: the generation changed (someone released
   the group), the club fully gathered, the club emptied, or `timeoutMs` elapsed (backstop).

Using the captured generation to detect release makes the barrier safe against a fast
wheel "lapping" a slow one; `goalSyncWaiting` is always reset to 0 by whoever triggers a
release, so counts never go stale.

## The invariant: only ever wait at the barrier with the valve CLOSED

This is the load-bearing rule that keeps the barrier safe on a real vehicle. **A wheel
must never block at the barrier while its valve is open** — otherwise the wait would keep
airing that corner while its `reached`/`overCeiling` checks are not running, risking
overshoot for up to the full timeout.

`goalRoutine()` enforces it with exactly two barrier call sites, both at points where the
valve is *provably* closed:

1. **In-loop rendezvous is placed immediately after `valve->close()`** in the target-reached
   path (right after `settleUntil` is armed). A corner that is still flowing has not reached
   this line, so it never blocks the group with a valve open; a corner that just closed after
   reaching target blocks here until every other active corner has also closed. (An earlier
   revision inferred valve state from a per-tick `valveOpenThisTick` flag and put the barrier
   at the end of the loop; that was replaced because tying it directly to the close event is
   unambiguous — no flag, and no six-valve edge case where a valve left open by a prior tick
   could reach an end-of-loop barrier.)
2. **Valves are closed before the exit rendezvous.** Both solenoids are closed *before* the
   final `goalSyncBarrier()` / `goalSyncLeave()`, covering the routine-timeout break that can
   fire mid-fill.

Both sites feed the same global barrier counters, so a corner sitting at the in-loop
rendezvous and a corner at the exit rendezvous still gather each other — placement never
causes a deadlock.

Consequence, using a 4500 ms corner and a 100 ms corner as the example: the 100 ms corner
reaches target, closes, and blocks at the barrier — **valve closed, so harmless** — until
the 4500 ms corner also closes and arrives. They then release together and finish
together. The wait is bounded by the timeout below.

## The timeout

A single, mode-independent cap:

| Constant | Value |
|----------|-------|
| `GOAL_SYNC_BARRIER_TIMEOUT_MS` | **6000 ms** |

Because of the invariant, every barrier wait happens with the valve closed, so the cap
can be as generous as needed with no airing risk. It must exceed the longest a slow corner
can stay open before it closes and reaches the barrier — a single fill can run up to
~5000 ms — so 6000 ms lets a fast corner wait for the slowest before the rendezvous, and
only fires on a genuine stall (the routine itself is independently capped at
`ROUTINE_TIMEOUT_MS` = 10 s).

> History: an earlier revision used a split 6000 ms (pressure) / 100 ms (height) cap,
> reasoned from the *old* pulse-based routine where height mode waited at the barrier with
> the valve open. Once the routine became flow-based and the never-wait-with-valve-open
> invariant was enforced for both modes, the short height cap was no longer needed and the
> two collapsed into one constant.

## Invariants (why it can't deadlock or leak)

- **Every `goalSyncJoin()` is matched by exactly one `goalSyncLeave()`.** All `break`
  paths out of the loop fall through to the final barrier + leave, and the start flag is
  only cleared after leaving. Club membership can't leak.
- **No permanent stall.** Normal release comes from the club gathering or a wheel leaving;
  the timeout guarantees forward progress even if a wheel is starved.
- **No wait with an open valve** (see the invariant above), so a fired timeout is never a
  safety event — it just lets the waiting (idle, closed) corners proceed.
- **No nested locking.** Join/Leave/Barrier each take and release the mutex in balanced,
  non-nested pairs.

## Known residual looseness

- **First iteration isn't tightly aligned.** The four wheel tasks poll on independent
  `delay(100)` cycles and pick up their start flags (set sequentially from the BLE task)
  up to ~100 ms apart, so the initial valve opens are staggered.
- **During the flow itself the corners run independently** (each to its own target); the
  barrier couples the *start* and the *completion*, not the moment-to-moment airflow.

## Verifying on-device

Build and flash the relevant env (e.g. `manifold_v4_dev`), then listen: corners should
finish and settle together during a preset change rather than one wrapping up long before
the rest. For hard data, temporarily log `thisWheelNum`, `millis() - startMs`, and whether
the barrier exited via generation-change vs. timeout inside `goalSyncBarrier()`; the
timeout branch should be rare, and when it fires the valve is always already closed.
