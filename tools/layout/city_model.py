"""city_model.py - the layout model of the town (docs/DESIGN.md 6 and 7).

A design model, not game data (yet): the town, its ground and its two moments, drawn so that the layout can be judged and
checked before it is moved into the game.

The town: one hill over a harbour, climbed in a spiral round the Rock of the Bell (heights in metres, y up):
    harbour   -6     the quay (the start), the sea gate, the headland and its lighthouse
    lower      -3..0  the lower town behind the sea wall, arches over its streets
    cistern  -10..-6  under the centre: the well, a tunnel, the floor of the gorge; water in A, walked in B
    west       0..14  the market stairs, the market and its great well, the west bastion
    centre    14      the Rock of the Bell: a round plaza in a gorge, the church, the bell tower; two bridges
    east       3..21  the east stair down from the rock, five locks climbing to the water tower
    north     21..28  the upper street to the top of the theatre, which faces the rock
    cliff     42      the graveyard and the night bell
The main path (docs/DESIGN.md 7.4) has no shortcuts back to finished places: its turns come from seeing a place first and
reaching it from an unexpected direction (the bridge seen from the bastion, reached from the floor of the gorge).

The moments: A, the past, warm dusk (amber light from a low sun, violet shade, terracotta, copper, lit windows); B, the
present, a cold ruin (moonlight, blue-grey stone, slate, verdigris, moss; the only warm light is a few lanterns and the
circle of the lantern). The storm came from the sea: damage follows how exposed a place was. The lantern, set down in A,
keeps a circle of A standing in B (the street_b view shows it on the west bridge).

    python tools/layout/city_model.py            (every view and the check; Python 3, numpy, matplotlib)
    python tools/layout/city_model.py check      (only the check of the numbers against the limits of the game)
    python tools/layout/city_model.py export     (the check, then the town as data for the game: data/town.dlt)
    python tools/layout/city_model.py aerial_a   (one view: aerial_a, aerial_b, street_a, street_b, vista_a, vista_b, plan)

Units: metres; x east, z north, y up. Views are drawn with a painter's algorithm and flat shading, the look of the game:
one colour per face, an outline on every edge.
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
MARKET_Y = 6.0
THEATRE = (0.0, 38.0)  # the centre of the orchestra; the seats rise north, facing the rock
THEATRE_Y = 22.0
GRAVE_Y = 42.0
GRAVE_Z0 = 78.0          # the graveyard: |x| < 24, z 78 .. 96, on the hill north of the town wall
BRIDGEHEADS = {"west": (-27.0, 0.0), "east": (27.0, 0.0)}
LOCK_LINE = spline([(36, -16), (48, -19), (60, -12), (70, -1), (77, 10), (80, 22)], 1.0)
LOCK_LEVELS = [3.0, 7.5, 12.0, 16.5, 21.0]
HARBOUR = (-22.0, -96.0, 34.0, -70.0)  # the basin: x0, z0, x1, z1
QUAY_Y = -6.2
LOCK_HEAD_RUN = 8.0      # the stair beside each lock head climbs 4.5 m over this
SLOPE = {"street": 0.445, "lane": 0.445, "stair": 0.625}  # tan 24 and 32 degrees (docs/DESIGN.md 11)
JUMP_UP = 1.2            # the step up on the main path
LANTERN_DEMO = (-19.0, ROCK_Y + 0.05, 0.0, 3.0)  # x, y, z, radius: a circle drawn on the west bridge (the picture street_b)
LIFT_X = -5.5             # the lift of the rock (docs/DESIGN.md 7.4): its shaft, south face, x
LIFT_HOUSE_Z = -19.4      # the lift house at the foot (2 m deep): 2 m clear between it and the shaft
LIFT_Z0, LIFT_Z1 = -16.4, -15.2  # the column of the shaft and of the ladder of B against the face (clear of the slope of the rock)

STREETS = [
    # name, points, width, kind ("street", "stair", "lane"). In this order: a street that starts or ends on an earlier one
    # takes its height there
    ("harbour road", [(-22, -73), (0, -72), (34, -72)], 4.0, "lane"),
    ("headland path", [(-22, -73), (-44, -78), (-66, -84), (-86, -86)], 2.4, "lane"),
    ("sea stair", [(8, -72), (4, -67), (0, -62), (-1, -56)], 3.0, "stair"),          # through the middle of the sea gate
    ("lower street", [(-1, -56), (-14, -51), (-28, -47), (-40, -42), (-50, -35)], 4.0, "street"),
    ("lower east lane", [(-1, -56), (14, -53), (26, -46), (31, -33)], 3.0, "lane"),
    ("market stairs", [(-50, -35), (-58, -24), (-61, -11), (-62, 2), (-59, 11)], 4.0, "stair"),  # clear of the well
    ("market street", [(-61, -11), (-46, -7), (-35, -3), (-27, 0)], 4.6, "street"),
    ("east stair", [(27, 0), (33, -3), (38, -8), (42, -11)], 3.6, "stair"),
    ("upper street", [(78, 28), (66, 36), (52, 46), (34, 52), (16, 56), (2, 57)], 4.0, "street"),
    ("cliff stair", [(10, 57), (8, 68), (6, 78.6)], 2.6, "stair"),                     # straight through its gate
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


def quay_level(s):
    """The quay beside the locks at distance s along LOCK_LINE: the level of its lock, and a stair of LOCK_HEAD_RUN m beside
    each lock head from one level to the next."""
    s = np.asarray(s, float)
    total = lengths(LOCK_LINE)[-1]
    n = len(LOCK_LEVELS)
    y = np.array([lock_level(lock_index(v)) for v in np.ravel(s)]).reshape(np.shape(s))
    for k in range(1, n):
        head = total * k / n
        t = np.clip((s - head + LOCK_HEAD_RUN / 2) / LOCK_HEAD_RUN, 0.0, 1.0)
        ramp = LOCK_LEVELS[k - 1] + (LOCK_LEVELS[k] - LOCK_LEVELS[k - 1]) * t
        y = np.where(np.abs(s - head) < LOCK_HEAD_RUN / 2, ramp, y)
    return y


PROBLEMS = []  # what the check found: (place, what)


def street_heights(name, curve, kind, h0, h1, ground_before):
    """The heights along a street: the ground, smoothed, between its two ends (pinned), never steeper than its kind allows.
    An end that cannot be reached within the slope is a problem of the layout and is reported."""
    k = SLOPE[kind]
    h = ground_before(curve[:, 0], curve[:, 1])
    kern = np.ones(9) / 9.0
    h = np.convolve(np.r_[np.full(4, h[0]), h, np.full(4, h[-1])], kern, mode="valid")
    d = lengths(curve)
    total = d[-1]
    h = 0.5 * h + 0.5 * (h0 + (h1 - h0) * d / total)    # half the ground, half an even climb: no wall of steps at one end
    if abs(h1 - h0) > k * total + 1e-6:
        PROBLEMS.append((name, "its ends differ by %.1f m over %.0f m: steeper than %s allows" % (h1 - h0, total, kind)))
    lo = np.maximum(h0 - k * d, h1 - k * (total - d))
    hi = np.minimum(h0 + k * d, h1 + k * (total - d))
    hi = np.maximum(hi, lo)
    ds = np.maximum(np.diff(d), 1e-6)
    for _ in range(8):
        h = np.clip(h, lo, hi)
        h[0], h[-1] = h0, h1
        for i in range(1, len(h)):
            h[i] = np.clip(h[i], h[i - 1] - k * ds[i - 1], h[i - 1] + k * ds[i - 1])
        for i in range(len(h) - 2, -1, -1):
            h[i] = np.clip(h[i], h[i + 1] - k * ds[i], h[i + 1] + k * ds[i])
    h[0], h[-1] = h0, h1
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
    y = np.where(d < 4.0, lv - 4.0, np.where(d < 8.5, quay_level(s), y))                                     # the basins and quays
    y = np.where(np.hypot(x - WELL[0], z - WELL[1]) < WELL_R + 5.0, MARKET_Y, y)                                    # the market of the well
    rt = np.hypot(x - THEATRE[0], z - THEATRE[1])                                                               # the theatre
    bowl = (z > THEATRE[1] - 3.0) & (rt < 17.0)
    seats = np.where(rt < 6.0, THEATRE_Y, THEATRE_Y + 0.9 * np.minimum(np.ceil((rt - 6.0) / 1.3), 9.0))
    y = np.where(bowl, seats, y)
    y = np.where((np.abs(x - THEATRE[0]) < 10.0) & (z > THEATRE[1] - 9.0) & (z <= THEATRE[1] - 3.0), THEATRE_Y, y)   # the stage terrace
    y = np.where((np.abs(x) < 24.0) & (z > GRAVE_Z0) & (z < GRAVE_Z0 + 18.0), GRAVE_Y, y)                                     # the graveyard
    x0, z0, x1, z1 = HARBOUR
    quay = (x > x0 - 4.0) & (x < x1 + 4.0) & (z > z0 + 18.0) & (z < z1 + 4.0)
    y = np.where(quay, QUAY_Y, y)
    return y


def _build_heights():
    """Each street in turn: an end on an earlier street takes that street's height there, else the carved ground."""
    out = {}
    for name, (c, w, kind) in STREET.items():
        ends = []
        for p in (c[0], c[-1]):
            pin = None
            for other, h in out.items():
                oc, ow, _ = STREET[other]
                dist, idx = distance_to(oc, p[0], p[1])
                if dist < ow / 2 + 0.6:
                    pin = float(h[idx])
                    break
            ends.append(pin if pin is not None else float(carved(p[0], p[1])))
        out[name] = street_heights(name, c, kind, ends[0], ends[1], carved)
    return out


_HEIGHTS = _build_heights()


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


FOLIAGE = ("tree", "cypress", "moss")  # crowns and scrub: drawn, not solid (the body goes through leaves, not over them)


def cone(cx, cz, r, y0, y1, mat, world="both", n=8):
    ring = ngon(cx, cz, r, n)
    kind = "overlay" if mat in FOLIAGE else "solid"
    for i in range(n):
        (ax, az), (bx, bz) = ring[i], ring[(i + 1) % n]
        poly([(ax, y0, az), (bx, y0, bz), (cx, y1, cz)], mat, world, kind)


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


# ---- the centre (docs/DESIGN.md 7.4) -----------------------------------------------------------------------------------

def build_west_gatehouse():
    """The gate of the west bridge on the rock: a wall across the end of the bridge, 4.5 m over the plaza, 10 m wide (no hop
    round it from the rails); its opening shut in A by a portcullis, choked in B by its own fallen arch. The bridge is seen,
    never crossed: in A it leads to a shut gate, in B it is gone."""
    gx = -(ROCK_R - 0.3)
    for z0, z1 in ((-5.0, -1.7), (1.7, 5.0)):
        prism(rect(gx, (z0 + z1) / 2, 1.0, z1 - z0, 0.0), ROCK_Y - 1.0, ROCK_Y + 4.5, "stone", "both", True, "stone_dark")
    prism(rect(gx, 0.0, 1.0, 3.4, 0.0), ROCK_Y + 3.2, ROCK_Y + 4.5, "stone", "both", True, "stone_dark")    # the lintel
    prism(rect(gx - 0.35, 0.0, 0.15, 3.4, 0.0), ROCK_Y - 0.2, ROCK_Y + 3.2, "iron", "A")                     # A: the portcullis
    prism(rect(gx, 0.0, 1.0, 3.4, 0.0), ROCK_Y - 0.2, ROCK_Y + 3.2, "debris", "B", True, "stone_dark")      # B: the fallen arch
    for k in range(3):                                                                          # B: the heap spills on the plaza
        prism(rect(gx + 1.2 + 0.9 * k, 0.3 * (k - 1), 1.0, 2.6 - 0.5 * k, 0.1 * k), ROCK_Y - 0.2, ROCK_Y + 1.6 - 0.5 * k, "debris", "B")


def build_bridge_wreck():
    """B: the spans of the west bridge fell into the gorge and lie round its floor like a stair, from under the stub on the
    bastion down to the floor, going north: the way down in the present (in the past the gorge holds the moat)."""
    for i in range(13):
        a = math.radians(174.0 - 7.0 * i)
        top = ROCK_Y - 1.5 * (i + 1)
        cx, cz = 20.0 * math.cos(a), 20.0 * math.sin(a)
        # Only the first two deck faces keep their lighter cut stone. The outline
        # and collision stay identical; the rest of the wreck is weathered debris.
        deck_face = "paving" if i < 2 else "debris"
        prism(rect(cx, cz, 3.0, 2.2, a), GORGE_Y - 0.5, top, "stone", "B", True, deck_face)
        KEEP_CLEAR.append((cx, cz, 1.0))


