"""city_model.py - the model of the map, version 2 (docs/LEVEL_DESIGN.md 5, ART_AND_MAP_PLAN_v2.md).

A design model, not game data (yet): the town, its ground and its two worlds, drawn so that the plan can be judged before it
is moved into the game.

The town: one hill over a harbour, climbed in a spiral round the Rock of the Bell.
    sea      -8    the harbour (start), the headland and its lighthouse
    low      -4..0 the lower town, behind the sea wall
    centre    8    the Rock of the Bell: a round plaza on a rock, the bell tower; round it a gorge, the open part of the
                   cistern: a moat of water in A, a dry gorge in B; two bridges cross it
    west     0..8  the market terraces round the great well
    east     4..18 the lock stairs: five locks climbing the hill to the water tower
    north   16..20 the upper town, a ring street round the theatre, which faces the rock
    top      28    the graveyard on the cliff, the night bell
Shortcuts that loop back: the lift down the face of the rock to the gorge and the lower town; the gorge (B) under both
bridges; the upper ring street back down to the market.

The worlds (the art style): A, 7:11, warm dusk: amber light from a low sun in the west, violet shade, terracotta and copper,
lit windows. B, now, cold ruin: moonlight from high, blue-grey stone, slate, verdigris and moss; the only warm colours are a
few lanterns. The storm came from the sea: the lower town is in ruins in B, the upper town stands.

    python tools/layout/city_model.py           (all views; Python 3, numpy, matplotlib; about a minute each)
    python tools/layout/city_model.py aerial_a  (one view: aerial_a, aerial_b, street_a, street_b, plan)

Units: metres; x east, z north, y up. Views are drawn with a painter's algorithm (polygons sorted far to near) and flat
shading, the look the game has: one colour per face, an outline on every edge.
"""

import math
import os
import sys

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.collections import PolyCollection  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "images")
SEA = -8.0

# ---- the art style: materials in both worlds (sRGB albedo), and the light of each world ------------------------------------
MAT = {
    #  name           A                      B
    "stone": ((0.93, 0.80, 0.64), (0.62, 0.68, 0.76)),
    "stone_dark": ((0.74, 0.55, 0.44), (0.36, 0.42, 0.50)),
    "rock": ((0.74, 0.58, 0.47), (0.36, 0.40, 0.47)),
    "paving": ((0.88, 0.78, 0.63), (0.50, 0.55, 0.60)),
    "roof": ((0.78, 0.32, 0.20), (0.30, 0.35, 0.43)),
    "roof2": ((0.86, 0.48, 0.30), (0.34, 0.38, 0.46)),
    "copper": ((0.24, 0.56, 0.50), (0.37, 0.63, 0.58)),
    "gold": ((0.96, 0.70, 0.26), (0.62, 0.50, 0.30)),
    "wood": ((0.56, 0.36, 0.22), (0.27, 0.27, 0.29)),
    "cloth": ((0.82, 0.30, 0.24), (0.30, 0.30, 0.34)),
    "cloth2": ((0.92, 0.72, 0.32), (0.30, 0.30, 0.34)),
    "grass": ((0.52, 0.62, 0.34), (0.24, 0.40, 0.30)),
    "field": ((0.84, 0.66, 0.34), (0.30, 0.38, 0.32)),
    "tree": ((0.40, 0.48, 0.26), (0.18, 0.28, 0.26)),
    "cypress": ((0.26, 0.36, 0.22), (0.14, 0.22, 0.22)),
    "moss": ((0.50, 0.52, 0.30), (0.26, 0.46, 0.33)),
    "sand": ((0.92, 0.80, 0.60), (0.48, 0.52, 0.54)),
    "debris": ((0.60, 0.50, 0.44), (0.32, 0.35, 0.40)),
    "water": ((0.20, 0.56, 0.68), (0.22, 0.34, 0.42)),
    "sea": ((0.22, 0.44, 0.66), (0.12, 0.20, 0.31)),
    "iron": ((0.30, 0.28, 0.30), (0.20, 0.22, 0.26)),
    "plaster0": ((0.95, 0.84, 0.68), (0.56, 0.62, 0.69)),
    "plaster1": ((0.92, 0.72, 0.54), (0.52, 0.58, 0.66)),
    "plaster2": ((0.86, 0.58, 0.44), (0.48, 0.54, 0.62)),
    "plaster3": ((0.96, 0.90, 0.78), (0.60, 0.66, 0.72)),
    "window": ((1.00, 0.82, 0.46), (0.10, 0.12, 0.16)),   # lit in A, dark in B
    "lantern": ((1.00, 0.82, 0.46), (1.00, 0.70, 0.36)),  # the one warm light of B
    "memory": ((0.80, 0.95, 1.00), (0.62, 0.92, 1.00)),
    "glow": ((1.00, 0.80, 0.46), (0.78, 0.58, 0.36)),     # the warm light a lantern throws on the ground
    "door": ((0.42, 0.24, 0.16), (0.16, 0.18, 0.22)),
    "shutter": ((0.30, 0.48, 0.44), (0.24, 0.30, 0.34)),
    "chimney": ((0.82, 0.62, 0.50), (0.42, 0.47, 0.55)),
}
EMISSIVE = {"window": (True, False), "lantern": (True, True), "memory": (True, True), "glow": (True, True)}
LIGHT = {
    #     to the light (sun / moon)      its colour            the shade (ambient)    sky top            horizon (fog)    fog
    "A": ((-0.70, 0.40, -0.58), (1.06, 0.86, 0.64), (0.36, 0.27, 0.52), (0.74, 0.48, 0.52), (0.99, 0.80, 0.62), 0.0020),
    "B": ((-0.30, 0.80, -0.52), (0.86, 0.96, 1.15), (0.24, 0.30, 0.45), (0.14, 0.19, 0.33), (0.40, 0.48, 0.60), 0.0040),
}


