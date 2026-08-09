"""Analyze a schema-3 serial dump: {start, goal, tank, ms, others_flowing} per sample.

The headline question this answers: does others_flowing (how many other corners were flowing during
the pulse) actually change the flow rate, once move size is held constant? Serial dumps can lose
bytes, so section sizes are reported rather than assumed.

Usage: python parse_dump3.py "<dump file>" [--compare "<older dump>"]
"""

import re
import sys
import os
from statistics import median

TUPLE5_RE = re.compile(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}")
TUPLE4_RE = re.compile(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}")
HEADER_RE = re.compile(r"/(\w+)\.dat \((\d+)\):")


def parse(path):
    """Returns [(name, claimed_count, rows)]. rows are 5-tuples; schema-2 dumps get others=-1."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    s, e = text.find("BEGIN IMPORTANT DATA FOR PRO"), text.find("END IMPORTANT DATA FOR PRO")
    body = text[s:e] if s >= 0 and e > s else text

    out = []
    for line in body.splitlines():
        h = HEADER_RE.search(line)
        rows = [tuple(int(v) for v in m) for m in TUPLE5_RE.findall(line)]
        if not rows:
            rows = [tuple(int(v) for v in m) + (-1,) for m in TUPLE4_RE.findall(line)]
        if h:
            out.append([h.group(1), int(h.group(2)), rows])
        elif rows:
            if out and not out[-1][2]:
                out[-1][2] = rows  # data line directly under its header
            else:
                out.append([None, None, rows])
    return out


def corr(xs, ys):
    n = len(xs)
    if n < 3:
        return 0.0
    mx, my = sum(xs) / n, sum(ys) / n
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    sxx = sum((x - mx) ** 2 for x in xs)
    syy = sum((y - my) ** 2 for y in ys)
    return sxy / (sxx * syy) ** 0.5 if sxx > 0 and syy > 0 else 0.0


def usable(rows, up):
    """The recording-path filter: right direction, moved >3 psi, valve open >10ms."""
    out = []
    for s, g, tank, ms, others in rows:
        move = (g - s) if up else (s - g)
        if move > 3 and ms > 10:
            out.append((s, g, tank, ms, others, move, move / (ms / 1000.0)))
    return out


def describe(name, rows, up):
    d = usable(rows, up)
    if len(d) < 10:
        print(f"  {name}: only {len(d)} usable samples")
        return
    moves = [r[5] for r in d]
    times = [r[3] for r in d]
    rates = [r[6] for r in d]
    print(f"  {name}: {len(d)} usable of {len(rows)}   move {min(moves)}-{max(moves)} psi   "
          f"corr(move,time) {corr(moves, times):+.3f}   median rate {median(rates):.1f} psi/s")

    others = [r[4] for r in d]
    if others and others[0] >= 0:
        dist = {k: others.count(k) for k in sorted(set(others))}
        print(f"      others_flowing distribution {dist}   corr(others,move) {corr(others, moves):+.3f}")
        # The controlled test. Within a move-size band, does contention change the rate? If the
        # medians match, contention is not what the model is missing.
        for lo, hi in ((4, 12), (12, 25), (25, 70)):
            band = [r for r in d if lo <= r[5] < hi]
            alone = [r[6] for r in band if r[4] <= 1]
            busy = [r[6] for r in band if r[4] >= 2]
            if len(alone) >= 5 and len(busy) >= 5:
                ma, mb = median(alone), median(busy)
                print(f"      move {lo:2d}-{hi:2d} psi: <=1 other {ma:6.1f} psi/s (n={len(alone):3d})   "
                      f">=2 others {mb:6.1f} psi/s (n={len(busy):3d})   ratio {mb / max(ma, 0.01):.2f}x")


def main():
    path = sys.argv[1]
    compare = None
    if "--compare" in sys.argv:
        compare = sys.argv[sys.argv.index("--compare") + 1]

    order = ["UpDataF", "UpDataR", "DownDataF", "DownDataR"]
    for label, p in (("NEW", path), ("OLD", compare)):
        if not p or not os.path.exists(p):
            continue
        print(f"=== {label}: {os.path.basename(p)} ===")
        sections = parse(p)
        # a trailing headerless chunk is whatever section didn't get its header through
        named = [s for s in sections if s[0]]
        orphans = [s for s in sections if not s[0] and s[2]]
        for i, s in enumerate(named):
            missing = f"  (dump claims {s[1]})" if s[1] != len(s[2]) else ""
            describe(s[0] + missing, s[2], s[0].startswith("Up"))
        for s in orphans:
            guess = [n for n in order if n not in [x[0] for x in named]]
            describe(f"{guess[0] if guess else '?'} (header lost, partial)", s[2],
                     bool(guess) and guess[0].startswith("Up"))
        print()


if __name__ == "__main__":
    main()