def build_gorge_parapet():
    """A parapet on the rim of the gorge, 2.4 m over the ground there, open only at the two bridges: the gorge is entered by
    the wreck of the west bridge in B and left by the lift, not jumped into from anywhere."""
    r = GORGE_R + 1.7
    n = 120
    for i in range(n):
        a0, a1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
        am = (a0 + a1) / 2
        if abs(r * math.sin(am)) < 1.7 and abs(math.cos(am)) > 0.9:
            continue                                                                    # the bridges
        p0 = np.array([r * math.cos(a0), r * math.sin(a0)])
        p1 = np.array([r * math.cos(a1), r * math.sin(a1)])
        v = (p1 - p0) / np.linalg.norm(p1 - p0)
        nx, nz = -v[1] * 0.3, v[0] * 0.3
        pts = [(p0[0] + nx, p0[1] + nz), (p1[0] + nx, p1[1] + nz), (p1[0] - nx, p1[1] - nz), (p0[0] - nx, p0[1] - nz)]
        out = [float(ground(q[0] * 1.08, q[1] * 1.08)) for q in (p0, p1)] + [float(ground(q[0], q[1])) for q in (p0, p1)]
        prism(pts, min(out) - 2.0, max(out) + 2.4, "stone", "both", True, "stone_dark")
        KEEP_CLEAR.append((float(p0[0]), float(p0[1]), 1.2))


def build_lift_frame():
    """The lift of the rock, the parts that do not change: the posts of its shaft on the south face, the upper station (a
    landing at the edge of the plaza, both moments), the lift house at the foot (its roof just over the moat of A) and, in
    B, a heap beside the lift house to climb onto its roof. The cage and the ladder are the thing "lift" (build_lift)."""
    for sx in (-1.6, 1.6):
        prism(rect(LIFT_X + sx, (LIFT_Z0 + LIFT_Z1) / 2, 0.4, LIFT_Z1 - LIFT_Z0, 0.0), GORGE_Y, ROCK_Y + 3.0, "stone_dark")
    gable(LIFT_X, (LIFT_Z0 + LIFT_Z1) / 2, 3.6, 2.0, 0.0, ROCK_Y + 3.0, 1.0, "roof")
    prism(rect(0.0, (LIFT_Z1 - 12.4) / 2 + 0.0, 3.2, -12.4 - LIFT_Z1, 0.0), ROCK_Y - 1.0, ROCK_Y, "stone", "both", True, "paving")
    prism(rect(LIFT_X, LIFT_HOUSE_Z, 3.2, 2.0, 0.0), GORGE_Y, GORGE_Y + 2.6, "stone_dark", "both", True, "roof")  # the lift house
    prism(rect(LIFT_X - 2.4, LIFT_HOUSE_Z, 1.4, 1.8, 0.0), GORGE_Y, GORGE_Y + 1.3, "debris", "B")             # B: a heap by it


def build_lift():
    """The causal rule at the scale of the rock. A: the cage of the lift waits at the top; its winch is on the roof of the lift
    house at the foot, just over the moat (reached by a switch on the roof, from B). LOWER brings the cage down.
    B, the cage never lowered: it fell when its rope rotted and tore the frame off the face: a wreck at the foot.
    B, the cage lowered in A: it sat at the foot for a century and the frame stands: a ladder of girders up the face, two
    rows against the rock, steps of 1.5 m, to the upper station."""
    zc = (LIFT_Z0 + LIFT_Z1) / 2
    hz = (LIFT_Z1 - LIFT_Z0) / 2
    boxes = [
        ("shut", "A", "iron", (LIFT_X, ROCK_Y + 1.1, zc), (1.2, 1.2, hz), 0.0),                # the cage at the top
        ("open", "A", "iron", (LIFT_X, GORGE_Y + 1.2, zc), (1.2, 1.2, hz), 0.0),               # the cage at the foot
        ("open", "B", "iron", (LIFT_X, GORGE_Y + 1.0, zc), (1.2, 1.0, hz), 0.0),               # B: there still, rusted
        ("shut", "B", "debris", (LIFT_X, GORGE_Y + 0.5, zc - 0.4), (1.5, 0.5, hz + 0.4), 0.2),  # B: the wreck
        ("shut", "B", "iron", (LIFT_X + 1.6, GORGE_Y + 0.3, zc - 1.2), (2.0, 0.3, 0.3), 0.5),
    ]
    front = (LIFT_Z0 - 1.2, LIFT_Z0 - 0.1)                                              # the first row stands out from the face
    for i in range(13):                                                                 # B, lowered: the ladder
        top = GORGE_Y + 1.5 + 1.5 * i
        if i < 7:
            x0 = -2.4 + 1.2 * i
            z0, z1 = front
        else:
            x0 = 4.8 - 1.2 * (i - 7)
            z0, z1 = LIFT_Z0, LIFT_Z1 - 0.1
        boxes.append(("open", "B", "iron", (x0 + 0.6, top - 0.15, (z0 + z1) / 2), (0.6, 0.15, (z1 - z0) / 2), 0.0))
    thing("lift", (LIFT_X, GORGE_Y + 2.6 + 1.1, LIFT_HOUSE_Z), boxes, "lower")


# ---- the landmarks ----------------------------------------------------------------------------------------------------------

def build_rock():
    cx, cz = TOWER
    poly([(x, ROCK_Y + 0.02, z) for x, z in ngon(0, 0, ROCK_R - 0.3, 32)], "paving", "both", "overlay")
    for k in range(24):                                                                 # the balustrade of the plaza
        a0, a1 = 2 * math.pi * k / 24, 2 * math.pi * (k + 1) / 24
        if abs(math.cos(a0)) > 0.97:
            continue                                                                    # the two bridges
        if k in (17, 18):
            continue                                                                    # the upper station of the lift
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
        lamp(tx + 4.0 * math.cos(a), ROCK_Y + 25.8, tz + 4.0 * math.sin(a), "both", "memory")
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
        if z < -12.0 and abs(x) < 10.0:
            continue                                                                    # the foot of the lift stays clear
        prism(rect(x, z, 1.2 + hash01(k, 7) * 2.0, 1.0 + hash01(k, 9) * 1.6, ang), GORGE_Y, GORGE_Y + 0.4 + hash01(k, 11) * 1.4, "debris", "B")
        if k % 3 == 0:
            cone(x + 1.5, z, 0.8, GORGE_Y, GORGE_Y + 1.2, "moss", "B", 5)
    # the bridges: west and east, arched; in B the west one has fallen in the middle
    for name, sgn in (("west", -1.0), ("east", 1.0)):
        x0, x1 = sgn * (ROCK_R - 0.2), BRIDGEHEADS[name][0] - sgn * 1.5    # from the rock onto the flat of the bastion
        n = 8
        runs = [(0, 7, "A"), (7, 8, "both")] if name == "west" else [(0, 8, "both")]
        for k0, k1, world in runs:                                     # whole runs: a rail is one piece, not steps
            xa = x0 + (x1 - x0) * k0 / n
            xb = x0 + (x1 - x0) * k1 / n
            beam((xa, ROCK_Y + 0.05, 0.0), (xb, ROCK_Y + 0.05, 0.0), 3.2, 1.0, "stone", world)
            for sz in (-1.5, 1.5):
                if name == "west" and world == "both" and sz > 0:
                    # This short north rail overlooks the first fallen span. Keep its
                    # footprint, height and deck unchanged, but let the view through in B.
                    beam((xa, ROCK_Y + 1.0, sz), (xb, ROCK_Y + 1.0, sz), 0.3, 0.95, "stone", "A")
                    lo, hi = sorted((xa, xb))
                    for j in range(4):
                        x = lo + 0.07 + (hi - lo - 0.14) * j / 3
                        prism(rect(x, sz, 0.14, 0.30, 0.0), ROCK_Y + 0.05, ROCK_Y + 1.0,
                              "iron", "B")
                    beam((xa, ROCK_Y + 1.0, sz), (xb, ROCK_Y + 1.0, sz), 0.30, 0.10, "iron", "B")
                else:
                    beam((xa, ROCK_Y + 1.0, sz), (xb, ROCK_Y + 1.0, sz), 0.3, 0.95, "stone", world)
        mid = (x0 + x1) / 2
        prism(rect(mid, 0.0, 1.6, 3.2, 0.0), GORGE_Y, ROCK_Y - 2.5, "stone_dark", "A" if name == "west" else "both")
    build_west_gatehouse()
    build_bridge_wreck()
    build_lift_frame()
    for x, z in ((-8.0, -9.5), (8.0, -9.5), (-9.0, 8.0), (9.0, 8.0)):                            # plane trees on the plaza
        prism(ngon(x, z, 0.3, 5), ROCK_Y, ROCK_Y + 2.5, "wood")
        cone(x, z, 2.6, ROCK_Y + 2.0, ROCK_Y + 6.5, "tree", "A", 9)
        cone(x, z, 1.4, ROCK_Y + 2.0, ROCK_Y + 4.0, "tree", "B", 6)
    for x, z in ((-5.0, -11.5), (5.0, -11.5)):                                                  # lamp posts: lit in both
        prism(ngon(x, z, 0.12, 4), ROCK_Y, ROCK_Y + 3.4, "iron")
        prism(ngon(x, z, 0.3, 6), ROCK_Y + 3.4, ROCK_Y + 4.0, "lantern", "A" if x < 0 else "both")
        lamp(x, ROCK_Y + 3.7, z, "A" if x < 0 else "both", "post")


GATES = {}


def build_wall():
    h = ground(WALL[:, 0], WALL[:, 1])
    d = lengths(WALL)
    for i in range(len(WALL) - 1):
        t = d[i] / d[-1]
        if t < 0.012 or t > 0.988:
            continue                                                                    # the sea gate
        a, b = WALL[i], WALL[i + 1]
        m = (a + b) / 2
        through = [nm for nm, (c, w, k) in STREET.items() if nm != "sea stair" and distance_to(c, m[0], m[1])[0] < w / 2 + 1.6]
        if through:                                                                     # a gate where a street goes through
            if through[0] not in GATES:
                GATES[through[0]] = m
            continue
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 0.9, v[0] * 0.9
        # 4.5 m over the highest ground within 5 m on either side: no wall is a step from a slope beside it
        near = [float(ground(q[0] + o * nx / 0.9, q[1] + o * nz / 0.9)) for q in (a, b) for o in (-5.0, -2.5, 2.5, 5.0)]
        base = min(h[i], h[i + 1]) - 3.0
        top = max([h[i], h[i + 1]] + near) + 4.5
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
    for name, (mx, mz) in GATES.items():                                                # the gates of the streets
        c, w, _ = STREET[name]
        dist, idx = distance_to(c, mx, mz)
        idx = int(min(idx, len(c) - 2))
        v = c[idx + 1] - c[idx]
        ang = math.atan2(v[1], v[0])
        y = float(_HEIGHTS[name][idx])
        for sg in (-1, 1):
            px_, pz_ = mx - v[1] / np.linalg.norm(v) * sg * (w / 2 + 1.0), mz + v[0] / np.linalg.norm(v) * sg * (w / 2 + 1.0)
            prism(rect(px_, pz_, 1.6, 2.0, ang), y - 2.0, y + 7.0, "stone")
        lint = rect(mx, mz, 2.0, w + 3.6, ang)
        prism(lint, y + 4.2, y + 5.4, "stone", "A")
    gx, gz = WALL_POINTS[0]                                                             # the sea gate
    yg = float(ground(gx, gz))
    for dx in (-4.5, 4.5):
        prism(rect(gx + dx, gz, 4.0, 4.4, 0.0), yg - 1.0, yg + 11.0, "stone")
        cone(gx + dx, gz, 3.0, yg + 11.0, yg + 15.0, "roof", "A", 4)
    prism(rect(gx, gz, 5.0, 4.4, 0.0), yg + 5.5, yg + 11.0, "stone", "A")


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
    deck_y = QUAY_Y + 0.3
    prism(ngon(lx, lz, 2.45, 12), SEA - 4.0, deck_y, "stone_dark", "both", True, "paving")
    prism(ngon(lx, lz, 1.4, 8), deck_y, QUAY_Y + 7.0, "stone", "A")
    prism(ngon(lx, lz, 1.6, 8), QUAY_Y + 7.0, QUAY_Y + 8.2, "lantern", "A")
    lamp(lx, QUAY_Y + 7.6, lz, "A", "light")
    cone(lx, lz, 1.8, QUAY_Y + 8.2, QUAY_Y + 10.0, "copper", "A", 8)
    prism(ngon(lx, lz, 1.4, 8), deck_y, QUAY_Y + 3.0, "stone", "B")
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
    build_quay_works(z1 - 3.0)
    build_harbour_frontage(z1 - 3.0)
    # The damaged mole sheds stone at its break, rather than leaving stepping
    # stones across the basin. Keep the highest debris below the quay deck.
    for i, (along, offset, radius) in enumerate(((0.53, -1.2, 0.85), (0.56, 0.9, 1.1),
                                                  (0.59, -0.5, 0.95), (0.62, 1.5, 0.8))):
        pos, tangent = at(mole, along)
        x, z = pos + offset * np.array([-tangent[1], tangent[0]])
        ring = ngon(x, z, radius, 6, hash01(i, 211))
        prism(ring, min(SEA - 4.0, float(ground(x, z)) - 0.3),
              SEA + 0.08 + 0.18 * hash01(i, 213), "debris", "B")
    hx, hz = -86.0, -88.0                                                               # the lighthouse on the headland
    y = float(ground(hx, hz))
    prism(ngon(hx, hz, 2.2, 10), y, y + 18.0, "plaster3")
    prism(ngon(hx, hz, 2.6, 10), y + 18.0, y + 20.0, "lantern", "A")
    lamp(hx, y + 19.0, hz, "A", "light")
    prism(ngon(hx, hz, 2.6, 10), y + 18.0, y + 20.0, "iron", "B")
    cone(hx, hz, 2.8, y + 20.0, y + 23.0, "roof", "both", 10)