def ss(e0, e1, v):
    t = np.clip((np.asarray(v, float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def hash01(*k):
    h = 2166136261
    for v in k:
        h = ((h ^ (int(v) & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
    h = ((h ^ (h >> 15)) * 2246822519) & 0xFFFFFFFF
    return (h & 0xFFFF) / 65535.0


def value_noise(x, z, seed=0):
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    ix, iz = np.floor(x).astype(np.int64), np.floor(z).astype(np.int64)
    fx, fz = x - ix, z - iz
    ux, uz = fx * fx * (3 - 2 * fx), fz * fz * (3 - 2 * fz)

    def h(a, b):
        v = (a * 374761393 + b * 668265263 + seed * 1442695041) & 0xFFFFFFFF
        v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
        return (v & 0xFFFF) / 65535.0

    a, b, c, d = h(ix, iz), h(ix + 1, iz), h(ix, iz + 1), h(ix + 1, iz + 1)
    return 2.0 * (a + (b - a) * ux + (c - a) * uz + (a - b - c + d) * ux * uz) - 1.0


def fbm(x, z, octaves=4, seed=0):
    total, amp, freq, norm = 0.0, 1.0, 1.0, 0.0
    for o in range(octaves):
        total = total + amp * value_noise(x * freq, z * freq, seed + o * 17)
        norm += amp
        amp *= 0.5
        freq *= 2.03
    return total / norm


# ---- curves -----------------------------------------------------------------------------------------------------------------

def spline(points, step=1.0):
    p = [np.array(points[0], float)] + [np.array(q, float) for q in points] + [np.array(points[-1], float)]
    out = []
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = p[i - 1], p[i], p[i + 1], p[i + 2]
        n = max(2, int(np.linalg.norm(p2 - p1) / step))
        for k in range(n):
            t = k / n
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    out.append(p[-2])
    return np.array(out)


def lengths(c):
    return np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(c, axis=0), axis=1))]


def at(c, t):
    d = lengths(c)
    s = t * d[-1]
    i = int(np.clip(np.searchsorted(d, s) - 1, 0, len(c) - 2))
    u = (s - d[i]) / max(d[i + 1] - d[i], 1e-9)
    v = c[i + 1] - c[i]
    return c[i] + u * v, v / max(np.linalg.norm(v), 1e-9)


def distance_to(c, x, z):
    px = np.asarray(x, float)[..., None]
    pz = np.asarray(z, float)[..., None]
    a, b = c[:-1], c[1:]
    ab = b - a
    l2 = np.maximum((ab ** 2).sum(1), 1e-9)
    t = np.clip(((px - a[:, 0]) * ab[:, 0] + (pz - a[:, 1]) * ab[:, 1]) / l2, 0, 1)
    d = np.hypot(px - (a[:, 0] + t * ab[:, 0]), pz - (a[:, 1] + t * ab[:, 1]))
    return d.min(-1), d.argmin(-1)


# ---- the plan ---------------------------------------------------------------------------------------------------------------
ROCK_R = 13.5          # the plaza on the Rock of the Bell, at y ROCK_Y
ROCK_Y = 14.0
GORGE_R = 23.0         # the gorge round it: floor at GORGE_Y; water at MOAT_Y in A
GORGE_Y = -6.0
MOAT_Y = -3.6
TOWER = (0.0, 3.0)
WELL = (-50.0, 6.0)
WELL_R = 6.0
THEATRE = (0.0, 38.0)  # the centre of the orchestra; the seats rise north, facing the rock
THEATRE_Y = 20.0
GRAVE_Y = 44.0
BRIDGEHEADS = {"west": (-27.0, 0.0), "east": (27.0, 0.0)}
LOCK_LINE = spline([(36, -16), (48, -19), (60, -12), (70, -1), (77, 10), (80, 22)], 1.0)
LOCK_LEVELS = [3.0, 7.5, 12.0, 16.5, 21.0]
HARBOUR = (-22.0, -96.0, 34.0, -70.0)  # the basin: x0, z0, x1, z1
QUAY_Y = -6.2

STREETS = [
    # name, points, width, kind ("street", "stair", "lane")
    ("sea stair", [(8, -71), (6, -66), (2, -61), (-1, -56)], 3.0, "stair"),
    ("lower street", [(-1, -56), (-14, -51), (-28, -47), (-40, -42), (-50, -35)], 4.0, "street"),
    ("lower east lane", [(-1, -56), (14, -53), (28, -46), (36, -34), (37, -20)], 3.0, "lane"),
    ("market stairs", [(-50, -35), (-58, -24), (-61, -11), (-58, 2), (-54, 14)], 4.0, "stair"),
    ("market street", [(-61, -11), (-46, -7), (-35, -3), (-27, 0)], 4.6, "street"),
    ("east approach", [(27, 0), (33, -5), (37, -14)], 3.6, "street"),
    ("upper ring", [(80, 26), (66, 36), (52, 46), (34, 52), (16, 56), (-4, 57), (-24, 54), (-40, 46), (-50, 34), (-54, 14)], 4.0, "street"),
    ("cliff stair", [(10, 57), (16, 61), (8, 64), (15, 67), (6, 71)], 2.6, "stair"),
    ("harbour road", [(-22, -73), (0, -72), (34, -72)], 4.0, "lane"),
    ("headland path", [(-22, -73), (-44, -78), (-66, -84), (-86, -86)], 2.4, "lane"),
]
STREET = {n: (spline(p), w, k) for n, p, w, k in STREETS}
WALL_POINTS = [(0, -62), (30, -63), (60, -50), (88, -22), (96, 12), (88, 38), (62, 58), (30, 64), (-20, 65), (-52, 58), (-74, 38),
               (-84, 4), (-80, -32), (-58, -56), (-28, -63), (0, -62)]
WALL = spline(WALL_POINTS, 2.0)


def inside_wall(x, z):
    c = False
    for i in range(len(WALL) - 1):
        (ax, az), (bx, bz) = WALL[i], WALL[i + 1]
        if (az > z) != (bz > z) and x < (bx - ax) * (z - az) / (bz - az) + ax:
            c = not c
    return c


# ---- the ground -------------------------------------------------------------------------------------------------------------

def coast_z(x):
    x = np.asarray(x, float)
    z = -76.0 + 4.0 * fbm(x / 26.0, 3.1, 3, 7)
    z = z - 24.0 * ss(-58.0, -96.0, x)          # the headland reaches out to sea in the west
    z = z + 8.0 * ss(70.0, 120.0, x)
    return z


def natural(x, z):
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    wz = z + 6.0 * fbm(x / 50.0, z / 50.0, 2, 4)
    y = np.interp(wz, [-140, -86, -72, -58, -40, -20, 10, 30, 50, 62, 70, 100, 160], [-24, -11, -8, -3, 1, 5, 10, 22, 28, 32, 46, 56, 80])
    y = y + 10.0 * ss(20.0, 85.0, x) * ss(-60.0, -20.0, z) * ss(60.0, 20.0, z)          # the east hill
    y = y + 16.0 * np.exp(-((x + 80.0) ** 2 / 600.0 + (z + 78.0) ** 2 / 200.0))        # the headland
    y = y + 18.0 * ss(110.0, 170.0, np.abs(x)) * ss(-50.0, 40.0, z)                     # hills to either side
    town = ss(110.0, 80.0, np.hypot(x - 4.0, z))
    y = y + (2.0 - 1.6 * town) * fbm(x / 22.0, z / 22.0, 4, 1)
    q = np.floor(y / 3.0)                                                                 # terraces of 3 m in town, held by
    stepped = (q + ss(0.85, 1.0, y / 3.0 - q)) * 3.0                                      # retaining walls
    y = y + (stepped - y) * town
    cz = coast_z(x)
    y = np.where(z < cz + 18.0, np.minimum(y, SEA + 0.4 * (z - cz)), y)
    return np.maximum(y, SEA - 16.0)


def lock_level(i):
    return LOCK_LEVELS[min(max(i, 0), len(LOCK_LEVELS) - 1)]


def lock_index(s):
    """The lock at distance s along LOCK_LINE (five equal basins)."""
    total = lengths(LOCK_LINE)[-1]
    return int(np.clip(s / total * len(LOCK_LEVELS), 0, len(LOCK_LEVELS) - 1))


def street_heights(curve, ground_before):
    h = ground_before(curve[:, 0], curve[:, 1])
    k = np.ones(9) / 9.0
    h = np.convolve(np.r_[np.full(4, h[0]), h, np.full(4, h[-1])], k, mode="valid")
    ds = np.maximum(np.linalg.norm(np.diff(curve, axis=0), axis=1), 1e-6)
    for _ in range(4):
        for i in range(1, len(h)):
            h[i] = np.clip(h[i], h[i - 1] - 0.55 * ds[i - 1], h[i - 1] + 0.55 * ds[i - 1])
        for i in range(len(h) - 2, -1, -1):
            h[i] = np.clip(h[i], h[i + 1] - 0.55 * ds[i], h[i + 1] + 0.55 * ds[i])
    return h


def carved(x, z):
    """The ground with the places of the town cut into it (the streets come after)."""
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    y = natural(x, z)
    r = np.hypot(x, z)
    y = np.where(r < GORGE_R + 1.5, np.minimum(y, GORGE_Y + (y - GORGE_Y) * ss(GORGE_R, GORGE_R + 1.5, r)), y)  # the gorge
    y = np.where(r < ROCK_R + 0.8, GORGE_Y + (ROCK_Y - GORGE_Y) * ss(ROCK_R + 0.8, ROCK_R, r), y)             # the rock
    for bx, bz in BRIDGEHEADS.values():                                                                         # the bastions
        y = np.where(np.hypot(x - bx, z - bz) < 4.5, ROCK_Y, y)
    d, i = distance_to(LOCK_LINE, x, z)                                                                         # the locks
    s = lengths(LOCK_LINE)[np.minimum(i, len(LOCK_LINE) - 1)]
    lv = np.array([lock_level(lock_index(v)) for v in np.ravel(s)]).reshape(np.shape(s))
    y = np.where(d < 4.0, lv - 4.0, np.where(d < 8.5, lv, y))                                                 # the basins and quays
    y = np.where(np.hypot(x - WELL[0], z - WELL[1]) < WELL_R + 5.0, 6.0, y)                                    # the market of the well
    rt = np.hypot(x - THEATRE[0], z - THEATRE[1])                                                               # the theatre
    bowl = (z > THEATRE[1] - 3.0) & (rt < 17.0)
    seats = np.where(rt < 6.0, THEATRE_Y, THEATRE_Y + 0.9 * np.minimum(np.ceil((rt - 6.0) / 1.3), 9.0))
    y = np.where(bowl, seats, y)
    y = np.where((np.abs(x - THEATRE[0]) < 10.0) & (z > THEATRE[1] - 9.0) & (z <= THEATRE[1] - 3.0), THEATRE_Y, y)   # the stage terrace
    y = np.where((np.abs(x) < 24.0) & (z > 70.0) & (z < 90.0), GRAVE_Y, y)                                     # the graveyard
    x0, z0, x1, z1 = HARBOUR
    quay = (x > x0 - 4.0) & (x < x1 + 4.0) & (z > z0 + 18.0) & (z < z1 + 4.0)
    y = np.where(quay, QUAY_Y, y)
    return y


_HEIGHTS = {name: street_heights(c, carved) for name, (c, w, k) in STREET.items()}


def ground(x, z):
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    y = carved(x, z)
    r = np.hypot(x, z)
    for name, (c, w, k) in STREET.items():
        d, i = distance_to(c, x, z)
        hs = _HEIGHTS[name][i]
        wgt = (1.0 - ss(w / 2 + 0.4, w / 2 + 3.0, d)) * (r > GORGE_R + 1.0)
        y = y + (hs - y) * wgt
    return y


def damage(x, z):
    """0 .. 1: how hard the storm from the sea hit: the sea front most, the lower town much, the upper town little."""
    return float(np.clip(0.95 * ss(-10.0, -62.0, z) + 0.15 * fbm(x / 15.0, z / 15.0, 2, 33), 0.0, 1.0))


# ---- polygons ---------------------------------------------------------------------------------------------------------------
POLYS = []  # (points (n, 3), material, world "both"/"A"/"B", kind "solid"/"terrain"/"water"/"overlay")


def poly(points, mat, world="both", kind="solid"):
    POLYS.append((np.asarray(points, float), mat, world, kind))


def ngon(cx, cz, r, n, phase=0.0):
    return [(cx + r * math.cos(phase + 2 * math.pi * k / n), cz + r * math.sin(phase + 2 * math.pi * k / n)) for k in range(n)]


def rect(cx, cz, w, d, a):
    c, s = math.cos(a), math.sin(a)
    return [(cx + ux * c - uz * s, cz + ux * s + uz * c) for ux, uz in ((-w / 2, -d / 2), (w / 2, -d / 2), (w / 2, d / 2), (-w / 2, d / 2))]


def prism(outline, y0, y1, mat, world="both", top=True, top_mat=None):
    n = len(outline)
    for i in range(n):
        (ax, az), (bx, bz) = outline[i], outline[(i + 1) % n]
        poly([(ax, y0, az), (bx, y0, bz), (bx, y1, bz), (ax, y1, az)], mat, world)
    if top:
        poly([(x, y1, z) for x, z in outline], top_mat or mat, world)


def gable(cx, cz, w, d, a, y0, rise, mat, world="both"):
    c, s = math.cos(a), math.sin(a)

    def p(ux, uz, y):
        return (cx + ux * c - uz * s, y, cz + ux * s + uz * c)

    hw, hd = w / 2 + 0.3, d / 2 + 0.3
    poly([p(-hw, -hd, y0), p(-hw, hd, y0), p(0, hd, y0 + rise), p(0, -hd, y0 + rise)], mat, world)
    poly([p(hw, -hd, y0), p(hw, hd, y0), p(0, hd, y0 + rise), p(0, -hd, y0 + rise)], mat, world)
    poly([p(-hw, -hd, y0), p(hw, -hd, y0), p(0, -hd, y0 + rise)], mat, world)
    poly([p(-hw, hd, y0), p(hw, hd, y0), p(0, hd, y0 + rise)], mat, world)


def cone(cx, cz, r, y0, y1, mat, world="both", n=8):
    ring = ngon(cx, cz, r, n)
    for i in range(n):
        (ax, az), (bx, bz) = ring[i], ring[(i + 1) % n]
        poly([(ax, y0, az), (bx, y0, bz), (cx, y1, cz)], mat, world)


def dome(cx, cz, r, y0, mat, world="both", n=12, bands=4):
    for b in range(bands):
        a0 = math.pi / 2 * b / bands
        a1 = math.pi / 2 * (b + 1) / bands
        r0, r1 = r * math.cos(a0), r * math.cos(a1)
        h0, h1 = y0 + r * math.sin(a0), y0 + r * math.sin(a1)
        for k in range(n):
            t0, t1 = 2 * math.pi * k / n, 2 * math.pi * (k + 1) / n
            pts = [(cx + r0 * math.cos(t0), h0, cz + r0 * math.sin(t0)), (cx + r0 * math.cos(t1), h0, cz + r0 * math.sin(t1)),
                   (cx + r1 * math.cos(t1), h1, cz + r1 * math.sin(t1))]
            if r1 > 1e-3:
                pts.append((cx + r1 * math.cos(t0), h1, cz + r1 * math.sin(t0)))
            poly(pts, mat, world)


def ribbon(curve, heights, width, mat, world="both", side=0.0, kind="overlay"):
    for i in range(len(curve) - 1):
        a, b = curve[i], curve[i + 1]
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * width / 2, v[0] * width / 2
        ya, yb = heights[i], heights[i + 1]
        poly([(a[0] + nx, ya, a[1] + nz), (b[0] + nx, yb, b[1] + nz), (b[0] - nx, yb, b[1] - nz), (a[0] - nx, ya, a[1] - nz)], mat, world, kind)
        if side > 0:
            for sg in (1, -1):
                poly([(a[0] + sg * nx, ya, a[1] + sg * nz), (b[0] + sg * nx, yb, b[1] + sg * nz), (b[0] + sg * nx, yb - side, b[1] + sg * nz),
                      (a[0] + sg * nx, ya - side, a[1] + sg * nz)], "stone_dark", world)


def beam(a, b, width, height, mat, world="both"):
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    v = np.array([b[0] - a[0], b[2] - a[2]])
    v = v / max(np.linalg.norm(v), 1e-9)
    nx, nz = -v[1] * width / 2, v[0] * width / 2
    t = [(a[0] + nx, a[1], a[2] + nz), (b[0] + nx, b[1], b[2] + nz), (b[0] - nx, b[1], b[2] - nz), (a[0] - nx, a[1], a[2] - nz)]
    btm = [(x, y - height, z) for x, y, z in t]
    poly(t, mat, world)
    for i in range(4):
        j = (i + 1) % 4
        poly([t[i], t[j], btm[j], btm[i]], mat, world)


def tube(a, b, r, n, mat, world="both"):
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    u = (b - a) / np.linalg.norm(b - a)
    p = np.cross(u, [0.0, 1.0, 0.0])
    p = p / max(np.linalg.norm(p), 1e-9)
    q = np.cross(u, p)
    ra = [a + r * (math.cos(2 * math.pi * k / n) * p + math.sin(2 * math.pi * k / n) * q) for k in range(n)]
    rb = [x + (b - a) for x in ra]
    for k in range(n):
        poly([ra[k], ra[(k + 1) % n], rb[(k + 1) % n], rb[k]], mat, world)
    poly(ra, mat, world)
    poly(rb, mat, world)


# ---- the terrain ------------------------------------------------------------------------------------------------------------

def build_terrain():
    def cells(x0, x1, z0, z1, step, skip):
        gx = np.arange(x0, x1 + 0.1, step)
        gz = np.arange(z0, z1 + 0.1, step)
        mx, mz = np.meshgrid(gx, gz)
        h = ground(mx, mz) if step < 5 else natural(mx, mz)
        for j in range(len(gz) - 1):
            for i in range(len(gx) - 1):
                cx, cz = gx[i] + step / 2, gz[j] + step / 2
                if skip(cx, cz):
                    continue
                ys = (h[j, i], h[j, i + 1], h[j + 1, i + 1], h[j + 1, i])
                if max(ys) < SEA - 0.3:
                    continue
                pts = [(gx[i], ys[0], gz[j]), (gx[i + 1], ys[1], gz[j]), (gx[i + 1], ys[2], gz[j + 1]), (gx[i], ys[3], gz[j + 1])]
                slope = (max(ys) - min(ys)) / step
                inside = inside_wall(cx, cz)
                if slope > 0.9:
                    mat = "stone_dark" if inside or math.hypot(cx, cz) < GORGE_R + 2 else "rock"
                elif min(ys) < SEA + 0.8:
                    mat = "sand"
                elif inside or math.hypot(cx, cz) < GORGE_R + 2:
                    mat = "paving"
                elif max(ys) > 46:
                    mat = "rock"
                else:
                    mat = "field" if fbm(cx / 30.0, cz / 30.0, 2, 21) > 0.12 else "grass"
                POLYS.append((np.array(pts), mat, "both", "terrain"))

    fine = (-140.0, 150.0, -130.0, 120.0)
    well = lambda x, z: math.hypot(x - WELL[0], z - WELL[1]) < WELL_R  # noqa: E731
    cells(*fine, 2.5, well)
    cells(-640.0, 660.0, -400.0, 600.0, 20.0, lambda x, z: fine[0] < x < fine[1] and fine[2] < z < fine[3])
    for x in np.arange(-900.0, 900.0, 30.0):                                                       # the sea, in tiles (drawn first)
        for z in np.arange(-900.0, 0.0, 30.0):
            if z > float(coast_z(x + 15.0)) + 4.0:
                continue
            poly([(x, SEA, z), (x + 30, SEA, z), (x + 30, SEA, z + 30), (x, SEA, z + 30)], "sea", "both", "sea")


# ---- the landmarks ----------------------------------------------------------------------------------------------------------

def build_rock():
    cx, cz = TOWER
    poly([(x, ROCK_Y + 0.02, z) for x, z in ngon(0, 0, ROCK_R - 0.3, 32)], "paving", "both", "overlay")
    for k in range(24):                                                                 # the balustrade of the plaza
        a0, a1 = 2 * math.pi * k / 24, 2 * math.pi * (k + 1) / 24
        if abs(math.cos(a0)) > 0.97:
            continue                                                                    # the two bridges
        p0 = (ROCK_R * math.cos(a0), ROCK_R * math.sin(a0))
        p1 = (ROCK_R * math.cos(a1), ROCK_R * math.sin(a1))
        world = "A" if (k % 5 == 2) else "both"
        beam((p0[0], ROCK_Y + 1.0, p0[1]), (p1[0], ROCK_Y + 1.0, p1[1]), 0.4, 1.0, "stone", world)
    # the church: a nave with a copper dome, the bell tower on its north side
    prism(ngon(cx, cz - 2.0, 6.5, 8, math.pi / 8), ROCK_Y, ROCK_Y + 11.0, "stone", "both", True, "stone_dark")
    dome(cx, cz - 2.0, 5.6, ROCK_Y + 11.0, "copper", "A")
    dome(cx, cz - 2.0, 5.6, ROCK_Y + 11.0, "copper", "B", bands=2)
    for k in range(8):
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(cx + 6.8 * math.cos(a), cz - 2.0 + 6.8 * math.sin(a), 1.0, 1.3, a), ROCK_Y, ROCK_Y + 9.0, "stone_dark")
    tx, tz = cx, cz + 5.5
    prism(ngon(tx, tz, 3.4, 8, math.pi / 8), ROCK_Y, ROCK_Y + 26.0, "stone")                    # the shaft (both)
    prism(ngon(tx, tz, 3.4, 8, math.pi / 8), ROCK_Y + 26.0, ROCK_Y + 40.0, "stone", "A")        # A: on up to the belfry
    for k in range(8):
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(tx + 3.0 * math.cos(a), tz + 3.0 * math.sin(a), 0.8, 0.8, a), ROCK_Y + 40.0, ROCK_Y + 46.0, "stone", "A")
    prism(ngon(tx, tz, 3.8, 8, math.pi / 8), ROCK_Y + 46.0, ROCK_Y + 47.0, "stone", "A")
    prism(ngon(tx, tz, 1.8, 10), ROCK_Y + 41.5, ROCK_Y + 44.5, "gold", "A")                    # the great bell
    cone(tx, tz, 3.8, ROCK_Y + 47.0, ROCK_Y + 58.0, "copper", "A", 8)                           # the spire
    for k in range(9):                                                                          # the ring of the lights
        a = 2 * math.pi * k / 9
        prism(ngon(tx + 4.0 * math.cos(a), tz + 4.0 * math.sin(a), 0.45, 6), ROCK_Y + 25.2, ROCK_Y + 26.4, "memory", "both")
    for k in range(8):                                                                          # B: broken teeth
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(tx + 2.9 * math.cos(a), tz + 2.9 * math.sin(a), 1.5, 0.8, a + math.pi / 2), ROCK_Y + 26.0,
              ROCK_Y + 27.0 + (k * 7 % 5), "stone", "B")
    # B: the top fell north-east into the gorge and lies on its floor in three pieces
    a = math.radians(55.0)
    for k, (r0, r1, rad) in enumerate(((16.0, 21.0, 3.2), (18.0, 22.0, 3.4))):
        p0 = (r0 * math.cos(a - 0.35 * k), GORGE_Y + rad * 0.85, r0 * math.sin(a - 0.35 * k))
        p1 = (r1 * math.cos(a + 0.35 - 0.35 * k), GORGE_Y + rad * 0.85, r1 * math.sin(a + 0.35 - 0.35 * k))
        tube(p0, p1, rad, 8, "stone", "B")
    prism(ngon(19.0 * math.cos(a - 0.9), 19.0 * math.sin(a - 0.9), 1.8, 10), GORGE_Y, GORGE_Y + 2.6, "gold", "B")  # the bell, cracked
    # the moat of A (the open part of the cistern), the dry floor of B
    ring_o = ngon(0, 0, GORGE_R - 0.2, 40)
    ring_i = ngon(0, 0, ROCK_R + 0.9, 40)
    for i in range(40):
        j = (i + 1) % 40
        poly([(ring_o[i][0], MOAT_Y, ring_o[i][1]), (ring_o[j][0], MOAT_Y, ring_o[j][1]), (ring_i[j][0], MOAT_Y, ring_i[j][1]),
              (ring_i[i][0], MOAT_Y, ring_i[i][1])], "water", "A", "water")
    for k in range(14):                                                                         # B: rubble on the floor
        ang = 2 * math.pi * hash01(k, 3)
        r = ROCK_R + 2.0 + 6.0 * hash01(k, 5)
        x, z = r * math.cos(ang), r * math.sin(ang)
        prism(rect(x, z, 1.2 + hash01(k, 7) * 2.0, 1.0 + hash01(k, 9) * 1.6, ang), GORGE_Y, GORGE_Y + 0.4 + hash01(k, 11) * 1.4, "debris", "B")
        if k % 3 == 0:
            cone(x + 1.5, z, 0.8, GORGE_Y, GORGE_Y + 1.2, "moss", "B", 5)
    # the bridges: west and east, arched; in B the west one has fallen in the middle
    for name, sgn in (("west", -1.0), ("east", 1.0)):
        x0, x1 = sgn * (ROCK_R - 0.2), BRIDGEHEADS[name][0] - sgn * 3.0    # from the rock to the bastion
        n = 8
        for k in range(n):
            xa = x0 + (x1 - x0) * k / n
            xb = x0 + (x1 - x0) * (k + 1) / n
            world = "A" if (name == "west" and 3 <= k <= 5) else "both"
            beam((xa, ROCK_Y + 0.05, 0.0), (xb, ROCK_Y + 0.05, 0.0), 3.2, 1.0, "stone", world)
            for sz in (-1.5, 1.5):
                beam((xa, ROCK_Y + 1.0, sz), (xb, ROCK_Y + 1.0, sz), 0.3, 0.95, "stone", world)
        mid = (x0 + x1) / 2
        prism(rect(mid, 0.0, 1.6, 3.2, 0.0), GORGE_Y, ROCK_Y - 2.5, "stone_dark", "A" if name == "west" else "both")
        if name == "west":
            prism(rect(mid + 0.8, 0.6, 4.0, 3.4, 0.3), GORGE_Y, GORGE_Y + 1.6, "debris", "B")
    # the lift down the south face of the rock to the floor of the gorge (a shortcut, opened from above)
    prism(rect(0.0, -ROCK_R - 1.2, 2.6, 2.6, 0.0), GORGE_Y, ROCK_Y + 3.0, "stone_dark")
    gable(0.0, -ROCK_R - 1.2, 3.0, 3.0, 0.0, ROCK_Y + 3.0, 1.2, "roof")
    prism(rect(0.0, -ROCK_R - 2.6, 1.6, 0.3, 0.0), GORGE_Y, GORGE_Y + 2.4, "iron", "A")
    for x, z in ((-8.0, -9.5), (8.0, -9.5), (-9.0, 8.0), (9.0, 8.0)):                            # plane trees on the plaza
        prism(ngon(x, z, 0.3, 5), ROCK_Y, ROCK_Y + 2.5, "wood")
        cone(x, z, 2.6, ROCK_Y + 2.0, ROCK_Y + 6.5, "tree", "A", 9)
        cone(x, z, 1.4, ROCK_Y + 2.0, ROCK_Y + 4.0, "tree", "B", 6)
    for x, z in ((-5.0, -11.5), (5.0, -11.5)):                                                  # lamp posts: lit in both
        prism(ngon(x, z, 0.12, 4), ROCK_Y, ROCK_Y + 3.4, "iron")
        prism(ngon(x, z, 0.3, 6), ROCK_Y + 3.4, ROCK_Y + 4.0, "lantern", "A" if x < 0 else "both")


