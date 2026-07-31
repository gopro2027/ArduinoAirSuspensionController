"""Dataset maintenance for the PC eval harness.

datasets.h is the canonical home of the historical training datasets (the original six were
extracted from comment blocks that used to live in src/pressureMath.cpp). This script appends
new datasets from serial dump files when they aren't already present in datasets.h.

Sections are taken positionally (UpF, UpR, DownF, DownR) rather than by header, because long dumps
routinely lose a header line to serial overflow.

Usage:
  python extract_datasets.py                              # re-check the built-in dumps
  python extract_datasets.py "<dump file>" <name_suffix>  # add a new dump
"""

import re
import sys
import os

HERE = os.path.dirname(__file__)
OUT = os.path.join(HERE, "datasets.h")

# schema 3 dumps carry a 5th value (others_flowing); older dumps have 4. EvalSample zero-fills the
# missing member, so both shapes can be emitted verbatim.
TUPLE_RE = re.compile(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(?:\s*,\s*(\d+))?\s*\}")

SECTION_ORDER = ["up_front", "up_rear", "down_front", "down_rear"]

BUILTIN = [
    (os.path.join(HERE, "..", "ai weights for corvette 7.19.2026.txt"), "car2026"),
]


def sections_from(path):
    """Split a dump into the four per-model sample lists, positionally."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    s, e = text.find("BEGIN IMPORTANT DATA FOR PRO"), text.find("END IMPORTANT DATA FOR PRO")
    body = text[s:e] if s >= 0 and e > s else text

    groups = []
    for line in body.splitlines():
        rows = TUPLE_RE.findall(line)
        if rows:
            groups.append(rows)
    if len(groups) != 4:
        print(f"  note: found {len(groups)} data lines, expected 4 "
              f"(sizes {[len(g) for g in groups]}) - a header or chunk was likely lost in serial")
    return groups


def drop(text, name):
    """Remove a previously appended dataset block so a corrected dump can replace it."""
    keep, removed = [], 0
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if f"{name}[]" in line or f"{name}_len" in line:
            removed += 1
            if keep and keep[-1].startswith("// From "):
                keep.pop()
            continue
        keep.append(line)
    if removed:
        print(f"{name}: replacing existing entry")
    return "\n".join(keep)


def add(path, suffix, existing, additions):
    groups = sections_from(path)
    for i, rows in enumerate(groups[:4]):
        name = f"ds_{SECTION_ORDER[i]}_{suffix}"
        if name in existing:
            existing = drop(existing, name)
            with open(OUT, "w", encoding="utf-8") as f:
                f.write(existing)
        body = ", ".join("{%s}" % ", ".join(v for v in t if v) for t in rows)
        additions.append(f"// From '{os.path.basename(path)}' serial dump ({SECTION_ORDER[i]})")
        additions.append(f"static const EvalSample {name}[] = {{{body}}};")
        additions.append(f"static const int {name}_len = {len(rows)};")
        additions.append("")
        print(f"{name}: {len(rows)} samples")


def main():
    with open(OUT, "r", encoding="utf-8") as f:
        existing = f.read()

    jobs = BUILTIN if len(sys.argv) < 3 else [(sys.argv[1], sys.argv[2])]
    additions = []
    for path, suffix in jobs:
        if not os.path.exists(path):
            raise SystemExit(f"dump not found: {path}")
        add(path, suffix, existing, additions)

    if additions:
        with open(OUT, "a", encoding="utf-8") as f:
            f.write("\n" + "\n".join(additions))
        print(f"appended to {OUT}")
    else:
        print("nothing to do")


if __name__ == "__main__":
    main()