def build_harbour_frontage(edge):
    """Separate the working shore from the through route without changing the tutorial."""
    deck = QUAY_Y
    # Two quay aprons with chamfered ends. Leave the slip, the start-area wreck
    # and the sea-stair corridor unchanged. The seaward edge is not a shortcut.
    for left, right, reach in ((-18.0, -5.0, 4.1), (15.0, 32.0, 4.6)):
        apron = [(left, edge + 0.04), (left + 2.0, edge - reach),
                 (right - 2.0, edge - reach), (right, edge + 0.04)]
        prism(apron, SEA - 4.0, deck + 0.025, "stone_dark", top_mat="stone")
        for i in range(3):
            t = (i + 0.5) / 3.0
            x = left + 2.8 + t * (right - left - 5.6)
            prism(ngon(x, edge - reach + 0.35, 0.21, 6), deck, deck + 0.34, "stone_dark")

    # Low timber work bays give the curtain wall a human-scale foreground.
    # The shared stone base and surviving rear wall explain the B-layer ruin.
    for x, width, depth, rise in ((-10.0, 5.2, 2.5, 0.9), (15.9, 3.6, 2.15, 0.70)):
        z = edge + 7.1
        y = float(ground(x, z))
        if not math.isfinite(y):
            raise ValueError("harbour work bay has no ground")
        top = y + 2.40
        prism(rect(x, z, width, depth, 0.0), min(deck, y) - 0.45, y + 0.12, "stone_dark")
        back_z = z + depth / 2.0 - 0.13
        prism(rect(x, back_z, width, 0.26, 0.0), y + 0.08, top, "plaster3", "A")
        prism(rect(x, back_z, width, 0.26, 0.0), y + 0.08, y + 0.74, "stone", "B")
        for dx in (-width / 2.0 + 0.16, width / 2.0 - 0.16):
            for dz in (-depth / 2.0 + 0.15, depth / 2.0 - 0.15):
                prism(rect(x + dx, z + dz, 0.21, 0.21, 0.0), y + 0.08, top + 0.04, "wood", "A")
        for dz in (-depth / 2.0 + 0.15, depth / 2.0 - 0.15):
            beam((x - width / 2.0, top, z + dz), (x + width / 2.0, top, z + dz), 0.24, 0.24, "wood", "A")
        gable(x, z, width + 0.55, depth + 0.65, 0.0, top + 0.10, rise, "roof", "A")
        prism(rect(x, z + 0.40, width * 0.65, 0.52, 0.0), y + 0.09, y + 0.8, "wood", "A")


def build_quay_works(edge):
    """Fit working berths to the existing quay; keep the sea stair and tutorial lane clear."""
    deck = QUAY_Y
    # Mooring stones and timber fenders sit on or against the masonry edge.
    for i, x in enumerate((-21.0, -15.0, -9.0, -3.0, 19.0, 24.0, 31.0)):
        prism(ngon(x, edge + 0.32, 0.19, 6), deck - 0.10, deck + 0.38, "stone_dark")
        h = deck - 0.12 if i % 3 == 0 else SEA + 0.4
        prism(rect(x, edge - 0.04, 0.24, 0.24, 0.0), SEA - 0.45, deck - 0.04, "wood", "A")
        prism(rect(x, edge - 0.04, 0.24, 0.24, 0.0), SEA - 0.45, h, "wood", "B")

    # A slip for hauling boats onto the quay, clear of the sea-gate approach.
    sx = -21.0
    start, end = edge + 0.12, edge - 6.0
    beam((sx, deck, start), (sx, SEA - 0.54, end), 2.8, 0.36, "stone_dark")
    for j in range(3):
        t = (j + 1) / 4.0
        z = start + (end - start) * t
        y = deck + (SEA - 0.54 - deck) * t - 0.10
        for side in (-1, 1):
            x = sx + side * 1.18
            prism(rect(x, z, 0.30, 0.38, 0.0), min(SEA - 4.0, float(ground(x, z)) - 0.2), y, "stone_dark")

    # The loading jib stands on a plinth in both layers; only the past retains its arm.
    x, z = 26.0, edge + 0.36
    prism(rect(x, z, 0.54, 0.54, 0.0), deck - 0.15, deck + 0.16, "stone")
    prism(rect(x, z, 0.26, 0.26, 0.0), deck + 0.15, deck + 3.65, "wood", "A")
    beam((x, deck + 3.65, z + 0.18), (x, deck + 3.65, edge - 1.9), 0.24, 0.22, "wood", "A")
    beam((x, deck + 1.99, z), (x, deck + 3.48, edge - 1.45), 0.18, 0.22, "wood", "A")
    prism(rect(x, edge - 1.85, 0.04, 0.04, 0.0), deck + 1.35, deck + 3.50, "iron", "A")
    prism(rect(x, z, 0.26, 0.26, 0.0), deck + 0.15, deck + 0.48, "wood", "B")


def build_west():
    cx, cz = WELL
    for k in range(24):                                                                 # the parapet of the well
        a0, a1 = 2 * math.pi * k / 24, 2 * math.pi * (k + 1) / 24
        beam((cx + (WELL_R + 0.5) * math.cos(a0), MARKET_Y + 1.0, cz + (WELL_R + 0.5) * math.sin(a0)),
             (cx + (WELL_R + 0.5) * math.cos(a1), MARKET_Y + 1.0, cz + (WELL_R + 0.5) * math.sin(a1)), 1.0, 1.0, "stone")
    ring = ngon(cx, cz, WELL_R, 24)
    for i, (x, z) in enumerate(ring):
        x2, z2 = ring[(i + 1) % 24]
        poly([(x, -10.0, z), (x2, -10.0, z2), (x2, MARKET_Y, z2), (x, MARKET_Y, z)], "stone_dark")
    for k in range(16):                                                                 # the stair down its wall (A: wood)
        a0 = 2 * math.pi * k / 16
        a1 = a0 + 2 * math.pi / 16
        y = MARKET_Y - 1.0 - k
        world = "A" if 6 <= k <= 9 else "both"
        pts = [(cx + (WELL_R - 1.4) * math.cos(a0), cz + (WELL_R - 1.4) * math.sin(a0)), (cx + WELL_R * math.cos(a0), cz + WELL_R * math.sin(a0)),
               (cx + WELL_R * math.cos(a1), cz + WELL_R * math.sin(a1)), (cx + (WELL_R - 1.4) * math.cos(a1), cz + (WELL_R - 1.4) * math.sin(a1))]
        prism(pts, y - 0.3, y, "wood" if world == "A" else "stone_dark", world)
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
    poly([(tx - 1.0, y + 13.0, tz - 2.27), (tx + 1.0, y + 13.0, tz - 2.27), (tx + 1.0, y + 15.0, tz - 2.27), (tx - 1.0, y + 15.0, tz - 2.27)],
         "gold", "A")                                                                   # the clock face
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
        if i < len(LOCK_LEVELS) - 1:
            prism(rect(a[0], a[1], 0.8, 8.0, ang), bed, LOCK_LEVELS[i + 1] - 0.6, "stone_dark")     # the lock head
            prism(rect(a[0] - v[0] * 0.5, a[1] - v[1] * 0.5, 0.3, 7.6, ang), lv - 0.6, LOCK_LEVELS[i + 1] - 0.2, "wood", "A")  # its gate
            poly([(a[0] - v[0] * 0.7 - v[1] * 1.2, LOCK_LEVELS[i + 1] - 0.7, a[1] - v[1] * 0.7 + v[0] * 1.2),
                  (a[0] - v[0] * 0.7 + v[1] * 1.2, LOCK_LEVELS[i + 1] - 0.7, a[1] - v[1] * 0.7 - v[0] * 1.2),
                  (a[0] - v[0] * 0.7 + v[1] * 1.2, lv - 0.6, a[1] - v[1] * 0.7 - v[0] * 1.2),
                  (a[0] - v[0] * 0.7 - v[1] * 1.2, lv - 0.6, a[1] - v[1] * 0.7 + v[0] * 1.2)], "water", "A", "water")   # the fall (A)
    total = d[-1]
    # the quays, laid as flat tiles of 0.5 m over the coarse ground (an offset strip folds inside a bend): each at the level
    # of the quay at the nearest point of the lock line, so a lock stair is a run of low steps
    xs = np.arange(LOCK_LINE[:, 0].min() - 9.0, LOCK_LINE[:, 0].max() + 9.0, 0.5)
    zs = np.arange(LOCK_LINE[:, 1].min() - 9.0, LOCK_LINE[:, 1].max() + 9.0, 0.5)
    gx, gz = np.meshgrid(xs + 0.25, zs + 0.25)
    dist, idx = distance_to(LOCK_LINE, gx, gz)
    sv = d[np.minimum(idx, len(d) - 1)]
    lev = quay_level(sv) + 0.3
    for j in range(len(zs)):
        for i in range(len(xs)):
            if 4.3 <= dist[j, i] <= 8.2:
                x0_, z0_, y = xs[i], zs[j], float(lev[j, i])
                poly([(x0_, y, z0_), (x0_ + 0.5, y, z0_), (x0_ + 0.5, y, z0_ + 0.5), (x0_, y, z0_ + 0.5)], "paving")
    # the stairs beside the lock heads
    for k in range(1, len(LOCK_LEVELS)):
        head = total * k / len(LOCK_LEVELS)
        for side in (-1, 1):
            for u in np.arange(-LOCK_HEAD_RUN / 2, LOCK_HEAD_RUN / 2 + 0.01, 0.5):
                p, v = at(LOCK_LINE, float(np.clip((head + u) / total, 0, 1)))
                n = np.array([-v[1], v[0]]) * side
                y = float(quay_level(head + u)) + 0.04
                q0, q1 = p + n * 4.4, p + n * 8.2
                poly([(q0[0], y, q0[1]), (q1[0], y, q1[1]), (q1[0], y - 0.28, q1[1]), (q0[0], y - 0.28, q0[1])], "stone_dark", "both", "overlay")
    lv = LOCK_LEVELS[-1]                                                                # the walkway over the head of the
    prism(rect(80.0, 19.6, 13.0, 1.6, 0.25), lv - 0.4, lv + 0.02, "wood", "both", True, "wood")  # top basin: the quays meet
    for sx in (-6.0, 6.0):
        prism(rect(80.0 + sx * math.cos(0.25), 19.6 + sx * math.sin(0.25), 0.3, 0.3, 0.25), lv - 4.0, lv - 0.4, "wood")
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
        z = GRAVE_Z0 + 4.0 + (k * 13 % 14)
        prism(rect(x, z, 0.8, 0.35, (k % 5) * 0.1), GRAVE_Y, GRAVE_Y + 1.0 + (k % 3) * 0.2, "stone", "A" if k % 7 == 0 else "both")
    for k in range(10):
        x, z = -22.0 + k * 5.0, GRAVE_Z0 + 17.0
        cone(x, z, 0.9, GRAVE_Y, GRAVE_Y + 7.0, "cypress", "both", 6)
    nx_, nz_ = 16.0, GRAVE_Z0 + 10.0                                                               # the night bell tower
    prism(rect(nx_, nz_, 4.0, 4.0, 0.1), GRAVE_Y, GRAVE_Y + 14.0, "stone")
    cone(nx_, nz_, 3.2, GRAVE_Y + 14.0, GRAVE_Y + 19.0, "roof", "both", 4)
    prism(ngon(nx_, nz_, 1.2, 10), GRAVE_Y + 10.5, GRAVE_Y + 12.6, "gold")
    prism(ngon(-12.0, GRAVE_Z0 + 4.0, 0.3, 6), GRAVE_Y + 0.4, GRAVE_Y + 1.2, "lantern", "B")       # a lantern on a new grave (B)
    lamp(-12.0, GRAVE_Y + 0.8, GRAVE_Z0 + 4.0, "B", "grave")


def build_arches():
    """Arches over the streets: a house front that bridges the street, every 25 m or so on the lower and market streets and
    the market stairs; in B a third of them have fallen (a heap under the gap)."""
    k = 0
    for name in ("lower street", "market stairs", "market street", "upper street"):
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
            if min(math.hypot(p[0] - bx, p[1] - bz) for bx, bz in BRIDGEHEADS.values()) < 14.0:
                s += 6.0
                continue                                                                # the bastion stays open
            if any(math.hypot(p[0] - kx, p[1] - kz) < kr + 3.0 for kx, kz, kr in KEEP_CLEAR if kr > 2.0):
                s += 6.0
                continue                                                                # not over a puzzle
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