def build_wall():
    h = ground(WALL[:, 0], WALL[:, 1])
    d = lengths(WALL)
    for i in range(len(WALL) - 1):
        t = d[i] / d[-1]
        if t < 0.012 or t > 0.988:
            continue                                                                    # the sea gate
        a, b = WALL[i], WALL[i + 1]
        base = min(h[i], h[i + 1]) - 3.0
        top = max(h[i], h[i + 1]) + 4.0
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 0.9, v[0] * 0.9
        prism([(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)], base, top, "stone_dark", "both", True, "stone")
        if i % 2 == 0:
            dmg = damage(a[0], a[1])
            world = "A" if hash01(i, 77) < dmg * 0.6 else "both"
            prism([(a[0] + nx * 1.3, a[1] + nz * 1.3), (b[0] + nx * 1.3, b[1] + nz * 1.3), (b[0] + nx, b[1] + nz), (a[0] + nx, a[1] + nz)], top, top + 1.0, "stone", world)
    for k, (x, z) in enumerate(WALL_POINTS[1:-1]):
        y = float(ground(x, z))
        fallen = damage(x, z) > 0.5 and k % 2 == 0
        prism(ngon(x, z, 3.0, 10), y - 3.0, y + 13.0, "stone", "A" if fallen else "both")
        cone(x, z, 3.4, y + 13.0, y + 17.0, "roof", "A", 10)
        if fallen:
            prism(ngon(x, z, 3.0, 10), y - 3.0, y + 6.0 + k % 3, "stone", "B")
    gx, gz = WALL_POINTS[0]                                                             # the sea gate
    yg = float(ground(gx, gz))
    for dx in (-4.5, 4.5):
        prism(rect(gx + dx, gz, 4.0, 4.4, 0.0), yg - 1.0, yg + 11.0, "stone")
        cone(gx + dx, gz, 3.0, yg + 11.0, yg + 15.0, "roof", "A", 4)
    prism(rect(gx, gz, 5.0, 4.4, 0.0), yg + 5.5, yg + 11.0, "stone", "A")
    prism(rect(gx - 1.0, gz, 3.0, 2.4, 0.3), yg, yg + 1.6, "debris", "B")


