"""Generate test point sets for convex hull benchmarks.

Output format (one file per distribution and size):
    line 1:        number of points n
    lines 2..n+1:  x,y

Usage: python3 generate_points.py   (writes into the folder this script is in)
"""
import math
import os
import random

SIZES = {"1k": 1_000, "10k": 10_000, "100k": 100_000, "1m": 1_000_000}
CENTER, RADIUS = 500.0, 500.0


def square(rng):
    return rng.uniform(0, 1000), rng.uniform(0, 1000)


def disk(rng):
    r = RADIUS * math.sqrt(rng.random())  # sqrt -> uniform over the area
    a = rng.uniform(0, 2 * math.pi)
    return CENTER + r * math.cos(a), CENTER + r * math.sin(a)


def circle(rng):
    a = rng.uniform(0, 2 * math.pi)  # random order, not pre-sorted
    return CENTER + RADIUS * math.cos(a), CENTER + RADIUS * math.sin(a)


# Circle needs many decimals: neighbouring points are so close that rounding
# to fewer digits would push some of them inside the hull.
DISTRIBUTIONS = {"square": (square, 4), "disk": (disk, 4), "circle": (circle, 10)}


def main():
    out_dir = os.path.dirname(os.path.abspath(__file__))
    for name, (gen, decimals) in DISTRIBUTIONS.items():
        for label, n in SIZES.items():
            rng = random.Random(f"{name}-{n}")  # fixed seed per file
            path = os.path.join(out_dir, f"{name}_{label}.csv")
            with open(path, "w", newline="\n") as f:
                f.write(f"{n}\n")
                for _ in range(n):
                    x, y = gen(rng)
                    f.write(f"{x:.{decimals}f},{y:.{decimals}f}\n")
            print(f"wrote {path}")


if __name__ == "__main__":
    main()