def build_bastion_gate():
    """The gate tower of the west bridge on the bastion: the dark frame close by of the street view, the bridge through it,
    the rock beyond (three depths, docs/DESIGN.md 6.3)."""
    gx = BRIDGEHEADS["west"][0] + 2.2
    for sz in (-2.7, 2.7):
        prism(rect(gx, sz, 1.2, 1.2, 0.0), ROCK_Y - 1.0, ROCK_Y + 8.0, "stone_dark")
    prism(rect(gx, 0.0, 1.2, 6.6, 0.0), ROCK_Y + 6.4, ROCK_Y + 8.0, "stone_dark", "both", True, "stone")
    gable(gx, 0.0, 1.8, 7.0, 0.0, ROCK_Y + 8.0, 1.2, "roof", "A")
    prism(rect(gx, 0.0, 1.2, 6.6, 0.0), ROCK_Y + 8.0, ROCK_Y + 8.6, "stone", "B")


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
            if min(math.hypot(x - bx, z - bz) for bx, bz in BRIDGEHEADS.values()) < 9.0:
                s += 14.0
                continue
            prism(ngon(x, z, 0.1, 4), y, y + 3.2, "iron")
            on_path = name in ("sea stair", "lower street", "market stairs", "market street", "east stair", "upper street", "cliff stair")
            lit_b = (on_path and k2 % 2 == 0) or hash01(int(x * 10), int(z * 10)) < 0.08
            prism(ngon(x, z, 0.5 if lit_b else 0.25, 6), y + 3.2, y + 4.0, "lantern", "both" if lit_b else "A")
            lamp(x, y + 3.6, z, "both" if lit_b else "A", "street")                     # a real light now (the pool of light is the game's)
            if not lit_b:
                prism(ngon(x, z, 0.25, 6), y + 3.2, y + 3.4, "iron", "B")
            s += 14.0
            k2 += 1


# ---- things that change (docs/DESIGN.md 3.3, 8) ----------------------------------------------------------------------------
THINGS = []  # (name, handle (x, y, z), [(state "shut"/"open", world, material, (cx, cy, cz), (hx, hy, hz), yaw)])


def street_y(name, x, z):
    """The height of the surface of a street at the point of its middle line nearest to (x, z)."""
    c, _, _ = STREET[name]
    _, i = distance_to(c, x, z)
    return float(_HEIGHTS[name][int(i)])


VERB_CODE = {"open": 0, "push": 1, "lay": 2, "ring": 3, "lower": 4}


def thing(name, handle, boxes, verb="open"):
    """A thing that changes: E at its handle in A opens it (the causal rule: what is done in A shows in B). Its boxes are
    drawn and solid in the moment of their world, while the thing is in their state. Not in POLYS: the game draws them.
    `verb` is what the prompt says (open, push, lay, ring, lower); a bell is a thing with no boxes of its own but its rope."""
    THINGS.append((name, tuple(float(v) for v in handle), boxes, verb))


def build_harbour_wayfinding():
    # Worn stone channels follow the foot traffic and turn up the sea stair.
    # In B the pattern stops on the near side of the wreck, not around it.
    for start, end, world in ((12.9, 16.6, "both"), (7.9, 12.9, "A")):
        for x in np.arange(start, end - 0.12, 0.62):
            for z in (-72.82, -71.18):
                quad = rect(x + 0.22, z, 0.44, 0.19, 0.0)
                poly([(px, QUAY_Y + 0.09, pz) for px, pz in reversed(quad)],
                     "stone_dark", world, "overlay")
    c, w, _ = STREET["sea stair"]
    ys = _HEIGHTS["sea stair"]
    for i in range(0, len(c) - 1, 2):
        a, b = c[i], c[i + 1]
        d = b - a
        n = np.array([-d[1], d[0]]) / np.linalg.norm(d)
        for side in (-1, 1):
            shift = n * side * (w / 2 - 0.28)
            inner = n * side * 0.18
            corners = [(a + shift, ys[i]), (b + shift, ys[i + 1]),
                       (b + shift - inner, ys[i + 1]), (a + shift - inner, ys[i])]
            if side == -1:
                corners.reverse()
            poly([(q[0], y + 0.09, q[1]) for q, y in corners],
                 "stone_dark", "both", "overlay")


def build_harbour_beats():
    """The first two beats of the harbour (docs/DESIGN.md 8), each teaching one thing:
    1. the switch: on the quay, east of the sea stair, the storm left a heap of wreck across the quay in B (the start is in
       B, on the far side of it); in A the quay is clear.
    2. the causal rule: the portcullis of the sea gate is shut in both moments. In A its winch (gold) raises it, but the
       cargo of the customs stands stacked in the passage behind it; in B the cargo is long gone, and the portcullis is
       rusted where it was left: shut, or raised if it was raised in A."""
    qy = QUAY_Y
    for k, (x, z, w, d, h, a) in enumerate(((11.0, -72.5, 2.6, 3.0, 3.6, 0.15), (11.4, -69.4, 2.4, 3.2, 3.0, -0.2),
                                             (10.8, -66.4, 2.4, 3.2, 3.8, 0.1), (11.2, -63.6, 2.6, 2.6, 4.4, -0.1),
                                             (11.0, -75.6, 2.8, 3.4, 3.0, 0.3))):
        prism(rect(x, z, w, d, a), qy - 6.0, qy + h, "debris", "B")
    beam((9.6, qy + 2.6, -74.0), (12.4, qy + 3.4, -64.0), 0.5, 0.5, "wood", "B")           # a mast across the heap
    gx, gz = WALL_POINTS[0]
    yq = street_y("sea stair", gx, gz - 1.6)                                             # under the portcullis
    yc = street_y("sea stair", gx, gz + 1.7)                                             # under the cargo
    pz = gz - 1.6
    for i, dx in enumerate((-1.7, 0.0, 1.7)):                                            # the cargo: A only, people's
        prism(rect(gx + dx, gz + 1.7, 1.7, 1.4, 0.04 * (i - 1)), yc - 1.0, yc + 1.5, "wood", "A")
        prism(rect(gx + dx * 0.9, gz + 1.7, 1.5, 1.3, -0.05 * (i - 1)), yc + 1.5, yc + 3.2, "cloth2" if i == 1 else "wood", "A")
    ym = street_y("sea stair", gx + 3.3, gz - 3.2)
    thing("sea gate", (gx + 3.3, ym + 1.1, gz - 3.2), [
        ("shut", "A", "iron", (gx, yq + 2.6, pz), (2.55, 3.2, 0.12), 0.0),
        ("open", "A", "iron", (gx, yq + 7.0, pz), (2.55, 1.6, 0.12), 0.0),
        ("shut", "B", "iron", (gx, yq + 2.2, pz), (2.55, 2.8, 0.12), 0.0),
        ("open", "B", "iron", (gx, yq + 5.0, pz), (2.55, 1.2, 0.12), 0.0),
        ("shut", "both", "wood", (gx + 3.3, ym + 0.5, gz - 3.2), (0.35, 0.5, 0.35), 0.0),  # the winch: a drum on a post
        ("open", "both", "wood", (gx + 3.3, ym + 0.5, gz - 3.2), (0.35, 0.5, 0.35), 0.0),
    ])


AIR_STEP_S = 36.0   # metres along the lower street: where beat 3 closes it (flat there, between two arches)


def street_frame(name, s):
    """A point of a street `s` metres along it, its height, the unit vector along it and the one across it."""
    c, w, _ = STREET[name]
    d = lengths(c)
    p, v = at(c, s / d[-1])                                                              # as the game reads the line
    i = int(np.clip(np.searchsorted(d, s) - 1, 0, len(c) - 2))
    f = (s - d[i]) / max(d[i + 1] - d[i], 1e-9)
    y = float(_HEIGHTS[name][i] + f * (_HEIGHTS[name][i + 1] - _HEIGHTS[name][i]))
    return p, y, v, np.array([-v[1], v[0]]), w


def build_air_step():
    """Beat 3 of the harbour (docs/DESIGN.md 8), the switch in the air. The lower street is closed in both moments at one
    place: in A by the barricade of the customs (5 m of timber, too high to climb); in B by the front of a fallen house
    (its top 3.4 m over the street: too high to jump to from the street). In A two crates of the customs stand against the
    barricade (steps of 1.2 m). The answer: on the top crate in A, jump toward the barricade and switch in the air; the
    barricade and the crates are gone, the heap of B is under the feet. Its far side goes down in steps to the street.
    Nothing here needs a thing that changes."""
    p, y, v, n, w = street_frame("lower street", AIR_STEP_S)
    ang = math.atan2(v[1], v[0])
    for u in (-6.0, -3.0, 0.0, 3.0, 6.0):
        KEEP_CLEAR.append((float(p[0] + v[0] * u), float(p[1] + v[1] * u), w / 2 + 6.0))
    span = w + 8.0                                                                       # into the houses on both sides

    def at(u, x=0.0):
        return p[0] + v[0] * u + n[0] * x, p[1] + v[1] * u + n[1] * x

    bx, bz = at(0.0)                                                                     # A: the barricade, u -0.4 .. 0.4
    prism(rect(bx, bz, 0.8, span, ang), y - 1.0, y + 5.0, "wood", "A")
    for k, x in enumerate((-2.4, -0.8, 0.8, 2.4)):                                      # its posts, a little proud of it
        px_, pz_ = at(-0.5, x)
        prism(rect(px_, pz_, 0.25, 0.3, ang), y - 1.0, y + 5.4 + 0.2 * (k % 2), "wood", "A")
    for u, top in ((-3.0, 1.2), (-1.6, 2.4)):                                            # A: the crates, steps of 1.2 m
        cx, cz = at(u, 0.6)
        prism(rect(cx, cz, 1.2, 1.4, ang), y - 0.5, y + top, "wood", "A", True, "cloth2" if top > 2.0 else "wood")
    for u0, u1, top in ((-0.4, 1.6, 3.4), (1.6, 2.8, 2.3), (2.8, 4.0, 1.2)):           # B: the fallen front and its steps
        cx, cz = at((u0 + u1) / 2)
        prism(rect(cx, cz, u1 - u0, span, ang), y - 1.0, y + top, "debris", "B", True, "stone_dark")
    cx, cz = at(0.6, 16.0)                                                               # B: the garden walls behind fell
    prism(rect(cx, cz, 2.0, 20.0, ang), y - 4.0, y + 3.4, "debris", "B", True, "stone_dark")  # with it (the way round below)
    bx, bz = at(0.8, -1.5)                                                               # B: a beam of the house on the heap
    beam((bx, y + 3.75, bz), (bx + v[0] * 0.3 + n[0] * 3.0, y + 3.65, bz + v[1] * 0.3 + n[1] * 3.0), 0.35, 0.3, "wood", "B")


class Piece:
    """A puzzle laid across a street at `s` metres along it: local coordinates u (along the street, toward its end) and x
    (across it, to the left), heights over `base` (the street at s). Boxes are prisms; `cross` spans the street and the
    houses beside it (the width of the street + 2 * `side`)."""

    def __init__(self, street, s, side=5.0):
        self.p, self.base, self.v, self.n, self.w = street_frame(street, s)
        self.street = street
        self.s = s
        self.ang = math.atan2(self.v[1], self.v[0])
        self.span = self.w + 2.0 * side

    def clear(self, u0, u1):
        """No house in the piece from u0 to u1 (and its width)."""
        u = u0
        while u <= u1:
            KEEP_CLEAR.append(self.xz(u) + (self.span / 2 + 1.5,))
            u += 2.0

    def xz(self, u, x=0.0):
        return (float(self.p[0] + self.v[0] * u + self.n[0] * x), float(self.p[1] + self.v[1] * u + self.n[1] * x))

    def y(self, u):
        """The street under the point u along it (its own height, not the base)."""
        return street_y_at(self.street, self.s + u)

    def box(self, u0, u1, x0, x1, top, mat, world="both", bottom=None, top_mat=None):
        cx, cz = self.xz((u0 + u1) / 2, (x0 + x1) / 2)
        y0 = self.base + (bottom if bottom is not None else -4.0)
        prism(rect(cx, cz, u1 - u0, x1 - x0, self.ang), y0, self.base + top, mat, world, True, top_mat)

    def cross(self, u0, u1, top, mat, world="both", bottom=None, top_mat=None):
        self.box(u0, u1, -self.span / 2, self.span / 2, top, mat, world, bottom, top_mat)

    def sides(self, u0, u1, top, mat="stone"):
        """Walls along both sides of the piece: no way round it through the houses."""
        for x in (-self.span / 2 - 0.4, self.span / 2 + 0.4):
            self.box(u0, u1, x - 0.4, x + 0.4, top, mat)

    def thing_box(self, state, world, mat, u0, u1, x0, x1, b0, b1):
        cx, cz = self.xz((u0 + u1) / 2, (x0 + x1) / 2)
        return (state, world, mat, (cx, self.base + (b0 + b1) / 2, cz), ((u1 - u0) / 2, (b1 - b0) / 2, (x1 - x0) / 2), self.ang)

    def handle(self, u, x, h):
        cx, cz = self.xz(u, x)
        return (cx, self.base + h + 1.1, cz)


def street_y_at(name, s):
    c, _, _ = STREET[name]
    d = lengths(c)
    s = float(np.clip(s, 0.0, d[-1]))
    i = int(np.clip(np.searchsorted(d, s) - 1, 0, len(c) - 2))
    f = (s - d[i]) / max(d[i + 1] - d[i], 1e-9)
    return float(_HEIGHTS[name][i] + f * (_HEIGHTS[name][i + 1] - _HEIGHTS[name][i]))


