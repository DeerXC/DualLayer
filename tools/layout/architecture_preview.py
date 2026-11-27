"""architecture_preview.py - pictures of the buildings of tools/layout/architecture.py, drawn with the renderer and the materials of the
model (city_model.py): no game and no town needed. The pictures go to docs/images/architecture/.
    python tools/layout/architecture_preview.py              everything (about 4 minutes)
    python tools/layout/architecture_preview.py gallery      the six house types, two rows facing a street
    python tools/layout/architecture_preview.py landmarks    the market hall, the clock tower, the water tower, the chapel, the customs house
    python tools/layout/architecture_preview.py square       a plaza, a lane up a hill, a chapel and a water tower: the buildings together
Needs numpy and matplotlib (the model needs them too)."""
import inspect
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np

import architecture as ar
import city_model as cm

OUT = os.path.join(HERE, "..", "..", "docs", "images", "architecture")

# previews only: the renderer paints back to front by the middle of a polygon, so a window could be painted under the wall it lies on;
# a decal is drawn over its wall here (the game has a depth buffer and does not need this)
_src = inspect.getsource(cm.render)
_old = 'elif kind == "overlay":\n            depth += 0.8'
if _old in _src:
    exec(_src.replace(_old, 'elif kind == "overlay":\n            depth -= 0.9'), cm.__dict__)


def reset():
    cm.POLYS = []
    ar.set_emitter(lambda pts, mat, world="both", kind="solid": cm.POLYS.append((np.asarray(pts, float), mat, world, kind)))


def ground_quad(x0, x1, z0, z1, y=-0.14, mat="grass"):
    cm.POLYS.append((np.array([(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)], float), mat, "both", "terrain"))


def strip(x0, x1, z0, z1, y=-0.12, mat="paving"):
    cm.POLYS.append((np.array([(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)], float), mat, "both", "overlay"))


def shot(name, world, eye, tgt, fov, size, title):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    n = cm.render(world, path, eye, tgt, fov, size, title)
    print("  %s: %d polygons drawn" % (os.path.relpath(path), n), flush=True)


def gallery():
    reset()
    ground_quad(-60, 60, -50, 50)
    strip(-60, 60, -3.5, 3.5)
    kinds = ["row", "shop", "altana", "cottage", "portico", "palazzetto"]
    specs = []
    for i, k in enumerate(kinds):
        (w0, w1), (d0, d1), _ = ar.KINDS[k]
        specs.append((k, w0 + (w1 - w0) * ar.hash01(i, 1), d0 + (d1 - d0) * ar.hash01(i, 2)))
    cx = -34.0
    for i, (k, W, D) in enumerate(specs):
        ar.house(ar.Frame(cx + W / 2, 3.8, (1, 0), (0, 1), 0.0), k, W, D, 100 + i * 7, sides=(1, 1), back_free=True)
        cx += W + 1.2
    cx = -34.0
    for i, (k, W, D) in enumerate(specs[::-1]):
        st = ["whole", "unroofed", "ruin", "whole", "whole", "unroofed"][i]
        ar.house(ar.Frame(cx + W / 2, -3.8, (1, 0), (0, -1), 0.0), k, W, D, 300 + i * 11, state=st, sides=(1, 1), back_free=True)
        cx += W + 1.2
    shot("houses_A_aerial.png", "A", (0, 38, -62), (0, 2, 4), 42, (1500, 860), "A: six house types, two rows facing a street")
    shot("houses_A_street.png", "A", (-30, 1.7, -1.5), (-4, 3.5, 4.5), 62, (1500, 860), "A: from the street")
    shot("houses_B_aerial.png", "B", (0, 38, -62), (0, 2, 4), 42, (1500, 860), "B: the same, the south row damaged")


def landmarks():
    F0 = ar.Frame(0, 0, (1, 0), (0, 1), 0.0)

    def scene(build, views):
        reset()
        ground_quad(-70, 70, -70, 70)
        strip(-70, 70, -12, -2)
        build()
        for v in views:
            shot(*v)
    scene(lambda: ar.market_hall(F0, 15.0, 9.4), [("market_hall_A_front.png", "A", (-16, 5, -27), (0, 6, 5), 50, (1300, 800), "the market hall"),
                                                  ("market_hall_A_inside.png", "A", (-1.5, 1.7, -4), (2, 2.6, 6), 75, (1300, 800), "the market hall, inside")])
    scene(lambda: ar.clock_tower(F0, 5.6), [("clock_tower_A.png", "A", (-12, 4, -22), (0, 14, 2), 48, (1300, 800), "the clock tower"),
                                            ("clock_tower_A_up.png", "A", (7, 2, -9), (0, 18, 3), 62, (1300, 800), "the clock tower, from below")])

    def water():
        ar.water_tower(F0, 3.4)
        ar.aqueduct(ar.Frame(8, -2.5, (1, 0), (0, 1), 0.0), 30.0, 7.0)
    scene(water, [("water_tower_A.png", "A", (-14, 5, -26), (4, 10, 0), 48, (1300, 800), "the water tower and the aqueduct")])
    scene(lambda: ar.chapel(F0, 5.4, 8.2), [("chapel_A_front.png", "A", (-9, 3, -14), (0, 4, 4), 52, (1300, 800), "the chapel"),
                                            ("chapel_A_inside.png", "A", (0.2, 1.7, -0.6), (0, 1.5, 8), 80, (1300, 800), "the chapel, inside")])
    scene(lambda: ar.dogana(F0, 26.0, 9.0), [("customs_house_A.png", "A", (-22, 6, -26), (2, 5, 2), 50, (1300, 800), "the customs house"),
                                             ("customs_house_B.png", "B", (-22, 6, -26), (2, 5, 2), 50, (1300, 800), "the customs house, B")])


