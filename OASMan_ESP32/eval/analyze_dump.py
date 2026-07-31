"""Inspect a device sample dump for data-quality problems.

Answers: is the recorded (start, end, tank, time) data self-consistent enough to fit, or is the
model being asked to explain noise? Usage: python analyze_dump.py "<dump file>"
"""

import re
import sys
import os
import math

TUPLE_RE = re.compile(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}")
MARKERS = [
    ("/UpDataF.dat", "up front", True),
    ("/UpDataR.dat", "up rear", True),
    ("/DownDataF.dat", "down front", False),
    ("/DownDataR.dat", "down rear", False),
]


def load(path):
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    out = []
    for marker, name, up in MARKERS:
        rows = []
        for i, line in enumerate(lines):
            if marker in line:
                found = TUPLE_RE.findall(line) or TUPLE_RE.findall(lines[i + 1])
                rows = [tuple(int(v) for v in t) for t in found]
                break
        out.append((name, up, rows))
    return out


def pearson(xs, ys):
    n = len(xs)
    if n < 3:
        return 0.0
    mx, my = sum(xs) / n, sum(ys) / n
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    dx = math.sqrt(sum((x - mx) ** 2 for x in xs))
    dy = math.sqrt(sum((y - my) ** 2 for y in ys))
    return num / (dx * dy) if dx > 0 and dy > 0 else 0.0


def analyze(name, up, rows):
    print(f"\n--- {name}  ({len(rows)} samples) ---")
    if not rows:
        return

    delta = [abs(e - s) for s, e, t, ms in rows]
    times = [ms for s, e, t, ms in rows]
    tanks = [t for s, e, t, ms in rows]
    starts = [s for s, e, t, ms in rows]
    # psi moved per second of valve-open time - the thing the model ultimately predicts
    rates = [d / (ms / 1000.0) for d, ms in zip(delta, times)]

    print(f"  move size  min {min(delta):3d}  median {sorted(delta)[len(delta)//2]:3d}  max {max(delta):3d} psi")
    print(f"  time       min {min(times):4d}  median {sorted(times)[len(times)//2]:4d}  max {max(times):4d} ms")
    print(f"  tank       min {min(tanks):3d}  max {max(tanks):3d} psi")
    print(f"  rate       min {min(rates):5.1f}  median {sorted(rates)[len(rates)//2]:5.1f}  max {max(rates):5.1f} psi/s"
          f"   ({max(rates)/max(min(rates), 0.01):.0f}x spread)")

    # How much of the sample set is small trim moves? That is the regime that oscillates in the car.
    small = sum(1 for d in delta if d <= 10)
    tiny = sum(1 for d in delta if d <= 5)
    print(f"  coverage   {tiny:3d} samples <=5 psi   {small:3d} <=10 psi   {len(delta)-small:3d} >10 psi")

    # Distinct move sizes - a fit can only learn a curve if the inputs actually vary
    print(f"  distinct move sizes: {len(set(delta))}, distinct times: {len(set(times))}")

    print(f"  corr(move, time)  {pearson(delta, times):+.3f}   <- should be strongly positive")
    print(f"  corr(tank, rate)  {pearson(tanks, rates):+.3f}   <- fills should be faster from a fuller tank")
    print(f"  corr(start, rate) {pearson(starts, rates):+.3f}")

    # Contradictions: pairs where a bigger move completed in less time. Physically impossible for
    # the same corner under the same conditions, so each one is evidence of an uncontrolled variable.
    bad = 0
    total = 0
    worst = None
    for i in range(len(rows)):
        for j in range(len(rows)):
            if i == j:
                continue
            total += 1
            if delta[i] > delta[j] * 1.5 and times[i] < times[j] * 0.8:
                bad += 1
                gap = (delta[i] / delta[j]) * (times[j] / times[i])
                if worst is None or gap > worst[0]:
                    worst = (gap, rows[i], rows[j])
    print(f"  contradictory pairs: {100.0*bad/max(total,1):.1f}% of all pairs")
    if worst:
        print(f"    worst: {worst[1]} moved {abs(worst[1][1]-worst[1][0])} psi in {worst[1][3]} ms, but")
        print(f"           {worst[2]} moved only {abs(worst[2][1]-worst[2][0])} psi in {worst[2][3]} ms")

    # Duplicate/near-duplicate rows - the ring of repeated presets means the same move is logged
    # over and over, which inflates the sample count without adding information
    uniq = len(set((s, e, t) for s, e, t, ms in rows))
    print(f"  unique (start,end,tank) combos: {uniq} of {len(rows)}")

    # goalRoutine only logs the first two pulses of a routine. The second pulse starts where a
    # recent first pulse ended, so we can separate them and compare rates. If the two groups differ,
    # conditions changed between pulses (other corners finishing = more flow for whoever is left),
    # which is an uncontrolled variable the model cannot see.
    first, second = [], []
    for i in range(len(rows)):
        s = rows[i][0]
        chained = any(abs(rows[j][1] - s) <= 2 for j in range(max(0, i - 4), i))
        (second if chained else first).append(rates[i])
    if len(first) >= 5 and len(second) >= 5:
        mf = sorted(first)[len(first) // 2]
        ms_ = sorted(second)[len(second) // 2]
        print(f"  median rate: 1st pulse {mf:5.1f} psi/s ({len(first)} samples)"
              f"   2nd pulse {ms_:5.1f} psi/s ({len(second)} samples)"
              f"   -> {ms_/max(mf,0.01):.2f}x")


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(__file__), "..", "ai weights for corvette 7.30.2026 SCEMA_VERSION 2.txt")
    print(f"Analyzing {os.path.basename(path)}")
    for name, up, rows in load(path):
        analyze(name, up, rows)


if __name__ == "__main__":
    main()