def corridor(street, s0, s1, h=9.5, gaps=()):
    """Walls along both sides of a street from s0 to s1 metres (garden walls and house fronts, joined): the street is the
    only way along it; `h` over the street, higher than any step of a puzzle on it plus a jump (no walk along their tops).
    `gaps`: (s, side, width) openings where another way meets it (side +1 left, -1 right, 0 both)."""
    c, w, _ = STREET[street]
    d = lengths(c)
    s = s0
    step = 1.0
    while s < s1 - 1e-6:
        e = min(s + step, s1)
        for side in (-1, 1):
            if any(abs((s + e) / 2 - gs) < gw / 2 and gside in (0, side) for gs, gside, gw in gaps):
                continue
            pa, va = at(c, s / d[-1])
            pb, vb = at(c, e / d[-1])
            na = np.array([-va[1], va[0]]) * side
            nb = np.array([-vb[1], vb[0]]) * side
            o0, o1 = w / 2 + 0.5, w / 2 + 1.1
            pts = [pa + na * o0, pb + nb * o0, pb + nb * o1, pa + na * o1]
            ys = [street_y_at(street, s), street_y_at(street, e)] + [float(ground(q[0], q[1])) for q in pts]
            prism([(q[0], q[1]) for q in pts], min(ys) - 2.0, max(ys) + h, "plaster1" if int(s / 6) % 2 else "stone", "both", True, "stone")
        s = e


def district_wall(points, h=4.5, mat="stone"):
    """A retaining wall between two terraces of the town (and the garden walls on it), along `points`: 4.5 m over the
    highest ground within 3 m of it, so it is never a step. It closes a district: the puzzle in it is the only way on."""
    line = spline(points, 1.0) if len(points) > 2 else np.array(points, float)
    if len(points) == 2:
        a, b = np.array(points[0], float), np.array(points[1], float)
        n = max(int(np.linalg.norm(b - a) / 1.0), 1)
        line = np.array([a + (b - a) * t for t in np.linspace(0.0, 1.0, n + 1)])
    for i in range(len(line) - 1):
        a, b = line[i], line[i + 1]
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 0.6, v[0] * 0.6
        pts = [(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)]
        near = [float(ground(q[0] + o * nx / 0.6, q[1] + o * nz / 0.6)) for q in (a, b) for o in (-3.0, 0.0, 3.0)]
        prism(pts, min(near) - 3.0, max(near) + h, mat, "both", True, "stone")
        KEEP_CLEAR.append((float(a[0]), float(a[1]), 1.5))


CANAL_S = 6.0     # metres along the market stairs: the customs canal (harbour, beat 4)
SCAFFOLD_S = 14.0  # metres along the market street: the terrace of the market and its scaffold (west)
PLANK_S = 14.0    # metres along the upper street: two houses and the plank (east)
SCAFFOLD_SPOT = None  # the middle platform of the scaffold: where the lantern is first set down (set by build_scaffold)
GATEHOUSE_S = 66.0  # metres along the upper street: the gatehouse of the theatre (north)


def build_canal():
    """Harbour, beat 4: the causal rule with a crate. The customs canal crosses the foot of the market stairs between two
    quays: the near quay (2.4 m, steps up to it), 6.6 m of canal, the far quay (2.4 m over the bed; steps down beyond).
    A: the canal holds water (the body sinks); a crate of the customs stands on the near quay: pushed (E), it goes into the
    canal and floats to the far quay. B: the canal is dry; its bed is reached down a fallen piece of the near quay, but
    the far quay is too high from it; a crate pushed in A lies on the bed against the far quay: a step."""
    k = Piece("market stairs", CANAL_S)
    k.clear(-1.0, 17.0)
    district_wall([k.xz(6.0, k.span / 2 + 0.8), (-82.0, -29.0)])                        # the terrace of the west, both ways
    district_wall([k.xz(6.0, -k.span / 2 - 0.8), (-24.0, -20.0), (-19.6, -16.4)])       # on to the parapet of the gorge
    b = k.base
    k.sides(-1.0, 17.0, 7.0)
    k.cross(0.0, 1.2, 1.2, "stone")                                                     # the steps up to the near quay
    k.cross(1.2, 2.4, 2.4, "stone")
    k.cross(2.4, 4.4, 2.4, "stone", top_mat="paving")                                   # the near quay
    bed = k.y(11.0) - b                                                                 # the bed: the street (it rises)
    k.cross(11.0, 13.0, bed + 2.4, "stone", top_mat="paving")                           # the far quay
    k.cross(13.0, 14.2, bed + 1.4, "stone")                                             # its steps down
    for i, x in enumerate((-k.span / 2 + 0.8, -k.span / 2 + 2.0)):                     # B: the fallen near quay, a way down
        k.box(4.4, 5.6 + 1.2 * i, x - 0.6, x + 0.6, 1.2 * (2 - i) - 0.1, "debris", "B")
    k.box(4.4, 5.6, -k.span / 2 + 0.2, -k.span / 2 + 2.6, 2.3, "debris", "B")
    for u0, u1, x0, x1 in ((4.4, 11.0, -k.span / 2, k.span / 2),):                      # A: the water of the canal
        corners = [k.xz(u0, x0), k.xz(u1, x0), k.xz(u1, x1), k.xz(u0, x1)]
        poly([(x, b + 2.2, z) for x, z in corners], "water", "A", "water")
    bed_hi = bed + 1.2
    thing("canal crate", k.handle(3.2, -1.3, 2.4), [
        k.thing_box("shut", "A", "wood", 2.6, 3.8, -0.6, 0.6, 2.4, 3.6),
        k.thing_box("open", "A", "wood", 9.8, 11.0, -0.6, 0.6, bed, bed_hi),
        k.thing_box("open", "B", "wood", 9.8, 11.0, -0.6, 0.6, bed, bed_hi),
    ], "push")


def build_scaffold():
    """West: the lantern. The market street meets the terrace of the market hall, 6.0 m over the street, in both moments.
    A: a scaffold of three timber platforms (1.4, 2.9, 4.4 m) climbs by the terrace wall, but the roof of a lean-to
    shed covers its top and the edge of the terrace: no jump from it. B: shed and scaffold long gone. The answer: set the
    lantern down on the scaffold in A, below the roof (its circle reaches the top platform, not the roof, which is more
    than 3.5 m over it); in B climb the scaffold of the circle and jump onto the terrace."""
    global SCAFFOLD_SPOT
    k = Piece("market street", SCAFFOLD_S)
    k.clear(-6.0, 9.0)
    yb = k.y(-2.0) - k.base                                                             # the street at the scaffold
    sx_, sz_ = k.xz(-2.0, 0.9)                                                          # the middle platform (2.9 m): the spot
    SCAFFOLD_SPOT = (sx_, k.y(-2.0) + 2.9, sz_)
    k.sides(-6.0, 9.0, yb + 13.0)
    k.cross(0.0, 6.0, yb + 6.0, "stone", top_mat="paving")                              # the terrace
    k.box(-2.6, -1.4, -1.6, -0.2, yb + 1.4, "wood", "A", top_mat="wood")                # the scaffold (A): steps of 1.5 m
    k.box(-2.6, -1.4, 0.2, 1.6, yb + 2.9, "wood", "A", top_mat="wood")
    k.box(-1.2, 0.0, 0.2, 1.6, yb + 4.4, "wood", "A", top_mat="wood")
    k.box(-1.6, 2.2, -k.span / 2, k.span / 2, yb + 7.8, "roof", "A", bottom=yb + 7.4)  # the roof of the shed (A)
    k.box(-1.6, -1.3, -k.span / 2, -k.span / 2 + 0.3, yb + 7.4, "wood", "A")          # its posts
    k.box(-1.6, -1.3, k.span / 2 - 0.3, k.span / 2, yb + 7.4, "wood", "A")


def build_plank():
    """East: the causal rule with a plank. The upper street climbs by four steps to the roof of a house (4.8 m) and its
    neighbour is 5 m away across a yard (whose ground rises to the sides: the roofs stand 2.5 m or more over it); the
    street goes on beyond the neighbour. A: a plank lies on the first roof (E lays it across); the neighbour's roof has a
    parapet with a shut door. B: no parapet; the plank laid in A is still there, grey, and holds."""
    k = Piece("upper street", PLANK_S)
    k.clear(-1.0, 17.0)
    k.sides(-1.0, 17.0, 10.0)
    for i in range(3):
        k.cross(1.2 * i, 1.2 * (i + 1), 1.2 * (i + 1), "stone")
    hi = 4.8
    k.cross(3.6, 5.4, hi, "plaster1", top_mat="roof")                                   # the first house
    top2 = k.y(10.4) - k.base + hi
    k.cross(10.4, 13.0, top2, "plaster2", top_mat="roof")                               # the neighbour
    for i in range(1, 4):
        k.cross(13.0 + 1.2 * (i - 1), 13.0 + 1.2 * i, top2 - 1.2 * i, "stone")
    k.box(11.4, 11.7, -k.span / 2, k.span / 2, top2 + 3.0, "plaster2", "A", bottom=top2)  # the parapet (A)
    thing("plank", k.handle(4.6, 1.6, hi), [
        k.thing_box("shut", "A", "wood", 3.8, 5.2, -1.4, -0.8, hi, hi + 0.15),
        k.thing_box("open", "A", "wood", 5.2, 10.6, -0.4, 0.4, hi - 0.15, hi),
        k.thing_box("open", "B", "wood", 5.2, 10.6, -0.4, 0.4, hi - 0.15, hi),
    ], "lay")


def build_gatehouse():
    """North: everything at once. The gatehouse of the theatre closes the upper street in A (6 m, 4 m deep); a wooden
    balcony hangs on its front at 2.4 m. In B it lies across the street as a heap of 4 m. A crate stands 7 m before it in A.
    The answer: push the crate under the balcony in A (it stays in B too, but 1.2 m does not reach a heap of 4 m); from
    the crate onto the balcony; jump, switch at the top: the heap of B is under the feet."""
    k = Piece("upper street", GATEHOUSE_S)
    k.clear(-9.0, 9.0)
    k.sides(-9.0, 9.0, 10.0)
    k.cross(0.0, 4.0, 6.0, "plaster1", "A", top_mat="roof")                             # the gatehouse (A)
    k.box(-1.2, 0.0, -1.2, 1.2, 2.4, "wood", "A", bottom=2.1)                           # its balcony (A): 1.5 m over the crate
    k.cross(-0.5, 4.0, 4.0, "debris", "B", top_mat="stone_dark")                        # the heap (B)
    k.cross(4.0, 5.2, k.y(4.6) - k.base + 2.8, "debris", "B")
    k.cross(5.2, 6.4, k.y(5.8) - k.base + 1.6, "debris", "B")
    yc = k.y(-7.0) - k.base
    thing("gatehouse crate", k.handle(-7.0, -1.3, yc), [
        k.thing_box("shut", "A", "wood", -7.6, -6.4, -0.6, 0.6, yc, yc + 1.2),
        k.thing_box("open", "A", "wood", -2.6, -1.4, -0.6, 0.6, k.y(-2.0) - k.base, k.y(-2.0) - k.base + 1.2),
        k.thing_box("open", "B", "wood", -2.6, -1.4, -0.6, 0.6, k.y(-2.0) - k.base, k.y(-2.0) - k.base + 1.2),
    ], "push")


def build_district_walls():
    """The walls between the districts (garden walls, terraces and the backs of houses joined, 4.5 m over the ground beside
    them): each district is left only through its puzzle (docs/DESIGN.md 7.3). The south-east: the lane of the lower town
    ends below the locks. The east: the upper town is reached over the plank, not round it. The north: the theatre and the
    cliff stair are reached through the gatehouse, not round it. The west: the bastion is reached from the terrace of the
    market (the lantern), not across the gardens north of the market street."""
    def rim(deg):
        a = math.radians(deg)
        return (GORGE_R + 1.4) * math.cos(a), (GORGE_R + 1.4) * math.sin(a)
    district_wall([rim(-50.0), (35.0, -27.0), (55.0, -32.0), (79.0, -31.0)])           # the lane / the locks
    k = Piece("market street", 24.0)                                                    # the market: the bastion is reached
    district_wall([k.xz(0.0, k.w / 2 + 0.8), rim(157.0)])                               # along the street from the terrace
    k = Piece("upper street", PLANK_S + 6.0)
    district_wall([k.xz(0.0, -k.span / 2 - 0.8), (73.5, 51.5)])                          # the plank: north to the town wall
    district_wall([k.xz(0.0, k.span / 2 + 0.8), (42.0, 18.0), rim(40.0)])                # and south to the gorge
    k = Piece("upper street", GATEHOUSE_S)
    district_wall([k.xz(0.0, -k.span / 2 - 0.8), (20.8, 66.0)])                          # the gatehouse: north
    district_wall([k.xz(0.0, k.span / 2 + 0.8), (20.0, 36.0), rim(72.0)])                # and south, east of the theatre


BELL_ROPES = {  # the bells (docs/DESIGN.md 4): rung in A at the foot of their towers
    "day bell": (-38.0, 14.0, 2.25 + 0.9),
    "dawn bell": (84.0, 26.0, 3.0 + 0.9),
    "night bell": (16.0, GRAVE_Z0 + 10.0, 2.0 + 0.9),
}


def build_bells():
    """A bell is rung at its rope, in A (the bells of B are fallen or cracked); a rung bell lights its tower in B."""
    for name, (tx, tz, r) in BELL_ROPES.items():
        best = None
        for k in range(16):                                                              # the side of the tower on a street
            a = 2 * math.pi * k / 16
            x, z = tx + r * math.cos(a), tz + r * math.sin(a)
            d = min(distance_to(c, x, z)[0] for c, _, _ in STREET.values())
            if best is None or d < best[0]:
                best = (d, x, z, a)
        _, x, z, a = best
        y = float(ground(x, z))
        thing(name, (x, y + 1.1, z), [
            ("open", "B", "lantern", (x, y + 2.9, z), (0.22, 0.25, 0.22), 0.0),          # a lamp lit over the rope (B)
        ], "ring")