def ship(x, z, ang, length, world, sails=True):
    c, s_ = math.cos(ang), math.sin(ang)
    prism(rect(x, z, length, length * 0.28, ang), SEA - 0.6, SEA + 1.4, "wood", world)
    if sails:
        for u in (-0.2, 0.2):
            mx, mz = x + c * u * length, z + s_ * u * length
            prism(ngon(mx, mz, 0.15, 4), SEA + 1.4, SEA + 1.4 + 0.75 * length, "wood", world)
            poly([(mx - s_ * 0.1, SEA + 2.4, mz + c * 0.1), (mx - s_ * 0.1, SEA + 1.2 + 0.7 * length, mz + c * 0.1),
                  (mx + c * 0.28 * length, SEA + 2.4, mz + s_ * 0.28 * length)], "cloth2", world)


def build_harbour():
    x0, z0, x1, z1 = HARBOUR
    prism([(x0 - 4, z1 - 3), (x1 + 4, z1 - 3), (x1 + 4, z1 + 4), (x0 - 4, z1 + 4)], SEA - 4.0, QUAY_Y, "stone_dark", "both", True, "paving")
    mole = spline([(x1 + 4, z1 - 2), (x1 + 10, z1 - 12), (x1 + 4, z0 + 4), (x0 + 16, z0)], 1.0)
    d = lengths(mole)
    for i in range(len(mole) - 1):
        t = d[i] / d[-1]
        a, b = mole[i], mole[i + 1]
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 1.8, v[0] * 1.8
        prism([(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)], SEA - 4.0,
              QUAY_Y + 0.3, "stone_dark", "A" if 0.5 < t < 0.64 else "both", True, "paving")
    lx, lz = mole[-1]                                                                   # the harbour light
    prism(ngon(lx, lz, 1.4, 8), QUAY_Y, QUAY_Y + 7.0, "stone", "A")
    prism(ngon(lx, lz, 1.6, 8), QUAY_Y + 7.0, QUAY_Y + 8.2, "lantern", "A")
    cone(lx, lz, 1.8, QUAY_Y + 8.2, QUAY_Y + 10.0, "copper", "A", 8)
    prism(ngon(lx, lz, 1.4, 8), QUAY_Y, QUAY_Y + 3.0, "stone", "B")
    cx, cz = 22.0, z1 + 9.0                                                             # the customs house, its copper dome
    y = float(ground(cx, cz))
    prism(rect(cx, cz, 10.0, 7.0, 0.0), y - 1.0, y + 7.0, "plaster3", "A")
    dome(cx, cz, 3.2, y + 7.0, "copper", "A")
    prism(rect(cx, cz, 10.0, 7.0, 0.0), y - 1.0, y + 3.5, "plaster3", "B")
    for k, (bx, bz) in enumerate(((-12.0, z1 - 6.0), (0.0, z1 - 7.0), (12.0, z1 - 6.0))):
        ship(bx, bz, 0.1 * k, 6.0, "A", sails=k == 1)
    ship(70.0, -170.0, -0.3, 18.0, "A")
    ship(-40.0, -230.0, 0.2, 20.0, "A")
    ship(-20.0, z1 - 10.0, 0.6, 12.0, "B", sails=False)                                 # B: a wreck in the basin
    for k in range(5):
        sx_, sz_ = -14.0 + 7.0 * k, z1 - 14.0 - 2.0 * (k % 2)
        prism(ngon(sx_, sz_, 3.0, 9), SEA - 1.0, SEA + 0.35, "sand", "B")
        cone(sx_ + 1.0, sz_, 0.6, SEA + 0.3, SEA + 1.8, "moss", "B", 4)
    hx, hz = -86.0, -88.0                                                               # the lighthouse on the headland
    y = float(ground(hx, hz))
    prism(ngon(hx, hz, 2.2, 10), y, y + 18.0, "plaster3")
    prism(ngon(hx, hz, 2.6, 10), y + 18.0, y + 20.0, "lantern", "A")
    prism(ngon(hx, hz, 2.6, 10), y + 18.0, y + 20.0, "iron", "B")
    cone(hx, hz, 2.8, y + 20.0, y + 23.0, "roof", "both", 10)