def square():
    reset()

    def g(x, z):
        return 0.24 * max(0.0, z - 12.0) + (0.6 * math.sin(x / 11.0) if z > 12 else 0.0)
    for xi in range(-80, 80, 4):
        for zi in range(-46, 70, 4):
            pts = [(xi, g(xi, zi), zi), (xi + 4, g(xi + 4, zi), zi), (xi + 4, g(xi + 4, zi + 4), zi + 4), (xi, g(xi, zi + 4), zi + 4)]
            cm.POLYS.append((np.array(pts, float), "grass" if zi > 14 else "sand", "both", "terrain"))
    for xi in range(-30, 30, 4):
        for zi in range(-26, 10, 4):
            strip(xi, xi + 4, zi, zi + 4, -0.1)

    def ribbon(curve, w):
        ss = np.arange(0, ar.curve_lengths(curve)[-1], 2.0)
        pts = []
        for s in ss:
            p, v = ar.curve_at(curve, s)
            n = np.array([-v[1], v[0]])
            pts.append((p + n * w / 2, p - n * w / 2))
        for i in range(len(pts) - 1):
            a, b = pts[i]
            c, d = pts[i + 1]
            cm.POLYS.append((np.array([(a[0], g(*a) + 0.05, a[1]), (c[0], g(*c) + 0.05, c[1]), (d[0], g(*d) + 0.05, d[1]), (b[0], g(*b) + 0.05, b[1])], float),
                             "paving", "both", "overlay"))
    occupied = [(-8.0, 12.7, 12.0), (26.8, 0.8, 7.0)]

    def accept(outline):
        return not any(math.hypot(x - qx, z - qz) < qr for x, z in outline for qx, qz, qr in occupied)
    ar.market_hall(ar.Frame(-8.0, 8.0, (1, 0), (0, 1), 0.0), 15.0, 9.4)
    ar.clock_tower(ar.Frame(24.0, -2.0, (0, 1), (1, 0), 0.0), 5.6)
    Ff = ar.Frame(8, -10, (1, 0), (0, 1), 0)
    ar.fdrum(Ff, 0, 0, 1.8, -0.1, 0.7, "stone", "both", 12, "stone_dark")
    ar.fdrum(Ff, 0, 0, 0.35, 0.7, 2.4, "stone", "both", 8, "stone")
    cm.POLYS.append((np.array([(8 + 1.5 * math.cos(t), 0.62, -10 + 1.5 * math.sin(t)) for t in np.linspace(0, 2 * math.pi, 13)[:-1]], float), "water", "A", "water"))
    ar.plan_row(np.array([(-30.0, 8.0), (-30.0, -26.0)]), -1, 0.0, 5, "hub", None, g, None, 0.5)
    ar.plan_row(np.array([(-30.0, -28.0), (22.0, -28.0)]), -1, 0.0, 6, "hub", None, g, None, 0.5)
    lane = np.array([(30.0, -2.0), (40.0, 4.0), (46.0, 16.0), (40.0, 30.0), (30.0, 40.0), (18.0, 48.0)])
    ribbon(lane, 4.2)
    ar.plan_row(lane, 1, 2.1, 7, "terrace", accept, g, None, 8.0)
    ar.plan_row(lane, -1, 2.1, 8, "terrace", accept, g, None, 8.0)
    ar.chapel(ar.Frame(8.0, 55.0, (1, 0), (0, 1), g(8, 55)), 5.4, 8.2)
    ar.water_tower(ar.Frame(66.0, 20.0, (1, 0), (0, 1), g(66, 20)), 3.4)
    ar.aqueduct(ar.Frame(70.0, 14.0, (1, 0), (0, 1), g(66, 20) - 2.0), 40.0, 8.0)
    shot("square_A_aerial.png", "A", (-52, 46, -78), (10, 4, 14), 44, (1600, 900), "A: a plaza, a lane up the hill, a chapel, a water tower")
    shot("square_A_plaza.png", "A", (14, 1.8, -23), (-8, 6.5, 6), 66, (1600, 900), "A: the plaza, looking at the market hall")
    shot("square_B_aerial.png", "B", (-52, 46, -78), (10, 4, 14), 44, (1600, 900), "B: the same")


if __name__ == "__main__":
    which = sys.argv[1:] or ["gallery", "landmarks", "square"]
    for w in which:
        {"gallery": gallery, "landmarks": landmarks, "square": square}[w]()