# ---- the signal mast: the far goal, seen from the first frame (docs/DESIGN.md 10.1) ----------------------------------------------
# The sea wall, 8 m in front of the start, hides the bell tower from the quay in both moments (tests/test_firstminute.cpp casts
# the rays). So the three emblems hang on a signal mast that stands over the tower and clears the wall: three lamps on a yard,
# dark until the bell of their district is rung. Round lamps, so that they read from every side; each has its own shape.
MAST_XZ = (0.0, 5.7)       # in the south face of the shaft of the bell tower (its face is at z 5.36): above the stone it stands free
MAST_BASE_Y = 26.0         # it comes out of the dome of the church
LAMP_Z = 3.4               # the lamps hang in front of the mast, on a yard (not on its axis: the mast would hide the middle one)
MAST_TOP_Y = 66.6
YARD_Y = 64.4
LAMP_Y = 61.2
LAMP_DX = 4.8
LAMP_SIZE = 3.0
EMBLEMS = []               # (glyph, bell, x, y, z, size, shape: 0 sphere, 1 cylinder, 2 cube)
OVERLOOKS = [              # (name, x, y, z, radius): standing here reveals the map this far round (docs/DESIGN.md 10.4)
    ("west bastion", -27.0, ROCK_Y, 0.0, 58.0),
    ("the plaza of the rock", 0.0, ROCK_Y, -8.0, 40.0),
    ("east bastion", 27.0, ROCK_Y, 0.0, 58.0),
    ("top row of the theatre", 0.0, THEATRE_Y + 0.9 * 9, 52.0, 52.0),
    ("the graveyard", 0.0, GRAVE_Y, 88.0, 80.0),
]
def build_signal_mast():
    mx, mz = MAST_XZ
    prism(rect(mx, mz, 0.5, 0.5, 0.0), MAST_BASE_Y, MAST_TOP_Y, "iron", "both", True, "iron")
    beam((mx - 6.6, YARD_Y, LAMP_Z), (mx + 6.6, YARD_Y, LAMP_Z), 0.36, 0.36, "iron")         # the yard, in front of the mast
    beam((mx, YARD_Y, mz), (mx, YARD_Y, LAMP_Z), 0.36, 0.36, "iron")                          # its arm back to the mast
    for dx in (-LAMP_DX, 0.0, LAMP_DX):                                                        # what the lamps hang by
        prism(rect(mx + dx, LAMP_Z, 0.14, 0.14, 0.0), LAMP_Y + LAMP_SIZE * 0.5, YARD_Y - 0.18, "iron")
    EMBLEMS.extend([("well", "day bell", mx - LAMP_DX, LAMP_Y, LAMP_Z, LAMP_SIZE, 0),         # west: the well, a sphere
                    ("cliff", "night bell", mx, LAMP_Y, LAMP_Z, LAMP_SIZE, 1),                 # north: the cliff, a column
                    ("lock", "dawn bell", mx + LAMP_DX, LAMP_Y, LAMP_Z, LAMP_SIZE, 2)])        # east: the lock, a box
# ---- the lamps (docs/DESIGN.md 6.3): every lantern of the model is also a light of the game -----------------------------------------
# (x, y, z, range, colour (sRGB), intensity, flicker, world "A" / "B" / "both", kind). The game lights the town with the nearest ones.
LAMPS = []
LAMP_KIND = {"street": 0, "post": 1, "light": 2, "memory": 3, "grave": 4}
LAMP_LOOK = {   # kind: (range m, colour, intensity, flicker). Intensity 18 lights the ground under a lamp 3.4 m up with 1.4.
    "street": (14.0, (1.00, 0.80, 0.55), 18.0, 0.06),
    "post": (15.0, (1.00, 0.82, 0.58), 20.0, 0.05),
    "light": (38.0, (1.00, 0.88, 0.65), 140.0, 0.03),
    "memory": (9.0, (0.62, 0.92, 1.00), 12.0, 0.08),
    "grave": (11.0, (1.00, 0.78, 0.50), 16.0, 0.10),
}
def lamp(x, y, z, world, kind="street"):
    """A light at (x, y, z) in the moments of `world`, of the look of its kind. Called where the model builds a lantern."""
    rng, col, inten, flick = LAMP_LOOK[kind]
    LAMPS.append((float(x), float(y), float(z), rng, col, inten, flick, world, kind))
STEPS = []  # (name, kind "thing"/"lantern", (x, y, z)): the main path in the order it is played
STEP_ORDER = ("sea gate", "canal crate", "day bell", "lantern", "lift", "dawn bell", "plank", "gatehouse crate", "night bell")
STEP_KIND = {"thing": 0, "lantern": 1}
def build_steps():
    """The main path as data (docs/DESIGN.md 7.4, 8): each thing at its handle, and the lantern on the scaffold of the west, in
    the order they are played. The game guides from it and the route test plays it."""
    handles = {name: handle for name, handle, _, _ in THINGS}
    for name in STEP_ORDER:
        if name == "lantern":
            STEPS.append((name, "lantern", SCAFFOLD_SPOT))
        else:
            STEPS.append((name, "thing", handles[name]))
def thing_polys(state="shut"):
    """The boxes of the things in one state as polygons (for the pictures of the model; the game draws its own)."""
    out = []
    for _, _, boxes, _ in THINGS:
        for st, world, mat, (cx, cy, cz), (hx, hy, hz), yaw in boxes:
            if st != state:
                continue
            outline = rect(cx, cz, 2 * hx, 2 * hz, yaw)
            y0, y1 = cy - hy, cy + hy
            out.append(([(x, y1, z) for x, z in outline], mat, world, "solid"))
            for i in range(4):
                (ax, az), (bx, bz) = outline[i], outline[(i + 1) % 4]
                out.append(([(ax, y0, az), (bx, y0, bz), (bx, y1, bz), (ax, y1, az)], mat, world, "solid"))
    return out


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


KEEP_CLEAR = []  # (x, z, r): the places of the puzzles; no house stands in them (build_houses comes after them)