def build_west():
    cx, cz = WELL
    for k in range(24):                                                                 # the parapet of the well
        a0, a1 = 2 * math.pi * k / 24, 2 * math.pi * (k + 1) / 24
        beam((cx + (WELL_R + 0.5) * math.cos(a0), 5.0, cz + (WELL_R + 0.5) * math.sin(a0)),
             (cx + (WELL_R + 0.5) * math.cos(a1), 5.0, cz + (WELL_R + 0.5) * math.sin(a1)), 1.0, 1.0, "stone")
    for i, (x, z) in enumerate(ngon(cx, cz, WELL_R, 24)):
        x2, z2 = ngon(cx, cz, WELL_R, 24)[(i + 1) % 24]
        poly([(x, -10.0, z), (x2, -10.0, z2), (x2, 4.0, z2), (x, 4.0, z)], "stone_dark")
    poly([(x, -10.0, z) for x, z in ngon(cx, cz, WELL_R, 24)], "paving")
    poly([(x, -6.0, z) for x, z in ngon(cx, cz, WELL_R - 0.05, 24)], "water", "A", "water")
    prism(ngon(cx + 1.0, cz - 1.0, 1.6, 10), -10.0, -8.0, "gold", "B")                   # the day bell, fallen
    hx, hz = -62.0, 18.0                                                                # the market hall and its copper dome
    y = float(ground(hx, hz))
    prism(rect(hx, hz, 12.0, 9.0, 0.2), y, y + 8.0, "plaster1", "both", True, "stone_dark")
    dome(hx, hz, 4.0, y + 8.0, "copper", "A")
    tx, tz = -38.0, 14.0                                                                # the clock tower and the day bell (A)
    y = float(ground(tx, tz))
    prism(rect(tx, tz, 4.5, 4.5, 0.2), y, y + 20.0, "stone")
    prism(ngon(tx, tz, 1.4, 10), y + 16.5, y + 18.5, "gold", "A")
    gable(tx, tz, 4.5, 4.5, 0.2, y + 20.0, 3.0, "roof", "A")
    for k in range(4):
        poly([(tx - 1.0 + 0.0, y + 13.0, tz - 2.27), (tx + 1.0, y + 13.0, tz - 2.27), (tx + 1.0, y + 15.0, tz - 2.27),
              (tx - 1.0, y + 15.0, tz - 2.27)], "window", "A") if k == 0 else None
    curve, width, _ = STREET["market street"]                                           # awnings (A), bare frames (B)
    for k, t in enumerate(np.linspace(0.1, 0.8, 6)):
        p, v = at(curve, t)
        side = 1 if k % 2 else -1
        nx, nz = -v[1] * side * (width / 2 + 1.0), v[0] * side * (width / 2 + 1.0)
        y = float(ground(p[0] + nx, p[1] + nz))
        ang = math.atan2(v[1], v[0])
        prism(rect(p[0] + nx, p[1] + nz, 2.6, 1.4, ang), y, y + 1.0, "wood")
        gable(p[0] + nx, p[1] + nz, 2.6, 2.2, ang + math.pi / 2, y + 2.6, 0.5, "cloth" if k % 2 else "cloth2", "A")


def build_east():
    d = lengths(LOCK_LINE)
    for i, lv in enumerate(LOCK_LEVELS):
        s0, s1 = d[-1] * i / 5, d[-1] * (i + 1) / 5
        seg = LOCK_LINE[(d >= s0) & (d <= s1)]
        if len(seg) < 2:
            continue
        bed = lv - 4.0
        ribbon(seg, np.full(len(seg), bed), 8.0, "paving", "both", 0.0, "overlay")
        ribbon(seg, np.full(len(seg), lv - 0.6), 7.6, "water", "A", 0.0, "water")
        a = seg[-1]
        v = (seg[-1] - seg[-2]) / max(np.linalg.norm(seg[-1] - seg[-2]), 1e-9)
        ang = math.atan2(v[1], v[0])
        prism(rect(a[0], a[1], 0.6, 5.4, ang), bed, lv + 3.2 if i < 4 else lv + 1.0, "wood", "A")     # the gates (A)
        prism(rect(a[0], a[1], 0.8, 5.4, ang), bed, bed + 1.2, "debris", "B")
        if i < 4:                                                                       # the fall into the next (A)
            pass
    tx, tz = 84.0, 26.0                                                                 # the water tower and the dawn bell
    y = float(ground(tx, tz))
    prism(ngon(tx, tz, 3.0, 10), y, y + 18.0, "stone")
    prism(ngon(tx, tz, 4.0, 10), y + 18.0, y + 22.0, "plaster2", "A")
    cone(tx, tz, 4.4, y + 22.0, y + 25.0, "roof", "A", 10)
    prism(ngon(tx, tz, 1.4, 10), y + 15.0, y + 17.4, "gold", "A")
    aq = spline([(150, 34), (126, 32), (104, 30), (88, 30)], 2.0)                       # the aqueduct, from the hills
    hs = np.linspace(26.0, y + 18.0, len(aq))
    dd = lengths(aq)
    for i in range(len(aq) - 1):
        t = dd[i] / dd[-1]
        world = "A" if 0.45 < t < 0.58 else "both"
        a, b = aq[i], aq[i + 1]
        beam((a[0], hs[i], a[1]), (b[0], hs[i + 1], b[1]), 2.0, 1.4, "stone", world)
        if i % 3 == 0 and world == "both":
            g = float(natural(a[0], a[1]))
            if g < hs[i] - 3.0:
                prism(rect(a[0], a[1], 1.2, 1.6, 0.0), g - 1.0, hs[i] - 1.4, "stone")


def build_north():
    cx, cz = THEATRE
    for k in range(9):                                                                  # the seats
        r0, r1 = 6.0 + 1.3 * k, 7.3 + 1.3 * k
        y = THEATRE_Y + 0.9 * (k + 1)
        n = 16
        for j in range(n):
            a0, a1 = math.pi * j / n, math.pi * (j + 1) / n
            world = "A" if (j < 5 and k > 4) else "both"
            pts = [(cx + r0 * math.cos(a0), cz + r0 * math.sin(a0)), (cx + r1 * math.cos(a0), cz + r1 * math.sin(a0)),
                   (cx + r1 * math.cos(a1), cz + r1 * math.sin(a1)), (cx + r0 * math.cos(a1), cz + r0 * math.sin(a1))]
            prism(pts, y - 3.0, y, "stone", world)
    prism(rect(cx, cz - 6.0, 18.0, 5.0, 0.0), THEATRE_Y - 1.0, THEATRE_Y + 1.0, "wood")      # the stage
    for dx in (-8.0, -4.0, 0.0, 4.0, 8.0):                                              # the stage front: columns (A), stumps (B)
        prism(rect(cx + dx, cz - 9.0, 1.2, 1.2, 0.0), THEATRE_Y + 1.0, THEATRE_Y + 8.0, "stone", "A")
        prism(rect(cx + dx, cz - 9.0, 1.2, 1.2, 0.0), THEATRE_Y + 1.0, THEATRE_Y + 2.0 + abs(dx) / 3, "stone", "B")
    beam((cx - 9.0, THEATRE_Y + 8.0, cz - 9.0), (cx + 9.0, THEATRE_Y + 8.0, cz - 9.0), 1.6, 1.0, "stone", "A")
    gable(cx, cz - 6.0, 18.0, 5.0, 0.0, THEATRE_Y + 8.0, 1.4, "cloth", "A")                       # an awning over the stage (A)
    for k in range(30):                                                                 # the graveyard
        x = -18.0 + (k * 37 % 36)
        z = 74.0 + (k * 13 % 14)
        prism(rect(x, z, 0.8, 0.35, (k % 5) * 0.1), GRAVE_Y, GRAVE_Y + 1.0 + (k % 3) * 0.2, "stone", "A" if k % 7 == 0 else "both")
    for k in range(10):
        x, z = -22.0 + k * 5.0, 91.0
        cone(x, z, 0.9, GRAVE_Y, GRAVE_Y + 7.0, "cypress", "both", 6)
    nx_, nz_ = 16.0, 84.0                                                               # the night bell tower
    prism(rect(nx_, nz_, 4.0, 4.0, 0.1), GRAVE_Y, GRAVE_Y + 14.0, "stone")
    cone(nx_, nz_, 3.2, GRAVE_Y + 14.0, GRAVE_Y + 19.0, "roof", "both", 4)
    prism(ngon(nx_, nz_, 1.2, 10), GRAVE_Y + 10.5, GRAVE_Y + 12.6, "gold")
    prism(ngon(-12.0, 78.0, 0.3, 6), GRAVE_Y + 0.4, GRAVE_Y + 1.2, "lantern", "B")       # a lantern on a new grave (B)


