"""Dataset maintenance for the PC eval harness.

datasets.h is the canonical home of the historical training datasets (the original six were
extracted from comment blocks that used to live in src/pressureMath.cpp). This script appends
new datasets from serial dump files when they aren't already present in datasets.h.

Usage: python extract_datasets.py
"""

import re
import os

HERE = os.path.dirname(__file__)
OUT = os.path.join(HERE, "datasets.h")

# serial dump captured from the corvette on 7/19/2026 (newer firmware, continuous timing values)
DUMP_FILE = os.path.join(HERE, "..", "ai weights for corvette 7.19.2026.txt")
DUMP_DATASETS = [
    ("/UpDataF.dat", "ds_up_front_car2026"),
    ("/UpDataR.dat", "ds_up_rear_car2026"),
    ("/DownDataF.dat", "ds_down_front_car2026"),
    ("/DownDataR.dat", "ds_down_rear_car2026"),
]

TUPLE_RE = re.compile(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}")


def main():
    with open(OUT, "r", encoding="utf-8") as f:
        existing = f.read()

    with open(DUMP_FILE, "r", encoding="utf-8") as f:
        dump_lines = f.readlines()

    additions = []
    for marker, name in DUMP_DATASETS:
        if name in existing:
            print(f"{name}: already present, skipping")
            continue
        samples = []
        for i, line in enumerate(dump_lines):
            if marker in line:
                # data is on the remainder of this line and/or the following line
                samples = TUPLE_RE.findall(line) or TUPLE_RE.findall(dump_lines[i + 1])
                break
        if not samples:
            raise SystemExit(f"dump marker not found or empty: {marker}")
        rows = ", ".join("{%s, %s, %s, %s}" % t for t in samples)
        additions.append(f"// From 'ai weights for corvette 7.19.2026.txt' serial dump ({marker})")
        additions.append(f"static const EvalSample {name}[] = {{{rows}}};")
        additions.append(f"static const int {name}_len = {len(samples)};")
        additions.append("")
        print(f"{name}: {len(samples)} samples")

    if additions:
        with open(OUT, "a", encoding="utf-8") as f:
            f.write("\n" + "\n".join(additions))
        print(f"appended to {OUT}")
    else:
        print("nothing to do")


if __name__ == "__main__":
    main()
