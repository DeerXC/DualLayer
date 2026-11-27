"""architecture_check.py - the checks of tools/layout/architecture.py (no game, no model): every building type is built many times with
different sizes and seeds, and what comes out is checked against the rules of the game (docs/DESIGN.md 7.10):
  * every polygon has 3 to 255 vertices, is planar and convex (the game cuts a polygon into a fan of triangles);
  * every material is one the town has (MATERIALS below, taken from city_model.MAT when it can be imported);
  * the proportions of a house: width of the front at least 0.8 times the height of its eaves (the old houses were 0.4 to 1.0);
  * no foothold: an upward face of a solid, more than 0.1 m2, between 1.2 m and 3.0 m up (a jump is 1.65 m, a crate 1.2 m more: 2.85 m), does
    not stand out of the house by more than 0.3 m. (A step, a bench or a counter below 1.2 m leads nowhere: the next foothold would have to be
    within 1.65 m of it and there is none.)
Run:  python tools/layout/architecture_check.py        (exit code 1 when a check fails)"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import architecture as ar

try:
    import town_style                                                          # the palette of the town (numpy only)
    MATERIALS = set(town_style.PALETTE)
except Exception:                                                              # without it: the names of the materials the library uses
    MATERIALS = {"stone", "stone_dark", "rock", "paving", "sand", "debris", "plaster0", "plaster1", "plaster2", "plaster3", "plaster4", "plaster5",
                 "plaster6", "plaster7", "chimney", "roof", "roof2", "roof3", "roof4", "copper", "shutter", "shutter2", "shutter3", "cloth",
                 "cloth2", "cloth3", "cloth4", "bloom", "wood", "door", "iron", "grass", "field", "tree", "cypress", "moss", "water", "sea",
                 "gold", "window", "lantern", "memory", "glow"}
failed = 0
checks = 0


def check(cond, what):
    global failed, checks
    checks += 1
    if not cond:
        failed += 1
        print("FAIL:", what)


def convex_planar(P):
    n = len(P)
    if n < 3 or n > 255:
        return False, "vertex count %d" % n
    N = np.cross(P[1] - P[0], P[2] - P[0])
    for i in range(1, n - 1):
        N2 = np.cross(P[i] - P[0], P[i + 1] - P[0])
        if np.linalg.norm(N2) > 1e-9 and np.linalg.norm(N) < 1e-9:
            N = N2
    ln = float(np.linalg.norm(N))
    if ln < 1e-9:
        return True, ""                                                        # a degenerate sliver: harmless (the exporter skips it)
    N = N / ln
    if float(np.abs((P - P[0]) @ N).max()) > 2e-3:
        return False, "not planar"
    ax = int(np.argmax(np.abs(N)))
    Q = np.delete(P, ax, axis=1)
    sign = 0
    for i in range(n):
        a, b, c = Q[i], Q[(i + 1) % n], Q[(i + 2) % n]
        z = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
        if abs(z) > 1e-6:
            sg = 1 if z > 0 else -1
            if sign == 0:
                sign = sg
            elif sg != sign:
                return False, "not convex"
    return True, ""


def build(fn):
    out = []
    ar.set_emitter(lambda pts, mat, world="both", kind="solid": out.append((np.asarray(pts, float), mat, world, kind)))
    fn()
    return out


def audit(polys, name):
    bad = 0
    for P, mat, world, kind in polys:
        ok, why = convex_planar(P)
        if not ok:
            bad += 1
            if bad <= 3:
                print("   %s: %s %s %s" % (name, why, mat, np.round(P[:4], 2).tolist()))
        if mat not in MATERIALS:
            bad += 1
            print("   %s: unknown material %r" % (name, mat))
        if world not in ("both", "A", "B") or kind not in ("solid", "overlay", "water", "terrain"):
            bad += 1
            print("   %s: bad world or kind %r %r" % (name, world, kind))
    check(bad == 0, "%s: %d bad polygons" % (name, bad))


F0 = ar.Frame(0, 0, (1, 0), (0, 1), 0.0)
print("houses:")
for kind, ((w0, w1), (d0, d1), (f0, f1)) in ar.KINDS.items():
    counts = []
    ratios = []
    for i in range(40):
        W = w0 + (w1 - w0) * ar.hash01(i, 1)
        D = d0 + (d1 - d0) * ar.hash01(i, 2)
        st = ("whole", "whole", "unroofed", "ruin")[i % 4]
        fl = None if i % 3 else f1
        box = {}
        polys = build(lambda: box.update(E=ar.house(F0, kind, W, D, 500 + i * 13, fl, st, None, (1, 1), -1.5, True)[0]))
        audit(polys, "%s #%d" % (kind, i))
        counts.append((len(polys), sum(1 for p in polys if p[3] == "solid")))
        ratios.append(W / box["E"])
        # a foothold: an upward face of a solid, at most 3.0 m up, more than 0.1 m2, that goes more than 0.3 m outside the footprint
        far = 0
        for P, mat, world, kd in polys:
            if kd != "solid" or len(P) < 3:
                continue
            N = np.cross(P[1] - P[0], P[2] - P[0])
            ln = float(np.linalg.norm(N))
            if ln < 1e-9 or -N[1] / ln < 0.7:                                  # the engine: the cross product points into the solid
                continue
            top = float(P[:, 1].mean())
            if top < 1.2 or top >= 3.0:
                continue                                                       # a step, a bench or a counter (reach 1.65 m: no second step), or out of reach
            area = 0.5 * ln if len(P) == 3 else 0.5 * sum(float(np.linalg.norm(np.cross(P[k] - P[0], P[k + 1] - P[0]))) for k in range(1, len(P) - 1))
            a, b = P[:, 0], P[:, 2]
            out = max(np.max(np.abs(a) - W / 2), np.max(-b), np.max(b - D))
            if out > 0.3 and area > 0.1:
                far += 1
        check(far == 0, "%s #%d: %d upward faces between 1.2 m and 3.0 m stand out of the house by more than 0.3 m (a foothold)" % (kind, i, far))
    check(min(ratios) >= 0.8, "%s: width over eaves height is %.2f, below 0.8" % (kind, min(ratios)))
    print("  %-11s polygons %3d to %3d (solid %2d to %3d), width / eaves %.2f to %.2f" % (kind, min(c[0] for c in counts), max(c[0] for c in counts),
          min(c[1] for c in counts), max(c[1] for c in counts), min(ratios), max(ratios)))
print("landmarks:")
for name, fn in (("market hall", lambda: ar.market_hall(F0, 15.0, 9.4, "whole")), ("market hall (ruin)", lambda: ar.market_hall(F0, 15.0, 9.4, "ruin")),
                 ("clock tower", lambda: ar.clock_tower(F0, 5.6, "whole")), ("water tower", lambda: ar.water_tower(F0, 3.4, "whole")),
                 ("aqueduct", lambda: ar.aqueduct(F0, 30.0, 7.0)), ("chapel", lambda: ar.chapel(F0, 5.4, 8.2, "whole")),
                 ("chapel (ruin)", lambda: ar.chapel(F0, 5.4, 8.2, "ruin")), ("customs house", lambda: ar.dogana(F0, 26.0, 9.0, "whole"))):
    polys = build(fn)
    audit(polys, name)
    top = max(P[:, 1].max() for P, *_ in polys)
    print("  %-18s polygons %4d (solid %4d), %.1f m high" % (name, len(polys), sum(1 for p in polys if p[3] == "solid"), top))
print("\n%d checks, %d failed" % (checks, failed))
sys.exit(1 if failed else 0)