def build_arches():
    """Arches over the streets: a house front that bridges the street, every 25 m or so on the lower and market streets and
    the market stairs; in B a third of them have fallen (a heap under the gap)."""
    k = 0
    for name in ("lower street", "market stairs", "market street", "upper ring"):
        c, w, kind = STREET[name]
        hs = _HEIGHTS[name]
        d = lengths(c)
        s = 12.0
        while s < d[-1] - 6.0:
            k += 1
            i = int(np.clip(np.searchsorted(d, s), 1, len(c) - 2))
            p, v = c[i], (c[i + 1] - c[i]) / max(np.linalg.norm(c[i + 1] - c[i]), 1e-9)
            nx, nz = -v[1], v[0]
            y = float(hs[i])
            fallen = hash01(k, 51) < 0.33 and damage(p[0], p[1]) > 0.3
            for side in (-1, 1):
                px_, pz_ = p[0] + nx * side * (w / 2 + 0.4), p[1] + nz * side * (w / 2 + 0.4)
                prism(rect(px_, pz_, 1.2, 0.8, math.atan2(v[1], v[0])), y - 0.5, y + 3.4, "stone_dark")
            span = [(p[0] + nx * (w / 2 + 0.8), p[1] + nz * (w / 2 + 0.8)), (p[0] - nx * (w / 2 + 0.8), p[1] - nz * (w / 2 + 0.8))]
            a, b = span
            body = [(a[0] - v[0] * 1.6, a[1] - v[1] * 1.6), (a[0] + v[0] * 1.6, a[1] + v[1] * 1.6), (b[0] + v[0] * 1.6, b[1] + v[1] * 1.6),
                    (b[0] - v[0] * 1.6, b[1] - v[1] * 1.6)]
            mat = "plaster%d" % (k % 4)
            prism(body, y + 3.4, y + 6.2, mat, "A" if fallen else "both")
            gable(p[0], p[1], 3.2, w + 1.6, math.atan2(v[1], v[0]), y + 6.2, 1.2, "roof", "A" if fallen else "both")
            if fallen:
                prism(rect(p[0], p[1], w * 0.7, 2.2, 0.3), y, y + 1.0, "debris", "B")
            s += 22.0 + 8.0 * hash01(k, 53)


def build_streets():
    for name, (c, w, k) in STREET.items():
        ribbon(c, _HEIGHTS[name] + 0.06, w, "paving", "both", 0.0, "overlay")
        if k == "stair":
            for i in range(0, len(c) - 1, 2):                                          # treads
                a, b = c[i], c[min(i + 1, len(c) - 1)]
                v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
                nx, nz = -v[1] * w / 2, v[0] * w / 2
                y = _HEIGHTS[name][i] + 0.07
                poly([(a[0] + nx, y, a[1] + nz), (a[0] - nx, y, a[1] - nz), (a[0] - nx, y - 0.25, a[1] - nz), (a[0] + nx, y - 0.25, a[1] + nz)],
                     "stone_dark", "both", "overlay")
        d = lengths(c)                                                                  # lamp posts
        s = 6.0
        k2 = 0
        while s < d[-1]:
            p, v = at(c, s / d[-1])
            side = 1 if k2 % 2 else -1
            x, z = p[0] - v[1] * side * (w / 2 - 0.3), p[1] + v[0] * side * (w / 2 - 0.3)
            y = float(ground(x, z))
            prism(ngon(x, z, 0.1, 4), y, y + 3.2, "iron")
            if min(math.hypot(x - bx, z - bz) for bx, bz in BRIDGEHEADS.values()) < 9.0:
                s += 14.0
                continue
            on_path = name in ("sea stair", "lower street", "market stairs", "market street", "east approach", "upper ring", "cliff stair")
            lit_b = (on_path and k2 % 2 == 0) or hash01(int(x * 10), int(z * 10)) < 0.08
            prism(ngon(x, z, 0.5 if lit_b else 0.25, 6), y + 3.2, y + 4.0, "lantern", "both" if lit_b else "A")
            if lit_b:                                                                  # B: a pool of warm light under it
                poly([(px_, y + 0.08, pz_) for px_, pz_ in ngon(x, z, 3.0, 12)], "glow", "B", "overlay")
            if not lit_b:
                prism(ngon(x, z, 0.25, 6), y + 3.2, y + 3.4, "iron", "B")
            s += 14.0
            k2 += 1


# ---- houses -----------------------------------------------------------------------------------------------------------------
HOUSES = []


def overlaps(p, q):
    for poly_ in (p, q):
        n = len(poly_)
        for i in range(n):
            ex, ez = poly_[(i + 1) % n][0] - poly_[i][0], poly_[(i + 1) % n][1] - poly_[i][1]
            a = [x * -ez + z * ex for x, z in p]
            b = [x * -ez + z * ex for x, z in q]
            if max(a) <= min(b) + 1e-3 or max(b) <= min(a) + 1e-3:
                return False
    return True


def site_free(out):
    xs = np.array([p[0] for p in out] + [sum(p[0] for p in out) / 4])
    zs = np.array([p[1] for p in out] + [sum(p[1] for p in out) / 4])
    for x, z in zip(xs, zs):
        if not inside_wall(x, z) or math.hypot(x, z) < GORGE_R + 2.5:
            return False
        if math.hypot(x - WELL[0], z - WELL[1]) < WELL_R + 6.0 or math.hypot(x - THEATRE[0], z - THEATRE[1]) < 18.5:
            return False
        if math.hypot(x + 62.0, z - 18.0) < 8.5 or math.hypot(x + 38.0, z - 14.0) < 4.0 or math.hypot(x - 84.0, z - 26.0) < 5.5:
            return False
        if any(math.hypot(x - bx, z - bz) < 6.0 for bx, bz in BRIDGEHEADS.values()):
            return False
    if distance_to(LOCK_LINE, xs, zs)[0].min() < 6.5 or distance_to(WALL, xs, zs)[0].min() < 2.4:
        return False
    for name, (c, w, k) in STREET.items():
        if distance_to(c, xs, zs)[0].min() < w / 2 + 0.25:
            return False
    g = ground(xs, zs)
    if g.max() - g.min() > 3.0:
        return False
    return not any(overlaps(out, h) for h in HOUSES)