def site_free(out):
    if any(math.hypot(px_ - kx, pz_ - kz) < kr for px_, pz_ in out for kx, kz, kr in KEEP_CLEAR):
        return False
    if any(math.hypot(sum(q[0] for q in out) / 4 - kx, sum(q[1] for q in out) / 4 - kz) < kr for kx, kz, kr in KEEP_CLEAR):
        return False
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
        if any(math.hypot(x - gx, z - gz) < 6.0 for gx, gz in GATES.values()):
            return False
    if distance_to(LOCK_LINE, xs, zs)[0].min() < 9.0 or distance_to(WALL, xs, zs)[0].min() < 2.4:
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
    # Street-facing eaves give the dense blocks a readable roofline without changing collision.
    if front is not None and not ruin:
        fv = np.array(front, float); fv /= max(np.linalg.norm(fv), 1e-9)
        rv = np.array([fv[1], -fv[0]])
        face_extent = w if abs(np.dot(rv, np.array([math.cos(ang), math.sin(ang)]))) > 0.7 else dd
        depth_extent = dd if face_extent == w else w
        ec = np.array([cx, cz]) + fv * (depth_extent / 2 + 0.26)
        a = ec - rv * (face_extent / 2 + 0.28); b = ec + rv * (face_extent / 2 + 0.28)
        beam((float(a[0]), top - 0.12, float(a[1])), (float(b[0]), top - 0.12, float(b[1])), 0.24, 0.11, roofmat,
             "A" if unroofed else "both")
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
        c, s = math.cos(ang), math.sin(ang)
        grade = max(base, float(ground(cx + fx * dd / 2, cz + fz * dd / 2)))
        out_n = dd / 2 + 0.04
        dx_, dz_ = cx + fx * out_n, cz + fz * out_n
        # Facade sizes are in metres for a 1.72 m traveller.
        if grade + 2.08 < base + height:
            poly([(dx_ - c * 0.46, grade, dz_ - s * 0.46), (dx_ + c * 0.46, grade, dz_ + s * 0.46), (dx_ + c * 0.46, grade + 2.08, dz_ + s * 0.46),
                  (dx_ - c * 0.46, grade + 2.08, dz_ - s * 0.46)], "door", "A" if ruin else "both")
            # Stone jambs frame a closed door; trim must never add a ledge.
            def trim(u0, u1, y0, y1, mat):
                x, z = dx_ + fx * 0.012, dz_ + fz * 0.012
                poly([(x + c * u, y, z + s * u) for u, y in
                      ((u0, y0), (u1, y0), (u1, y1), (u0, y1))],
                     mat, "A" if ruin else "both", "overlay")

            for lo, hi in ((-0.56, -0.46), (0.46, 0.56)):
                trim(lo, hi, grade + 0.02, grade + 2.08, "stone_dark")
            trim(-0.56, 0.56, grade + 2.08, min(grade + 2.20, top), "stone")
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
                y = grade + (3.10 if height > 6.0 else 1.15)
                if y + 0.81 >= base + height:
                    continue
                poly([(px - c * 0.4, y, pz - s * 0.4), (px + c * 0.4, y, pz + s * 0.4), (px + c * 0.4, y + 0.81, pz + s * 0.4),
                      (px - c * 0.4, y + 0.81, pz - s * 0.4)], "window" if hash01(k, 71 + j) < 0.40 else "stone_dark", "A")
                sill_x, sill_z = px + fx * 0.016, pz + fz * 0.016
                poly([(sill_x + c * u, v, sill_z + s * u) for u, v in
                      ((-0.48, y - 0.09), (0.48, y - 0.09), (0.48, y), (-0.48, y))],
                     "stone", "A" if ruin else "both", "overlay")
                for sg in (-1, 1):                                                     # shutters
                    q0 = (px + sg * c * 0.45, pz + sg * s * 0.45)
                    q1 = (px + sg * c * 0.85, pz + sg * s * 0.85)
                    poly([(q0[0], y, q0[1]), (q1[0], y, q1[1]), (q1[0], y + 0.81, q1[1]), (q0[0], y + 0.81, q0[1])], "shutter",
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
            if y < SEA + 1.5 or (abs(x) < 26 and GRAVE_Z0 - 4.0 < z < GRAVE_Z0 + 20.0):
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
MAIN_PATH = [  # (label, x, z, how the next leg goes: "walk", "under" (in the cistern), "to design")
    ("1 start: the quay", 6, -72, "walk"), ("sea gate", 0, -62, "walk"), ("lower town", -28, -47, "walk"),
    ("market stairs", -58, -24, "walk"), ("the market", -46, -7, "walk"), ("2 west bastion: the bridge seen, its gate shut", -29, 0, "walk"),
    ("3 the wreck of the west bridge (B): down", -19, 4, "walk"), ("4 the gorge floor (B)", -14, 14, "walk"),
    ("5 the lift: lowered in A, its frame climbed in B", -5.5, -19, "walk"), ("6 the plaza", 0, -8, "walk"),
    ("east bridge", 18, 0, "walk"), ("east stair", 38, -8, "walk"), ("the locks", 52, -10, "walk"),
    ("water tower: dawn bell", 80, 24, "walk"), ("upper street", 34, 52, "walk"), ("the theatre", 2, 56, "walk"),
    ("cliff stair", 8, 68, "walk"), ("7 graveyard: night bell", 0, 86, "to design"), ("the bell tower: the end", 0, 8.5, ""),
]


def check():
    """The numbers of the layout against the limits of the game (docs/DESIGN.md 11). Returns the list of problems."""
    rows = []
    for name, (c, w, kind) in STREET.items():
        h = _HEIGHTS[name]
        ds = np.maximum(np.linalg.norm(np.diff(c, axis=0), axis=1), 1e-6)
        worst = float((np.abs(np.diff(h)) / ds).max())
        ok = worst <= SLOPE[kind] + 1e-3
        rows.append("  %-16s %-6s %6.1f -> %6.1f m  steepest %4.1f deg (limit %2.0f) %s" % (
            name, kind, h[0], h[-1], math.degrees(math.atan(worst)), math.degrees(math.atan(SLOPE[kind])), "" if ok else "TOO STEEP"))
        if not ok:
            PROBLEMS.append((name, "steepest %.0f degrees" % math.degrees(math.atan(worst))))
    joints = [("market street at the west bastion", _HEIGHTS["market street"][-1], ROCK_Y),
              ("east stair at the east bastion", _HEIGHTS["east stair"][0], ROCK_Y),
              ("east stair at the first quay", _HEIGHTS["east stair"][-1], float(quay_level(0.0))),
              ("upper street at the last quay", _HEIGHTS["upper street"][0], LOCK_LEVELS[-1]),
              ("cliff stair at the graveyard", _HEIGHTS["cliff stair"][-1], GRAVE_Y),
              ("upper street at the top row of the theatre", _HEIGHTS["upper street"][-1], THEATRE_Y + 0.9 * 9),
              ("sea stair at the quay", _HEIGHTS["sea stair"][0], QUAY_Y),
              ("market stairs at the market", _HEIGHTS["market stairs"][-1], MARKET_Y)]
    for name, a, b in joints:
        ok = abs(a - b) <= 0.3
        rows.append("  %-36s %6.1f against %6.1f %s" % (name, a, b, "" if ok else "STEP %.1f m" % abs(a - b)))
        if not ok:
            PROBLEMS.append((name, "a step of %.1f m" % abs(a - b)))
    total = lengths(LOCK_LINE)[-1]
    sl = (LOCK_LEVELS[1] - LOCK_LEVELS[0]) / LOCK_HEAD_RUN
    rows.append("  lock stairs: %.1f m over %.1f m, %.0f degrees (limit 32); lock line %.0f m" % (
        LOCK_LEVELS[1] - LOCK_LEVELS[0], LOCK_HEAD_RUN, math.degrees(math.atan(sl)), total))
    if sl > SLOPE["stair"]:
        PROBLEMS.append(("lock stairs", "too steep"))
    rows.append("  the gorge: %.0f m from the plaza down to its floor; the bridges %.1f m long" % (ROCK_Y - GORGE_Y, abs(BRIDGEHEADS["west"][0]) - 1.5 - ROCK_R + 0.2))
    rows.append("  gates in the town wall for: %s" % (", ".join(GATES) if GATES else "none"))
    print("check:")
    print("\n".join(rows))
    if PROBLEMS:
        print("problems:")
        for place, what in PROBLEMS:
            print("  %s: %s" % (place, what))
    else:
        print("no problems")
    return PROBLEMS


# ---- the renderer -----------------------------------------------------------------------------------------------------------

def colour(mat, world):
    a, b = MAT[mat]
    return np.array(a if world == "A" else b, float)


def render(world, path, eye, target, fov, size, title, overlay=None, ortho=None, lantern=None, *, vertical_fov=False):
    """`lantern` (x, y, z, r), in B: the lantern set down in A; inside its circle what stands is A, lit by A (docs/DESIGN.md
    3.2)."""
    to_light, light, amb, sky_top, horizon, fog = LIGHT[world]
    La, light_a, amb_a = np.array(LIGHT["A"][0], float), LIGHT["A"][1], LIGHT["A"][2]
    La /= np.linalg.norm(La)
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
    focal = ((H if vertical_fov else W) / 2) / math.tan(math.radians(fov) / 2) if ortho is None else None
    polys, cols, depths, edges, widths = [], [], [], [], []
    for pts, mat, w, kind in POLYS:
        pts = np.asarray(pts, dtype=float)
        inside = False
        if lantern is not None and world == "B":
            cx_, cy_, cz_ = pts.mean(axis=0)
            inside = math.hypot(cx_ - lantern[0], cz_ - lantern[2]) <= lantern[3] and abs(cy_ - lantern[1]) < 8.0 and kind != "sea"
        shown = "A" if inside else world
        if w not in ("both", shown):
            continue
        if mat in ("water",) and shown == "B" and kind == "water":
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
        alb = colour(mat, shown)
        emissive = mat in EMISSIVE and EMISSIVE[mat][0 if shown == "A" else 1]
        if emissive:
            c = alb * 1.15
        else:
            Lw, lw, aw = (La, light_a, amb_a) if inside else (L, light, amb)
            ndl = max(0.0, float(nrm @ Lw))
            hemi = 0.65 + 0.35 * nrm[1]
            c = alb * (np.array(aw) * hemi * 1.15 + np.array(lw) * ndl * 1.05)
            if kind in ("water", "sea"):                                               # the sky in the water
                c = 0.85 * alb * (0.7 + 0.3 * np.array(light)) + 0.15 * np.array(horizon)
        dist = np.linalg.norm(centre - eye)
        fo = 1.0 - math.exp(-dist * fog * (0.6 if kind == "sea" else 1.0))
        if emissive or inside:
            fo *= 0.5
        c = c * (1 - fo) + np.array(LIGHT["A"][4] if inside else horizon) * fo
        c = np.clip(c, 0, 1)
        depth = cam[:, 2].mean()
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
    if lantern is not None and world == "B" and ortho is None:                          # the rim of the circle, and the lantern
        ring = [np.array([lantern[0] + lantern[3] * math.cos(a), lantern[1] + 0.1, lantern[2] + lantern[3] * math.sin(a)]) for a in np.linspace(0, 2 * math.pi, 64)]
        scr = []
        for p in ring:
            rel = p - eye
            zc = rel @ f
            if zc > 0.6:
                scr.append((W / 2 + focal * (rel @ right) / zc, H / 2 - focal * (rel @ up) / zc))
        if scr:
            xs_, ys_ = zip(*scr)
            ax.plot(xs_, ys_, color=(1.0, 0.78, 0.42), lw=3.0, alpha=0.85, zorder=2)
        rel = np.array([lantern[0], lantern[1] + 0.4, lantern[2]]) - eye
        zc = rel @ f
        if zc > 0.6:
            ax.plot(W / 2 + focal * (rel @ right) / zc, H / 2 - focal * (rel @ up) / zc, "o", ms=11, color=(1.0, 0.86, 0.5), mec=(0.5, 0.3, 0.1), zorder=3)
    if overlay is not None:
        overlay(ax, eye, right, up, f, focal, W, H, ortho)
    ax.set_xlim(0, W)
    ax.set_ylim(H, 0)
    ax.set_axis_off()
    ax.text(14, 26, title, fontsize=12, color="white" if world == "B" else (0.25, 0.12, 0.10), weight="bold")
    fig.savefig(path, dpi=100)
    plt.close(fig)
    return len(polys)


HEIGHT_OF = {"2": ROCK_Y, "4": GORGE_Y, "5": GORGE_Y, "6": ROCK_Y, "east bridge": ROCK_Y, "the bell tower: the end": ROCK_Y + 40.0}


def plan_overlay(ax, eye, right, up, f, focal, W, H, ortho):
    def project(x, y, z):
        rel = np.array([x, y, z]) - eye
        return W / 2 + ortho * (rel @ right), H / 2 - ortho * (rel @ up)

    pts = [project(x, float(ground(x, z)) + 1.0, z) for _, x, z, _ in MAIN_PATH]
    for k in range(len(MAIN_PATH) - 1):
        how = MAIN_PATH[k][3]
        (ax0, ay0), (ax1, ay1) = pts[k], pts[k + 1]
        style = {"walk": ("-", (0.75, 0.10, 0.15)), "under": ("--", (0.10, 0.35, 0.80)), "to design": (":", (0.35, 0.35, 0.35))}[how]
        ax.plot([ax0, ax1], [ay0, ay1], ls=style[0], color=style[1], lw=3.0, zorder=3)
    offsets = {"2": (-8, -16, "right"), "3": (-8, -10, "right"), "the market": (-8, 4, "right"), "4": (8, 22, "left"),
               "5": (8, 22, "left"), "6": (-8, -4, "right"), "east bridge": (8, -10, "left"), "east stair": (8, 10, "left")}
    for (name, x, z, _), (px, py) in zip(MAIN_PATH, pts):
        ax.plot(px, py, "o", color=(0.75, 0.10, 0.15), ms=7, zorder=4)
        key = name.split(" ")[0] if name[0].isdigit() else name
        dx, dy, ha = offsets.get(key, (8, -6, "left"))
        ax.text(px + dx, py + dy, "%s (%.0f m)" % (name, HEIGHT_OF.get(key, float(ground(x, z)))), fontsize=8.5, weight="bold", zorder=5, ha=ha,
                bbox=dict(boxstyle="round,pad=0.15", fc="white", alpha=0.8, ec="none"))


# ---- the town as data for the game (src/scene/Town.h reads it) -------------------------------------------------------------
TOWN_MAGIC = b"DLTOWN\0\0"
# ---- the plan of both moments, for the map (docs/DESIGN.md 10.4): a raster of 1 m cells, one class per cell and moment ---------------
PLAN_CELL = 1.0
PLAN_BOX = (-98.0, -100.0, 102.0, 102.0)  # x0, z0, x1, z1: the town, its harbour and its cliff
PLAN_WATER, PLAN_CLIFF, PLAN_BUILDING, PLAN_STREET = 1, 2, 3, 4
PLAN_PRIORITY = {0: 0, PLAN_WATER: 1, PLAN_CLIFF: 2, PLAN_BUILDING: 3, PLAN_STREET: 4}  # a street is drawn over an arch
PLAN_SKIP = {"window", "lantern", "memory", "glow", "door", "shutter", "tree", "cypress", "moss", "cloth", "cloth2", "iron"}
def build_plan():
    """What stands in each cell of the plan, in A and in B: 0 nothing, 1 water, 2 cliff, 3 building, 4 street (a floor to walk)."""
    from matplotlib.path import Path
    x0, z0, x1, z1 = PLAN_BOX
    nx = int(round((x1 - x0) / PLAN_CELL))
    nz = int(round((z1 - z0) / PLAN_CELL))
    sheets = {"A": np.zeros((nz, nx), np.uint8), "B": np.zeros((nz, nx), np.uint8)}
    cx = x0 + (np.arange(nx) + 0.5) * PLAN_CELL
    cz = z0 + (np.arange(nz) + 0.5) * PLAN_CELL
    gx, gz = np.meshgrid(cx, cz)
    sea = carved(gx, gz) < SEA + 0.05                                     # the sea and the basin (the same in both moments)
    for w in sheets.values():
        w[sea] = PLAN_WATER
    step = 0.35                                                           # the samples of a polygon: finer than a cell
    for pts, mat, world, kind in POLYS:
        if kind == "sea" or (mat in PLAN_SKIP and kind != "water"):
            continue
        pts = np.asarray(pts, float)
        n = np.cross(pts[1] - pts[0], pts[2] - pts[0])
        ln = float(np.linalg.norm(n))
        if ln < 1e-9:
            continue
        ny = abs(float(n[1])) / ln
        if kind == "water":
            cls = PLAN_WATER if ny > 0.5 else 0
        elif kind == "terrain":
            cls = PLAN_CLIFF if ny < 0.75 else 0                          # a slope of more than about 41 degrees
        elif kind == "overlay":
            cls = PLAN_STREET if mat == "paving" else 0
        else:
            if ny < 0.25:
                continue                                                  # a wall seen edge-on: its top carries the footprint
            cls = PLAN_STREET if (mat == "paving" and ny > 0.8) else PLAN_BUILDING
        if cls == 0:
            continue
        xs, zs = pts[:, 0], pts[:, 2]
        if xs.max() < x0 or xs.min() > x1 or zs.max() < z0 or zs.min() > z1:
            continue
        sx = np.arange(max(xs.min(), x0) + step / 2, min(xs.max(), x1), step)
        sz = np.arange(max(zs.min(), z0) + step / 2, min(zs.max(), z1), step)
        if len(sx) == 0 or len(sz) == 0:
            sx = np.array([(xs.min() + xs.max()) / 2])
            sz = np.array([(zs.min() + zs.max()) / 2])
        mx, mz = np.meshgrid(sx, sz)
        pts2 = np.stack([mx.ravel(), mz.ravel()], axis=1)
        inside = Path(np.stack([xs, zs], axis=1)).contains_points(pts2, radius=1e-6)
        if not inside.any():
            continue
        hit = pts2[inside]
        ci = np.clip(((hit[:, 0] - x0) / PLAN_CELL).astype(int), 0, nx - 1)
        cj = np.clip(((hit[:, 1] - z0) / PLAN_CELL).astype(int), 0, nz - 1)
        for key in (("A", "B") if world == "both" else (world,)):
            sheet = sheets[key]
            cur = sheet[cj, ci]
            better = np.array([PLAN_PRIORITY[int(c)] for c in cur]) < PLAN_PRIORITY[cls]
            sheet[cj[better], ci[better]] = cls
    return sheets
TOWN_VERSION = 6
KIND_CODE = {"solid": 0, "terrain": 1, "overlay": 2, "water": 3}
WORLD_CODE = {"A": 1, "B": 2, "both": 3}
SPAWN = (14.0, QUAY_Y, -72.0, -0.55, 0.29)  # x, y (the feet), z, yaw (0 looks north, +z), pitch (+ up): on the quay, east of the
#   sea stair. The first frame holds the heap of the quay (left), the winch of the sea gate (left of the middle) and the three lamps of the
#   signal mast, high in the sky over the wall (right of the middle); tests/test_firstminute.cpp keeps it so for every lens.
START_MOMENT = "B"                    # the game starts in the present (docs/DESIGN.md 3.4): the heap of the quay is the first view
STATE_CODE = {"shut": 0, "open": 1}


def export(path):
    """Writes the town: little-endian, every number a float32 unless said otherwise.
        magic (8 bytes) "DLTOWN" 0 0, version (u32)
        sea level, spawn x, y, z, yaw, pitch; the moment of the start (u8: 0 A, 1 B)
        2 lights (A, B): to the light (3), its colour (3), the shade (3), the sky top (3), the horizon (3), fog density
        materials (u32 count): name (u8 length, bytes), colour in A (3), in B (3), shines in A (u8), in B (u8); sRGB albedo
        places (u32 count): name (u8 length, bytes), x, y, z: the stations of the main path
        streets (u32 count): name (u8 length, bytes), kind (u8: 0 street, 1 stair, 2 lane), width, point count (u16), the
            points of its middle line (x, y, z each): what the tests walk
        polygons (u32 count): world (u8: 1 A, 2 B, 3 both), kind (u8: 0 solid, 1 terrain, 2 overlay, 3 water), material
            (u16), vertex count (u8), the vertices (x, y, z each)
        things (u32 count): name (u8 length, bytes), verb (u8: 0 open, 1 push, 2 lay, 3 ring, 4 lower), handle x, y, z, box count
            (u16), the boxes: state (u8: 0 shut, 1 open),
            world (u8), material (u16), centre (3), half size (3), yaw
        steps (u32 count): the main path in the order it is played (docs/DESIGN.md 7.4, 8): name (u8 length, bytes), kind (u8:
            0 a thing: E at its handle, 1 the lantern: set down at the spot), x, y, z (the handle, or the spot). The game
            guides from them (scene/Guidance.h) and tests/test_route.cpp plays them in this order.
        emblems (u32 count): glyph (u8 length, bytes), bell (u8 length, bytes: the thing whose ringing lights it), x, y, z, size,
            shape (u8: 0 sphere, 1 cylinder, 2 cube): the lamps of the signal mast (scene/Emblems.h)
        overlooks (u32 count): name (u8 length, bytes), x, y, z, radius: where standing reveals the map (scene/MapMemory.h)
        lamps (u32 count): x, y, z, range, colour (3, sRGB), intensity, flicker (float32), worlds (u8: 1 A, 2 B, 3 both), kind (u8: 0 street,
            1 post, 2 light, 3 memory, 4 grave): the lights of the town (scene/Lamps.h)
        plan: cell size, x0, z0 (float32), nx, nz (u16), then nx * nz bytes for A and nx * nz for B, row by row from z0 (south) up,
            each x from x0: 0 nothing, 1 water, 2 cliff, 3 building, 4 street (scene/Town.h Plan)
    The sea is not written: the game draws its own. Returns the number of polygons written."""
    import struct
    names = list(MAT.keys())
    index = {n: i for i, n in enumerate(names)}
    out = bytearray()
    out += TOWN_MAGIC + struct.pack("<I", TOWN_VERSION)
    out += struct.pack("<6fB", SEA, *SPAWN, {"A": 0, "B": 1}[START_MOMENT])
    for w in ("A", "B"):
        to_light, light, amb, sky_top, horizon, fog = LIGHT[w]
        out += struct.pack("<16f", *to_light, *light, *amb, *sky_top, *horizon, fog)
    out += struct.pack("<I", len(names))
    for n in names:
        a, b = MAT[n]
        em = EMISSIVE.get(n, (False, False))
        raw = n.encode("ascii")
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<6f2B", *a, *b, int(em[0]), int(em[1]))
    out += struct.pack("<I", len(MAIN_PATH))
    for name, x, z, _ in MAIN_PATH:
        key = name.split(" ")[0] if name[0].isdigit() else name
        raw = name.encode("ascii")[:255]
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<3f", x, HEIGHT_OF.get(key, float(ground(x, z))), z)
    out += struct.pack("<I", len(STREET))
    for name, (c, w, kind) in STREET.items():
        raw = name.encode("ascii")
        hs = _HEIGHTS[name]
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<BfH", {"street": 0, "stair": 1, "lane": 2}[kind], w, len(c))
        out += np.stack([c[:, 0], hs, c[:, 1]], axis=1).astype("<f4").tobytes()
    body = bytearray()
    count = 0
    for pts, mat, world, kind in POLYS:
        if kind == "sea" or len(pts) < 3:
            continue
        if len(pts) > 255:
            raise ValueError("a polygon of %d vertices" % len(pts))
        body += struct.pack("<BBHB", WORLD_CODE[world], KIND_CODE[kind], index[mat], len(pts))
        body += np.asarray(pts, dtype="<f4").tobytes()
        count += 1
    out += struct.pack("<I", count) + body
    out += struct.pack("<I", len(THINGS))
    for name, handle, boxes, verb in THINGS:
        raw = name.encode("ascii")
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<B3fH", VERB_CODE[verb], *handle, len(boxes))
        for st, world, mat, centre, half, yaw in boxes:
            out += struct.pack("<BBH7f", STATE_CODE[st], WORLD_CODE[world], index[mat], *centre, *half, yaw)
    out += struct.pack("<I", len(STEPS))
    for name, kind, spot in STEPS:
        raw = name.encode("ascii")
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<B3f", STEP_KIND[kind], *[float(v) for v in spot])
    out += struct.pack("<I", len(EMBLEMS))
    for glyph, bell, x, y, z, size, shape in EMBLEMS:
        g, b = glyph.encode("ascii"), bell.encode("ascii")
        out += struct.pack("<B", len(g)) + g + struct.pack("<B", len(b)) + b + struct.pack("<4fB", x, y, z, size, shape)
    out += struct.pack("<I", len(OVERLOOKS))
    for name, x, y, z, r in OVERLOOKS:
        raw = name.encode("ascii")
        out += struct.pack("<B", len(raw)) + raw + struct.pack("<4f", x, y, z, r)
    out += struct.pack("<I", len(LAMPS))
    for x, y, z, rng, col, inten, flick, world, kind in LAMPS:
        out += struct.pack("<8f", x, y, z, rng, *col, inten) + struct.pack("<fBB", flick, WORLD_CODE[world], LAMP_KIND[kind])
    sheets = build_plan()
    nz, nx = sheets["A"].shape
    out += struct.pack("<3fHH", PLAN_CELL, PLAN_BOX[0], PLAN_BOX[1], nx, nz)
    out += sheets["A"].tobytes() + sheets["B"].tobytes()
    for k in ("A", "B"):
        counts = np.bincount(sheets[k].ravel(), minlength=5)
        print("  plan %s: %d x %d cells; water %d, cliff %d, building %d, street %d" % (k, nx, nz, *counts[1:]))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(out)
    print("  lamps: %d (%s)" % (len(LAMPS), ", ".join("%s %d" % (k, sum(1 for l in LAMPS if l[8] == k)) for k in LAMP_KIND)))
    print("%s: %d polygons, %d things, %d materials, %.1f MB" % (os.path.relpath(path), count, len(THINGS), len(names), len(out) / 1e6))
    return count


def build_arrival_trim():
    """Non-colliding edge details for the quay and first street approach."""
    _, _, _, quay_end = HARBOUR
    quay_edge = quay_end - 3.0
    first = len(POLYS)
    for x in (-15.0, -10.0, -5.0, 18.0, 23.0, 28.0):
        tube((x, QUAY_Y - 0.12, quay_edge + 0.27),
             (x, QUAY_Y + 0.42, quay_edge + 0.27), 0.11, 7, "wood")
        tube((x, QUAY_Y + 0.30, quay_edge + 0.27),
             (x, QUAY_Y + 0.35, quay_edge + 0.27), 0.125, 7, "iron")
        tube((x, SEA - 0.2, quay_edge - 0.13),
             (x, QUAY_Y + 0.02, quay_edge - 0.13), 0.14, 6, "wood")
    for name, begin, end in (("sea stair", 1.2, 6.4), ("lower street", 1.2, 10.0)):
        for start in np.arange(begin, end, 0.78):
            edge = []
            inner = []
            for at_s in (start, start + 0.72):
                point, y, _, normal, width = street_frame(name, at_s)
                for strip, offset in ((edge, 0.10), (inner, 0.29)):
                    x, z = point + normal * (width / 2 - offset)
                    strip.append((float(x), y + 0.065, float(z)))
            poly([inner[0], inner[1], edge[1], edge[0]], "stone_dark", kind="overlay")
    for i in range(first, len(POLYS)):
        points, mat, world, _ = POLYS[i]
        POLYS[i] = (points, mat, world, "overlay")


def build_city_trim():
    """Visual-only street dressing; keeps the walkable lane and puzzle clearances unchanged."""
    first = len(POLYS)
    specs = (("lower street", 11.0, 28.0, -1), ("market stairs", 1.0, 20.0, 1),
             ("market street", 2.0, 22.0, 1), ("upper street", 30.0, 55.0, -1),
             ("cliff stair", 1.0, 21.0, 1))
    for name, lo, hi, side in specs:
        total = float(lengths(STREET[name][0])[-1])
        for start in np.arange(lo, min(hi, total - 0.8), 0.92):
            outer, inner = [], []
            for ss in (start, start + 0.8):
                point, y, _, normal, width = street_frame(name, ss)
                for strip, inset in ((outer, 0.09), (inner, 0.27)):
                    x, z = point + normal * side * (width / 2 - inset)
                    strip.append((float(x), y + 0.055, float(z)))
            poly([inner[0], inner[1], outer[1], outer[0]], "stone_dark" if int(start) % 4 else "stone", kind="overlay")
        for ss in np.arange(lo + 2.8, min(hi, total - 1.0), 8.0):
            point, y, _, normal, width = street_frame(name, ss)
            x, z = point + normal * side * (width / 2 + 0.28)
            tube((float(x), y, float(z)), (float(x), y + 2.75, float(z)), 0.052, 6, "iron")
            # A keeps the working lamp; B retains the same post without a floating/unmotivated light box.
            prism(rect(float(x), float(z), 0.36, 0.36, 0.0), y + 2.50, y + 2.80, "lantern", "A")
            lamp(float(x), y + 2.65, float(z), "A", "street")
    for j in range(first, len(POLYS)):
        pts, mat, world, _ = POLYS[j]
        POLYS[j] = (pts, mat, world, "overlay")


def build_all():
    """Every part of the town, in the order they depend on each other (the puzzles keep the houses off their places)."""
    build_terrain()
    build_streets()
    build_rock()
    build_wall()
    build_harbour()
    build_west()
    build_east()
    build_north()
    build_bastion_gate()
    build_harbour_beats()
    build_harbour_wayfinding()
    build_air_step()
    corridor("lower street", 30.0, 53.7)
    corridor("market stairs", 0.0, 22.0)
    corridor("market street", 4.0, 24.0)
    corridor("upper street", 4.0, 24.0)
    corridor("upper street", 56.0, 76.0)
    build_canal()
    build_scaffold()
    build_plank()
    build_gatehouse()
    build_lift()
    build_gorge_parapet()
    build_district_walls()
    build_arches()
    build_houses()
    build_bells()
    build_signal_mast()
    build_steps()
    build_country()
    build_arrival_trim()
    build_city_trim()


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    build_all()
    print("%d polygons, %d houses" % (len(POLYS), len(HOUSES)))
    if only in ("", "check", "export"):
        check()
        if only == "check":
            return 1 if PROBLEMS else 0
        if only == "export":
            if PROBLEMS:
                print("not exported: the check found problems")
                return 1
            export(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "town.dlt"))
            return 0
    POLYS.extend(thing_polys("shut"))                    # the pictures show the things as the game starts
    os.makedirs(OUT, exist_ok=True)
    views = {
        "aerial_a": ("A", "city2_aerial_a.png", (-70.0, 120.0, -235.0), (8.0, 4.0, -8.0), 44.0, (1600, 1000),
                     "World A - 7:11, warm dusk (from over the sea)"),
        "aerial_b": ("B", "city2_aerial_b.png", (-70.0, 120.0, -235.0), (8.0, 4.0, -8.0), 44.0, (1600, 1000),
                     "World B - now, cold ruin (from over the sea)"),
        "street_a": ("A", "city2_street_a.png", (0.0, 0.0, 0.0), (0.0, ROCK_Y + 12.0, 4.0), 58.0, (1600, 900),
                     "World A - on the west bastion: the bridge over the gorge to the Rock of the Bell"),
        "street_b": ("B", "city2_street_b.png", (0.0, 0.0, 0.0), (0.0, ROCK_Y + 12.0, 4.0), 58.0, (1600, 900),
                     "World B - the bridge has fallen; the lantern set down on it in A keeps its middle standing"),
        "vista_a": ("A", "city2_vista_a.png", (10.0, GRAVE_Y + 15.0, 92.0), (0.0, 10.0, -12.0), 74.0, (1600, 900),
                    "World A - from the night bell tower over the graveyard: the theatre, the Rock of the Bell, the town, the sea"),
        "vista_b": ("B", "city2_vista_b.png", (10.0, GRAVE_Y + 15.0, 92.0), (0.0, 10.0, -12.0), 74.0, (1600, 900),
                    "World B - the same view, after the storm"),
    }
    for key, (world, name, eye, target, fov, size, title) in views.items():
        if key.startswith("street"):                     # on the bastion: the gate tower, the bridge through it, the rock
            eye = (BRIDGEHEADS["west"][0] - 4.3, ROCK_Y + 1.7, -0.4)
            target = (-4.0, ROCK_Y + 4.0, 0.4)
        if only in ("", key):
            n = render(world, os.path.join(OUT, name), eye, target, fov, size, title,
                       lantern=LANTERN_DEMO if key == "street_b" else None)
            print("%s: %d polygons drawn" % (name, n))
    if only in ("", "plan"):
        n = render("A", os.path.join(OUT, "city2_plan.png"), (4.0, 300.0, -2.0), (4.0, 0.0, -2.0001), 0.0, (1400, 1300),
                   "Plan: the main path (red: walked, blue dashed: the cistern under the town, grey dotted: to be designed)", plan_overlay, ortho=5.4)
        print("city2_plan.png: %d polygons drawn" % n)
    return 0


if __name__ == "__main__":
    sys.exit(main())