def house(cx, cz, w, dd, ang, k, height, front=None):
    out = rect(cx, cz, w, dd, ang)
    if not site_free(out):
        return False
    HOUSES.append(out)
    base = float(np.min(ground(np.array([p[0] for p in out]), np.array([p[1] for p in out]))))
    top = base + height
    mat = "plaster%d" % (k % 4)
    dmg = damage(cx, cz)
    roll = hash01(k, 7)
    ruin = roll < 0.05 + 0.7 * dmg
    unroofed = not ruin and roll < 0.2 + 0.55 * dmg
    roofmat = "roof" if k % 3 else "roof2"
    if ruin:
        prism(out, base - 1.0, top, mat, "A")
        prism(out, base - 1.0, base + height * (0.35 + 0.3 * hash01(k, 9)), mat, "B", True, "debris")
        cone(cx, cz, 1.2, base + 1.0, base + 4.0, "moss", "B", 5)
    else:
        prism(out, base - 1.0, top, mat)
    kind = hash01(k, 31)
    if kind < 0.07 and not ruin and w < 4.8:                                          # a tall narrow tower house
        tw = rect(cx, cz, min(w, dd) * 0.7, min(w, dd) * 0.7, ang)
        prism(tw, top, top + 5.0, mat, "A" if unroofed else "both")
        cone(cx, cz, min(w, dd) * 0.55, top + 5.0, top + 7.5, roofmat, "A" if unroofed else "both", 4)
        return True
    if 0.07 <= kind < 0.20 and not ruin:                                              # a loggia on the top floor
        c, s = math.cos(ang), math.sin(ang)
        for ux, uz in ((-w / 2 + 0.3, -dd / 2 + 0.3), (w / 2 - 0.3, -dd / 2 + 0.3), (w / 2 - 0.3, dd / 2 - 0.3), (-w / 2 + 0.3, dd / 2 - 0.3)):
            px_, pz_ = cx + ux * c - uz * s, cz + ux * s + uz * c
            prism(ngon(px_, pz_, 0.2, 4), top, top + 2.2, "stone", "A" if unroofed else "both")
        gable(cx, cz, w, dd, ang, top + 2.2, 0.35 * min(w, dd) + 0.4, roofmat, "A" if unroofed else "both")
        if unroofed:
            prism(out, top, top + 0.6, mat, "B")
        return True
    c, s = math.cos(ang), math.sin(ang)
    if not ruin and height > 7.5 and 0.20 <= kind < 0.45:                              # the top storey set back: a stepped mass
        upper = rect(cx, cz, w * 0.72, dd * 0.72, ang)
        prism(upper, top, top + 2.6, mat, "A" if unroofed else "both")
        poly([(x, top + 0.02, z) for x, z in out], "paving", "A" if unroofed else "both")                     # its terrace
        gable(cx, cz, w * 0.72, dd * 0.72, ang, top + 2.6, 0.30 * min(w, dd) + 0.3, roofmat, "A" if unroofed else "both")
        top = top + 2.6
    flat = k % 5 == 0
    pitch = 0.22 + 0.33 * hash01(k, 41)
    if flat:                                                                           # a roof terrace with a parapet
        poly([(x, top + 0.02, z) for x, z in out], "paving", "A" if (ruin or unroofed) else "both")
        for i in range(4):
            a, b = out[i], out[(i + 1) % 4]
            beam((a[0], top + 0.8, a[1]), (b[0], top + 0.8, b[1]), 0.25, 0.8, mat, "A" if ruin else "both")
    elif not (height > 7.5 and 0.20 <= kind < 0.45 and not ruin):
        gable(cx, cz, w, dd, ang, top, pitch * min(w, dd) + 0.4, roofmat, "A" if (ruin or unroofed) else "both")
        if hash01(k, 43) < 0.5:                                                         # a chimney
            ux, uz = w * 0.28, dd * 0.15
            px_, pz_ = cx + ux * c - uz * s, cz + ux * s + uz * c
            prism(rect(px_, pz_, 0.5, 0.5, ang), top, top + pitch * min(w, dd) + 1.4, "chimney", "A" if (ruin or unroofed) else "both")
    if front is not None:                                                              # the street side: door, shutters, balcony
        fx, fz = front
        out_n = dd / 2 + 0.04
        dx_, dz_ = cx + fx * out_n, cz + fz * out_n
        poly([(dx_ - c * 0.55, base, dz_ - s * 0.55), (dx_ + c * 0.55, base, dz_ + s * 0.55), (dx_ + c * 0.55, base + 2.2, dz_ + s * 0.55),
              (dx_ - c * 0.55, base + 2.2, dz_ - s * 0.55)], "door", "A" if ruin else "both")
        if height > 6.0 and hash01(k, 45) < 0.35 and not ruin:                         # a balcony on the first floor
            by = base + 3.1
            bx0, bz0 = cx + fx * (dd / 2 + 0.7), cz + fz * (dd / 2 + 0.7)
            prism(rect(bx0, bz0, w * 0.55, 1.4, ang), by - 0.15, by, "wood", "A" if unroofed else "both")
            beam((bx0 - c * w * 0.27 + fx * 0.65, by + 0.9, bz0 - s * w * 0.27 + fz * 0.65),
                 (bx0 + c * w * 0.27 + fx * 0.65, by + 0.9, bz0 + s * w * 0.27 + fz * 0.65), 0.08, 0.9, "iron", "A" if unroofed else "both")
            if not unroofed and hash01(k, 47) < 0.6:
                cone(bx0 + c * w * 0.18, bz0 + s * w * 0.18, 0.4, by, by + 0.7, "tree", "A", 5)        # flowers in pots (A)
    if front is not None:                                                              # lit windows on the street side (A)
        fx, fz = front
        c, s = math.cos(ang), math.sin(ang)
        for j in range(2):
            if hash01(k, 20 + j) < 0.45:
                u = (j - 0.5) * w * 0.45
                px, pz = cx + u * c + fx * (dd / 2 + 0.03), cz + u * s + fz * (dd / 2 + 0.03)
                y = base + 3.0 if height > 6 else base + 1.6
                poly([(px - c * 0.4, y, pz - s * 0.4), (px + c * 0.4, y, pz + s * 0.4), (px + c * 0.4, y + 1.1, pz + s * 0.4),
                      (px - c * 0.4, y + 1.1, pz - s * 0.4)], "window", "A")
                for sg in (-1, 1):                                                     # shutters
                    q0 = (px + sg * c * 0.45, pz + sg * s * 0.45)
                    q1 = (px + sg * c * 0.85, pz + sg * s * 0.85)
                    poly([(q0[0], y, q0[1]), (q1[0], y, q1[1]), (q1[0], y + 1.1, q1[1]), (q0[0], y + 1.1, q0[1])], "shutter",
                         "A" if ruin else "both")
    return True


def build_houses():
    k = 0
    for name, (c, w, kind) in STREET.items():
        if kind == "stair" or name in ("headland path", "harbour road"):
            continue
        d = lengths(c)
        for side in (-1, 1):
            for row, offset in enumerate((0.0, 7.0)):
                s = 2.0
                while s < d[-1] - 2.0:
                    k += 1
                    ww = 3.6 + (k * 7 % 5) * 0.5
                    dd = 5.0 + (k * 11 % 3) * 0.6
                    p, v = at(c, min(1.0, (s + ww / 2) / d[-1]))
                    n = np.array([-v[1], v[0]]) * side
                    ang = math.atan2(v[1], v[0])
                    ctr = p + n * (w / 2 + 0.3 + offset + dd / 2)
                    by_gorge = math.hypot(ctr[0], ctr[1]) < 34                         # low by the gorge: the rock stands out
                    h = (4.0 if by_gorge else 5.0) + (k * 17 % 5) * 1.4 + (1.5 if row == 1 else 0.0)
                    placed = house(ctr[0], ctr[1], ww, dd, ang, k, h, front=(-n[0], -n[1]) if row == 0 else None)
                    s += ww if placed else 1.2
    for gx in np.arange(-80.0, 92.0, 6.2):                                              # the blocks between the streets
        for gz in np.arange(-60.0, 64.0, 6.2):
            k += 1
            x = gx + hash01(k, 1) * 1.5
            z = gz + hash01(k, 2) * 1.5
            best, ang = 1e9, 0.0
            for name, (c, w, kind) in STREET.items():
                dist, i = distance_to(c, x, z)
                if dist < best:
                    i = int(min(i, len(c) - 2))
                    v = c[i + 1] - c[i]
                    best, ang = dist, math.atan2(v[1], v[0])
            house(x, z, 4.0 + hash01(k, 3) * 1.6, 4.8 + hash01(k, 4) * 1.2, ang, k, 5.0 + hash01(k, 5) * 4.0)


def build_country():
    k = 0
    for gx in np.arange(-150.0, 160.0, 4.0):
        for gz in np.arange(-110.0, 140.0, 4.0):
            k += 1
            x = gx + hash01(k, 1) * 2.0
            z = gz + hash01(k, 2) * 2.0
            if inside_wall(x, z) or distance_to(WALL, x, z)[0] < 6.0:
                continue
            if any(distance_to(c, x, z)[0] < w / 2 + 2.0 for c, w, _ in STREET.values()):
                continue
            y = float(ground(x, z))
            if y < SEA + 1.5 or (abs(x) < 26 and 66 < z < 92):
                continue
            r = hash01(k, 3)
            wild = fbm(x / 25.0, z / 25.0, 2, 41)
            if x < -60 and z < -55:                                                     # the headland: pines
                if r < 0.35:
                    h = 6.0 + 2.0 * hash01(k, 4)
                    felled = hash01(k, 5) < 0.4
                    prism(ngon(x, z, 0.22, 5), y, y + h, "wood", "A" if felled else "both")
                    cone(x, z, 3.0, y + h - 0.6, y + h + 1.0, "cypress", "A" if felled else "both", 9)
                    if felled:
                        beam((x, y + 0.4, z), (x + 5.0, y + 0.3, z + 2.0), 0.4, 0.4, "wood", "B")
            elif wild > 0.15 and r < 0.4:                                               # olives in A, scrub in B
                prism(ngon(x, z, 0.16, 4), y, y + 1.2, "wood", "A")
                cone(x, z, 1.6, y + 0.9, y + 2.6, "tree", "A", 7)
                cone(x, z, 1.4, y - 0.1, y + 1.3, "moss", "B", 5)
            elif r < 0.06:
                cone(x, z, 0.8, y, y + 6.0, "cypress", "both", 6)


# ---- the main path and the shortcuts (for the plan) -------------------------------------------------------------------------
MAIN_PATH = [("start: the quay", 6, -72), ("sea gate", 0, -62), ("lower town", -28, -47), ("market stairs", -58, -24),
             ("the well", -50, 6), ("west bridge", -20, 0), ("Rock of the Bell", 0, -6), ("east bridge", 20, 0),
             ("the locks", 48, -18), ("lock stairs", 72, 0), ("water tower: dawn bell", 84, 26), ("upper ring", 40, 50),
             ("the theatre", 0, 38), ("cliff stair", 12, 62), ("graveyard: night bell", 0, 80), ("back to the rock", 0, 4)]
SHORTCUTS = [("lift: the plaza to the gorge (B) and the lower town", [(0, -12), (0, -20), (-4, -40)]),
             ("the gorge (B): under both bridges", [(-18, -8), (0, -19), (18, -8)]),
             ("the upper ring back down to the market", [(-40, 46), (-54, 14)])]


# ---- the renderer -----------------------------------------------------------------------------------------------------------

def colour(mat, world):
    a, b = MAT[mat]
    return np.array(a if world == "A" else b, float)


def render(world, path, eye, target, fov, size, title, overlay=None, ortho=None):
    to_light, light, amb, sky_top, horizon, fog = LIGHT[world]
    L = np.array(to_light, float)
    L /= np.linalg.norm(L)
    W, H = size
    eye = np.asarray(eye, float)
    f = np.asarray(target, float) - eye
    f /= np.linalg.norm(f)
    up0 = np.array([0.0, 1.0, 0.0]) if abs(f[1]) < 0.99 else np.array([0.0, 0.0, 1.0])
    right = np.cross(up0, f)
    right /= np.linalg.norm(right)
    up = np.cross(f, right)
    focal = (W / 2) / math.tan(math.radians(fov) / 2) if ortho is None else None
    polys, cols, depths, edges, widths = [], [], [], [], []
    for pts, mat, w, kind in POLYS:
        if w not in ("both", world):
            continue
        if mat in ("water",) and world == "B" and kind == "water":
            continue
        rel = pts - eye
        cam = np.stack([rel @ right, rel @ up, rel @ f], axis=1)
        if ortho is None:
            if (cam[:, 2] < 0.6).all():
                continue
            if (cam[:, 2] < 0.6).any():                                                 # clip against the near plane
                out = []
                n = len(cam)
                for i in range(n):
                    p0, p1 = cam[i], cam[(i + 1) % n]
                    in0, in1 = p0[2] >= 0.6, p1[2] >= 0.6
                    if in0:
                        out.append(p0)
                    if in0 != in1:
                        t = (0.6 - p0[2]) / (p1[2] - p0[2])
                        out.append(p0 + t * (p1 - p0))
                if len(out) < 3:
                    continue
                cam = np.array(out)
            sx = W / 2 + focal * cam[:, 0] / cam[:, 2]
            sy = H / 2 - focal * cam[:, 1] / cam[:, 2]
        else:
            sx = W / 2 + ortho * cam[:, 0]
            sy = H / 2 - ortho * cam[:, 1]
        if sx.max() < -50 or sx.min() > W + 50 or sy.max() < -50 or sy.min() > H + 50:
            continue
        nrm = np.cross(pts[1] - pts[0], pts[2] - pts[0])
        if np.linalg.norm(nrm) < 1e-9 and len(pts) > 3:
            nrm = np.cross(pts[2] - pts[0], pts[3] - pts[0])
        if np.linalg.norm(nrm) < 1e-9:
            continue
        nrm /= np.linalg.norm(nrm)
        centre = pts.mean(axis=0)
        view = (eye - centre) if ortho is None else -f
        if nrm @ view < 0:
            nrm = -nrm
        alb = colour(mat, world)
        emissive = mat in EMISSIVE and EMISSIVE[mat][0 if world == "A" else 1]
        if emissive:
            c = alb * 1.15
        else:
            ndl = max(0.0, float(nrm @ L))
            hemi = 0.65 + 0.35 * nrm[1]
            c = alb * (np.array(amb) * hemi * 1.15 + np.array(light) * ndl * 1.05)
            if kind in ("water", "sea"):                                               # the sky in the water
                c = 0.85 * alb * (0.7 + 0.3 * np.array(light)) + 0.15 * np.array(horizon)
        dist = np.linalg.norm(centre - eye)
        fo = 1.0 - math.exp(-dist * fog * (0.6 if kind == "sea" else 1.0))
        if emissive:
            fo *= 0.5
        c = c * (1 - fo) + np.array(horizon) * fo
        c = np.clip(c, 0, 1)
        depth = cam[:, 2].mean() if ortho is None else -float(rel.mean(axis=0) @ f)
        if kind == "sea":
            depth = 1e9                                                               # the lowest surface: under everything
        elif kind == "terrain":
            depth += 2.0                                                              # the ground under what stands on it
        elif kind == "overlay":
            depth += 0.8
        elif kind == "water":
            depth += 0.9
        polys.append(np.stack([sx, sy], axis=1))
        cols.append(c)
        depths.append(depth)
        edges.append(c * (0.55 if kind not in ("terrain", "water", "sea") else 1.0))
        widths.append(0.4 if kind not in ("terrain", "water", "sea") else 0.3)
    order = np.argsort(depths)[::-1]
    fig = plt.figure(figsize=(W / 100, H / 100), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1])
    grad = np.linspace(0, 1, 256)[:, None, None]
    sky = np.array(sky_top)[None, None, :] * (1 - grad) + np.array(horizon)[None, None, :] * grad
    ax.imshow(np.repeat(sky, 4, axis=1), extent=(0, W, H, 0), aspect="auto", zorder=0)
    pc = PolyCollection([polys[i] for i in order], facecolors=[cols[i] for i in order], edgecolors=[edges[i] for i in order],
                        linewidths=[widths[i] for i in order], zorder=1)
    ax.add_collection(pc)
    if overlay is not None:
        overlay(ax, eye, right, up, f, focal, W, H, ortho)
    ax.set_xlim(0, W)
    ax.set_ylim(H, 0)
    ax.set_axis_off()
    ax.text(14, 26, title, fontsize=12, color="white" if world == "B" else (0.25, 0.12, 0.10), weight="bold")
    fig.savefig(path, dpi=100)
    plt.close(fig)
    return len(polys)


def plan_overlay(ax, eye, right, up, f, focal, W, H, ortho):
    def project(x, y, z):
        rel = np.array([x, y, z]) - eye
        return W / 2 + ortho * (rel @ right), H / 2 - ortho * (rel @ up)

    pts = [project(x, float(ground(x, z)) + 1.0, z) for _, x, z in MAIN_PATH]
    xs, ys = zip(*pts)
    ax.plot(xs, ys, color=(0.75, 0.10, 0.15), lw=3.0, zorder=3)
    for (name, x, z), (px, py) in zip(MAIN_PATH, pts):
        ax.plot(px, py, "o", color=(0.75, 0.10, 0.15), ms=7, zorder=4)
        ax.text(px + 8, py - 6, name, fontsize=9, weight="bold", zorder=5,
                bbox=dict(boxstyle="round,pad=0.15", fc="white", alpha=0.8, ec="none"))
    for name, line in SHORTCUTS:
        q = [project(x, 0.0, z) for x, z in line]
        qx, qy = zip(*q)
        ax.plot(qx, qy, color=(0.12, 0.35, 0.80), lw=2.5, ls="--", zorder=3)
        ax.text(qx[-1] + 8, qy[-1] + 14, name, fontsize=8, color=(0.12, 0.35, 0.80), zorder=5,
                bbox=dict(boxstyle="round,pad=0.15", fc="white", alpha=0.7, ec="none"))


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    build_terrain()
    build_streets()
    build_rock()
    build_wall()
    build_harbour()
    build_west()
    build_east()
    build_north()
    build_arches()
    build_houses()
    build_country()
    print("%d polygons, %d houses" % (len(POLYS), len(HOUSES)))
    os.makedirs(OUT, exist_ok=True)
    views = {
        "aerial_a": ("A", "city2_aerial_a.png", (-70.0, 120.0, -235.0), (8.0, 4.0, -8.0), 44.0, (1600, 1000),
                     "World A - 7:11, warm dusk (from over the sea)"),
        "aerial_b": ("B", "city2_aerial_b.png", (-70.0, 120.0, -235.0), (8.0, 4.0, -8.0), 44.0, (1600, 1000),
                     "World B - now, cold ruin (from over the sea)"),
        "street_a": ("A", "city2_street_a.png", (0.0, 0.0, 0.0), (0.0, ROCK_Y + 12.0, 4.0), 72.0, (1600, 900),
                     "World A - on the west bastion: the bridge over the gorge to the Rock of the Bell"),
        "street_b": ("B", "city2_street_b.png", (0.0, 0.0, 0.0), (0.0, ROCK_Y + 12.0, 4.0), 72.0, (1600, 900),
                     "World B - the same place: the bridge has fallen in the middle"),
        "vista_a": ("A", "city2_vista_a.png", (10.0, GRAVE_Y + 15.0, 92.0), (0.0, 10.0, -12.0), 74.0, (1600, 900),
                    "World A - from the night bell tower over the graveyard: the theatre, the Rock of the Bell, the town, the sea"),
        "vista_b": ("B", "city2_vista_b.png", (10.0, GRAVE_Y + 15.0, 92.0), (0.0, 10.0, -12.0), 74.0, (1600, 900),
                    "World B - the same view, after the storm"),
    }
    for key, (world, name, eye, target, fov, size, title) in views.items():
        if key.startswith("street"):                                                 # on the west bastion, at the bridge
            eye = (-31.0, ROCK_Y + 1.7, -1.6)
            target = (0.0, ROCK_Y + 7.0, 2.0)
        if only in ("", key):
            n = render(world, os.path.join(OUT, name), eye, target, fov, size, title)
            print("%s: %d polygons drawn" % (name, n))
    if only in ("", "plan"):
        n = render("A", os.path.join(OUT, "city2_plan.png"), (4.0, 300.0, -2.0), (4.0, 0.0, -2.0001), 0.0, (1400, 1300),
                   "Plan: red = the main path (a spiral round the rock), blue = shortcuts that loop back", plan_overlay, ortho=5.4)
        print("city2_plan.png: %d polygons drawn" % n)
    return 0


if __name__ == "__main__":
    sys.exit(main())
