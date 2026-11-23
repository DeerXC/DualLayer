"""city_model.py - the terrain and block model of the planned map (docs/LEVEL_DESIGN.md, section 5).

A design model, not game data. The city is built on a terrain (a height field: the sea to the south, a ridge to the
south-west, a hillside rising east, a cliff and the mountain to the north). Streets are curves that cut their own bed into
the terrain; houses are placed along them, turned to the street, so that the districts run into one another. The landmarks
(the bell tower, the well, the locks, the theatre, the graveyard, the aqueduct, the town wall) are solids on that terrain.
The script draws five figures into docs/images and checks a few rules of section 10 on the model.

    python tools/layout/city_model.py            (Python 3, numpy, matplotlib)

Units: metres. x east, z north, y up; y = 0 is the rim of the square (world y = y - 1 in the game).
Worlds: "both", "A" (whole, 7:11), "B" (after the storm).
"""

import math
import os
import sys

import numpy as np

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "images")

# ---- colours (sRGB) -------------------------------------------------------------------------------------------------------
STONE = (0.82, 0.75, 0.62)
STONE_B = (0.50, 0.50, 0.50)
STONE_DARK = (0.62, 0.56, 0.47)
PAVING = (0.74, 0.68, 0.58)
GRASS = (0.52, 0.60, 0.38)
GRASS_B = (0.36, 0.43, 0.33)
ROCK = (0.55, 0.52, 0.48)
SAND = (0.80, 0.74, 0.58)
ROOF = (0.66, 0.33, 0.24)
ROOF_B = (0.42, 0.28, 0.24)
WOOD = (0.55, 0.38, 0.22)
CLOTH = (0.80, 0.30, 0.25)
CLOTH2 = (0.90, 0.75, 0.35)
IRON = (0.32, 0.34, 0.38)
DEBRIS = (0.40, 0.37, 0.34)
WATER = (0.25, 0.50, 0.85)
SEA = (0.22, 0.38, 0.58)
GOLD = (1.00, 0.80, 0.30)
TREE = (0.24, 0.38, 0.22)
TREE_B = (0.26, 0.30, 0.24)
MEMORY = (0.80, 0.95, 1.00)
SEA_LEVEL = -8.0

def ss(e0, e1, v):
    """Smooth step from e0 to e1 (e0 may be larger than e1)."""
    t = np.clip((v - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

# ---- curves ---------------------------------------------------------------------------------------------------------------
def spline(points, step=1.0):
    """A Catmull-Rom curve through `points` (x, z), sampled about every `step` metres. Returns an (n, 2) array."""
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

def lengths(curve):
    d = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(curve, axis=0), axis=1))]
    return d

def at(curve, t):
    """The point at fraction t of the length of `curve`, and the unit direction there."""
    d = lengths(curve)
    s = t * d[-1]
    i = int(np.clip(np.searchsorted(d, s) - 1, 0, len(curve) - 2))
    u = (s - d[i]) / max(d[i + 1] - d[i], 1e-9)
    p = curve[i] + u * (curve[i + 1] - curve[i])
    v = curve[i + 1] - curve[i]
    return p, v / max(np.linalg.norm(v), 1e-9)

def distance_to(curve, x, z):
    """The distance from points (x, z) (arrays) to the polyline, and the index of the nearest sample."""
    px = np.asarray(x, float)[..., None]
    pz = np.asarray(z, float)[..., None]
    a = curve[:-1]
    b = curve[1:]
    ab = b - a
    l2 = np.maximum((ab ** 2).sum(1), 1e-9)
    t = np.clip(((px - a[:, 0]) * ab[:, 0] + (pz - a[:, 1]) * ab[:, 1]) / l2, 0, 1)
    qx = a[:, 0] + t * ab[:, 0]
    qz = a[:, 1] + t * ab[:, 1]
    d = np.hypot(px - qx, pz - qz)
    i = d.argmin(-1)
    return d.min(-1), i

# ---- the plan: streets, the square, the landmarks -------------------------------------------------------------------------
SQUARE = (0.0, 34.0)
SQUARE_R = 16.0          # the rim of the square: an irregular round place, 32 m across
POOL_R0, POOL_R1 = 6.5, 10.5
TOWER = (0.0, 34.0)
WELL = (-40.0, 30.0)
WELL_R = 8.0
THEATRE = (0.0, 58.0)    # the centre of the orchestra; the seats rise north on the hillside
STAGE_Y = 0.8

# streets: name, control points, width, kind ("street", "stair", "lane"); their heights follow the terrain, smoothed
STREETS = [
    ("the harbour road", [(34, -42), (24, -42), (14, -44), (6, -46), (-4, -45), (-6, -42), (0, -40)], 3.6, "lane"),
    ("the headland path", [(-4, -45), (-12, -49), (-20, -48), (-28, -53), (-36, -52), (-44, -57), (-50, -56)], 2.2, "lane"),
    ("the high street", [(1, -33), (7, -24), (5, -13), (10, -2), (9, 9), (12, 19), (9, 23)], 4.0, "street"),
    ("the lower lane", [(-4, -33), (-11, -26), (-12, -16), (-18, -8), (-16, 2), (-22, 10), (-19, 18), (-23, 25)], 3.0, "lane"),
    ("the market street", [(-13, 31), (-19, 28), (-25, 31), (-31, 30)], 5.0, "street"),
    ("the clock lane", [(-12, 40), (-20, 43), (-26, 45)], 3.0, "lane"),
    ("the theatre lane", [(-28, 38), (-26, 48), (-20, 56), (-16, 63)], 3.0, "lane"),
    ("the lock walk", [(11, 35.0), (18, 34.6), (25, 35.3), (32, 34.6), (39, 35.3), (46, 34.6), (53, 35.3), (60, 34.4)], 3.6, "street"),
    ("the hill lane", [(58, 33), (54, 41), (46, 44), (42, 51), (33, 54), (28, 60), (20, 63)], 3.0, "lane"),
    ("the cliff path", [(14, 66), (22, 70), (14, 75), (22, 80), (12, 85)], 2.5, "stair"),
    ("the north lane", [(10, 46), (17, 50), (14, 55), (19, 59), (16, 63)], 3.0, "lane"),
]
STREET = {name: (spline(points), width, kind) for name, points, width, kind in STREETS}

# the town wall: a closed curve (it runs along the cliff top in the north); the gate is in its south side
WALL_POINTS = [(0, -37), (22, -35), (42, -26), (60, -10), (70, 12), (68, 36), (60, 54), (42, 68), (26, 76), (10, 80), (-14, 80),
               (-32, 62), (-50, 50), (-58, 30), (-56, 6), (-46, -16), (-28, -32), (0, -37)]
WALL = spline(WALL_POINTS, 2.0)
GATE_T = (0.0, 0.025)    # the gate: the first few metres of the curve

COURTYARD = (-4.5, 12.5, 4.5, 22.0)   # x0, z0, x1, z1
WALL_WALK = spline([(-1, -30), (-6, -20), (-5, -9), (-2, 1), (0, 11)], 1.0)
WALK_Y = 5.0             # the top of the wall walk
WALK_GAP = (0.47, 0.60)  # the break in it (fractions of its length)
AQUEDUCT = spline([(96, 42), (78, 46), (60, 44), (44, 46), (28, 44), (16, 42), (9, 38)], 2.0)
AQUEDUCT_Y = (17.0, 10.0)  # its deck, from the hills to the spout over the pool
AQUEDUCT_BREAK = (0.38, 0.46)
LOCKS = [(12.0, 24.0, -1.5), (24.0, 34.0, 1.5), (34.0, 44.0, 4.5), (44.0, 54.0, 8.0), (54.0, 62.0, 11.0)]  # x0, x1, quay height
CANAL_HALF = 2.2

# ---- the terrain ----------------------------------------------------------------------------------------------------------
def _hash(ix, iz, seed):
    h = (ix * 374761393 + iz * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return (h & 0xFFFF) / 65535.0

def value_noise(x, z, seed=0):
    """Smooth value noise in -1 .. 1, one cell per unit."""
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    ix, iz = np.floor(x).astype(np.int64), np.floor(z).astype(np.int64)
    fx, fz = x - ix, z - iz
    ux, uz = fx * fx * (3 - 2 * fx), fz * fz * (3 - 2 * fz)
    a, b = _hash(ix, iz, seed), _hash(ix + 1, iz, seed)
    c, d = _hash(ix, iz + 1, seed), _hash(ix + 1, iz + 1, seed)
    return 2.0 * (a + (b - a) * ux + (c - a) * uz + (a - b - c + d) * ux * uz) - 1.0

def fbm(x, z, octaves=4, seed=0):
    total, amp, freq, norm = 0.0, 1.0, 1.0, 0.0
    for o in range(octaves):
        total = total + amp * value_noise(x * freq, z * freq, seed + o * 17)
        norm += amp
        amp *= 0.5
        freq *= 2.03
    return total / norm

def ridged(x, z, octaves=4, seed=0):
    """Ridged noise in 0 .. 1: sharp crests, as a mountain skyline."""
    total, amp, freq, norm = 0.0, 1.0, 1.0, 0.0
    for o in range(octaves):
        total = total + amp * (1.0 - np.abs(value_noise(x * freq, z * freq, seed + o * 31)))
        norm += amp
        amp *= 0.5
        freq *= 2.1
    return total / norm

def coast_z(x):
    """The shore line (z of the water's edge) along x: a headland under the ridge in the west, the harbour cove under the
    gate, a long beach to the east, all of it uneven."""
    x = np.asarray(x, float)
    z = -50.0 + 4.0 * fbm(x / 26.0, 3.1, 3, 7)
    z = z + 9.0 * np.exp(-((x - 36.0) / 12.0) ** 2)                     # the cove of the harbour
    z = z - 22.0 * ss(-36.0, -70.0, x)                                  # the headland reaches out to sea
    z = z + 6.0 * ss(60.0, 100.0, x)                                    # the shore turns north-east
    return z

def natural(x, z):
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    wx = x + 9.0 * fbm(x / 60.0, z / 60.0, 2, 3)                        # a warp, so that no contour is a straight line
    wz = z + 9.0 * fbm(x / 60.0 + 5.2, z / 60.0 + 1.3, 2, 4)
    y = np.zeros_like(x)
    y = y + 7.0 * ss(46.0, 64.0, wz) + 12.0 * ss(64.0, 74.0, wz)        # the hillside and the cliff
    y = y + 12.0 * ss(14.0, 62.0, wx) * ss(-24.0, 12.0, wz)             # the hillside of the east
    y = y + 9.0 * ss(70.0, 110.0, wx)
    y = y - 2.5 * ss(-14.0, -44.0, wx) * ss(80.0, 60.0, wz)              # the west sinks a little
    y = y + 18.0 * np.exp(-((wx + 50.0) ** 2 / 500.0 + (wz + 50.0) ** 2 / 160.0))   # the ridge of the headland
    y = y + 11.0 * ss(-56.0, -100.0, wx) * ss(-70.0, -20.0, wz)          # the hills of the west
    mountain = ss(92.0, 140.0, wz)                                       # the mountain: ridged, rising in layers
    y = y + mountain * (22.0 + 38.0 * ridged(x / 70.0, z / 70.0, 4, 11) * ss(100.0, 160.0, wz))
    y = y + 10.0 * ss(70.0, 120.0, np.abs(x)) * ss(70.0, 95.0, wz)
    town = ss(95.0, 60.0, np.hypot(x - 6.0, (z - 20.0) * 0.9))          # rough land outside the town, calm inside it
    y = y + (1.0 + 3.5 * town) * fbm(x / 22.0, z / 22.0, 4, 1)
    cz = coast_z(x)                                                     # the shore: a beach, or a cliff under the headland
    cliff = ss(-36.0, -62.0, x)
    shore = SEA_LEVEL + (0.22 + 0.9 * cliff) * (z - cz)
    y = np.where(z < cz + 30.0, np.minimum(y, np.maximum(shore, SEA_LEVEL - 6.0) + 0.0 * y), y)
    y = np.where(z < cz, np.minimum(y, SEA_LEVEL - 0.6 + 0.35 * (z - cz)), y)
    return np.maximum(y, SEA_LEVEL - 14.0)

_HEIGHTS = {}

def street_heights(curve):
    """Heights along a street: the terrain, smoothed, so that it is a walkable bed."""
    key = id(curve)
    if key not in _HEIGHTS:
        _HEIGHTS[key] = _street_heights(curve)
    return _HEIGHTS[key].copy()

def _street_heights(curve):
    kind = next((k for c, w, k in STREET.values() if c is curve), "street")
    h = natural(curve[:, 0], curve[:, 1])
    k = np.ones(9) / 9.0
    hp = np.r_[np.full(4, h[0]), h, np.full(4, h[-1])]
    h = np.convolve(hp, k, mode="valid")
    ds = np.maximum(np.linalg.norm(np.diff(curve, axis=0), axis=1), 1e-6)
    grade = 0.60 if kind == "stair" else 0.45                            # 31 / 24 degrees: cut and fill where steeper
    for _ in range(4):
        for i in range(1, len(h)):
            h[i] = np.clip(h[i], h[i - 1] - grade * ds[i - 1], h[i - 1] + grade * ds[i - 1])
        for i in range(len(h) - 2, -1, -1):
            h[i] = np.clip(h[i], h[i + 1] - grade * ds[i], h[i + 1] + grade * ds[i])
    return h

def lock_height(x):
    for x0, x1, y in LOCKS:
        if x0 <= x < x1:
            return y
    return LOCKS[-1][2] if x >= LOCKS[-1][1] else 0.0

def ground(x, z):
    """The terrain with the square, the streets, the courtyard and the locks cut into it."""
    x = np.asarray(x, float)
    z = np.asarray(z, float)
    y = natural(x, z)
    r = np.hypot(x - SQUARE[0], z - SQUARE[1])
    wob = 1.0 + 0.10 * np.sin(np.arctan2(z - SQUARE[1], x - SQUARE[0]) * 3.0 + 0.7)
    rim = SQUARE_R * wob
    y = y + (0.0 - y) * (1.0 - ss(rim, rim + 6.0, r))                  # the square, flat, blended into the slopes round it
    y = np.where((r > POOL_R0) & (r < POOL_R1), -1.5, y)               # the pool round the tower
    for name, (curve, width, kind) in STREET.items():
        if name == "the lock walk":
            continue
        d, i = distance_to(curve, x, z)
        hs = street_heights(curve)[i]
        w = 1.0 - ss(width / 2 + 0.5, width / 2 + 4.0, d)
        y = y + (hs - y) * w
    rt = np.hypot(x - THEATRE[0], z - THEATRE[1])                      # the theatre: a bowl cut into the hillside
    bowl = (z > THEATRE[1] - 12.0) & (rt < 18.0)
    seats = np.where(rt < 6.5, STAGE_Y, STAGE_Y + 0.65 * np.minimum(np.ceil((rt - 6.5) / 1.3), 8.0))
    y = np.where(bowl & (z > THEATRE[1] - 0.5), np.minimum(seats - 3.0, y) * 0 + seats - 0.02, y)
    y = np.where(bowl & (z <= THEATRE[1] - 0.5), STAGE_Y, y)
    x0, z0, x1, z1 = COURTYARD
    inside = (x > x0 - 2) & (x < x1 + 2) & (z > z0 - 2) & (z < z1 + 2)
    y = np.where(inside, 0.0, y)
    lock = (x > LOCKS[0][0]) & (x < LOCKS[-1][1]) & (z > 20.0) & (z < 36.0)
    quay = np.vectorize(lock_height)(x)
    y = np.where(lock, quay, y)
    d, _ = distance_to(np.array([[WELL[0] - 0.01, WELL[1]], [WELL[0] + 0.01, WELL[1]]]), x, z)
    y = np.where(d < WELL_R + 4.0, np.minimum(y, natural(WELL[0], WELL[1])) * 0 + 0.0, y)   # the rim of the well: level with the square
    return y

# ---- solids ---------------------------------------------------------------------------------------------------------------
POLYS = []   # (points (n, 3) as x, y, z), colour, world, kind ("solid", "water", "terrain", "glow")

def poly(points, colour, world="both", kind="solid"):
    POLYS.append((np.asarray(points, float), colour, world, kind))

def prism(outline, y0, y1, colour, world="both", top=True, top_colour=None):
    """A vertical prism over a 2D outline [(x, z), ...] (counter-clockwise or not)."""
    n = len(outline)
    for i in range(n):
        (ax, az), (bx, bz) = outline[i], outline[(i + 1) % n]
        poly([(ax, y0, az), (bx, y0, bz), (bx, y1, bz), (ax, y1, az)], colour, world)
    if top:
        poly([(x, y1, z) for x, z in outline], top_colour or colour, world)

def rect(cx, cz, w, d, angle):
    c, s = math.cos(angle), math.sin(angle)
    pts = []
    for ux, uz in ((-w / 2, -d / 2), (w / 2, -d / 2), (w / 2, d / 2), (-w / 2, d / 2)):
        pts.append((cx + ux * c - uz * s, cz + ux * s + uz * c))
    return pts

def ngon(cx, cz, r, n, phase=0.0):
    return [(cx + r * math.cos(phase + 2 * math.pi * k / n), cz + r * math.sin(phase + 2 * math.pi * k / n)) for k in range(n)]

def gable(cx, cz, w, d, angle, y0, rise, colour, world="both"):
    """A pitched roof over a w x d footprint, its ridge along d."""
    c, s = math.cos(angle), math.sin(angle)

    def p(ux, uz, y):
        return (cx + ux * c - uz * s, y, cz + ux * s + uz * c)
    hw, hd = w / 2 + 0.3, d / 2 + 0.3
    poly([p(-hw, -hd, y0), p(-hw, hd, y0), p(0, hd, y0 + rise), p(0, -hd, y0 + rise)], colour, world)
    poly([p(hw, -hd, y0), p(hw, hd, y0), p(0, hd, y0 + rise), p(0, -hd, y0 + rise)], colour, world)
    poly([p(-hw, -hd, y0), p(hw, -hd, y0), p(0, -hd, y0 + rise)], colour, world)
    poly([p(-hw, hd, y0), p(hw, hd, y0), p(0, hd, y0 + rise)], colour, world)

def cone(cx, cz, r, y0, y1, colour, world="both", n=8):
    ring = ngon(cx, cz, r, n)
    for i in range(n):
        (ax, az), (bx, bz) = ring[i], ring[(i + 1) % n]
        poly([(ax, y0, az), (bx, y0, bz), (cx, y1, cz)], colour, world)

def ribbon(curve, heights, width, colour, world="both", thickness=0.0):
    """A strip along a curve at the given heights (one per sample), `thickness` down as a side."""
    n = len(curve)
    for i in range(n - 1):
        a, b = curve[i], curve[i + 1]
        v = b - a
        v = v / max(np.linalg.norm(v), 1e-9)
        nx, nz = -v[1] * width / 2, v[0] * width / 2
        ya, yb = heights[i], heights[i + 1]
        poly([(a[0] + nx, ya, a[1] + nz), (b[0] + nx, yb, b[1] + nz), (b[0] - nx, yb, b[1] - nz), (a[0] - nx, ya, a[1] - nz)], colour, world)
        if thickness > 0:
            poly([(a[0] + nx, ya, a[1] + nz), (b[0] + nx, yb, b[1] + nz), (b[0] + nx, yb - thickness, b[1] + nz), (a[0] + nx, ya - thickness, a[1] + nz)],
                 colour, world)
            poly([(a[0] - nx, ya, a[1] - nz), (b[0] - nx, yb, b[1] - nz), (b[0] - nx, yb - thickness, b[1] - nz), (a[0] - nx, ya - thickness, a[1] - nz)],
                 colour, world)

def beam(a, b, width, height, colour, world="both"):
    """A beam whose top runs from a to b (x, y, z)."""
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    v = np.array([b[0] - a[0], b[2] - a[2]])
    v = v / max(np.linalg.norm(v), 1e-9)
    nx, nz = -v[1] * width / 2, v[0] * width / 2
    t = [(a[0] + nx, a[1], a[2] + nz), (b[0] + nx, b[1], b[2] + nz), (b[0] - nx, b[1], b[2] - nz), (a[0] - nx, a[1], a[2] - nz)]
    btm = [(x, y - height, z) for x, y, z in t]
    poly(t, colour, world)
    for i in range(4):
        j = (i + 1) % 4
        poly([t[i], t[j], btm[j], btm[i]], colour, world)

# ---- the landmarks --------------------------------------------------------------------------------------------------------
def build_tower():
    cx, cz = TOWER
    prism(ngon(cx, cz, POOL_R0, 16), -1.5, 0.4, STONE_DARK)                                  # the plinth in the pool
    prism(ngon(cx, cz, 5.6, 8, math.pi / 8), 0.4, 16.0, STONE)                               # the nave
    for k in range(8):                                                                       # buttresses
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(cx + 5.9 * math.cos(a), cz + 5.9 * math.sin(a), 1.0, 1.4, a), 0.4, 12.0, STONE_DARK)
    prism(ngon(cx, cz, 4.0, 8, math.pi / 8), 16.0, 44.0, STONE, "A")                         # A: the shaft
    for k in range(8):
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(cx + 3.6 * math.cos(a), cz + 3.6 * math.sin(a), 0.9, 0.9, a), 44.0, 53.0, STONE, "A")  # the belfry
    prism(ngon(cx, cz, 4.4, 8, math.pi / 8), 53.0, 54.2, STONE, "A")
    prism(ngon(cx, cz, 2.2, 12), 46.5, 50.0, GOLD, "A")                                      # the great bell
    cone(cx, cz, 4.4, 54.2, 67.0, STONE, "A", 8)                                             # the spire
    for k in range(12):                                                                      # the ring of twelve lamps
        a = 2 * math.pi * k / 12
        prism(ngon(cx + 6.6 * math.cos(a), cz + 6.6 * math.sin(a), 0.3, 6), 16.0, 16.8, MEMORY, "A")
    prism(ngon(cx, cz, 4.0, 8, math.pi / 8), 16.0, 26.0, STONE_B, "B")                       # B: the stump ...
    for k in range(8):                                                                       # ... broken at 26 .. 31
        a = math.pi / 8 + 2 * math.pi * k / 8
        prism(rect(cx + 3.5 * math.cos(a), cz + 3.5 * math.sin(a), 1.6, 0.8, a + math.pi / 2), 26.0, 26.0 + (k * 7 % 5) + 1.0, STONE_B, "B")
    build_fallen_top()
    bx, bz = cx - 6.0, cz + 6.2                                                              # the fallen bell in the pool
    prism(ngon(bx, bz, 2.2, 12), -1.5, 1.4, GOLD, "B")
    prism(rect(bx, bz + 1.9, 0.8, 1.0, 0.3), -1.5, 1.42, DEBRIS, "B")                        # its crack, a way in

def frame(a, b):
    """Two unit vectors perpendicular to the axis a -> b."""
    u = (b - a) / np.linalg.norm(b - a)
    p = np.cross(u, [0.0, 1.0, 0.0])
    if np.linalg.norm(p) < 1e-6:
        p = np.array([1.0, 0.0, 0.0])
    p /= np.linalg.norm(p)
    q = np.cross(u, p)
    return p, q

def tube(a, b, r, n, colour, world="both", caps=(True, True), phase=math.pi / 8):
    """A prism of n sides lying along a -> b (its ends open when caps is False: a broken, hollow shaft)."""
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    p, q = frame(a, b)
    ring_a = [a + r * (math.cos(phase + 2 * math.pi * k / n) * p + math.sin(phase + 2 * math.pi * k / n) * q) for k in range(n)]
    ring_b = [x + (b - a) for x in ring_a]
    for k in range(n):
        j = (k + 1) % n
        poly([ring_a[k], ring_a[j], ring_b[j], ring_b[k]], colour, world)
    if caps[0]:
        poly(ring_a, colour, world)
    if caps[1]:
        poly(ring_b, colour, world)

def lying_cone(a, tip, r, n, colour, world="both"):
    a = np.asarray(a, float)
    tip = np.asarray(tip, float)
    p, q = frame(a, tip)
    ring = [a + r * (math.cos(2 * math.pi * k / n) * p + math.sin(2 * math.pi * k / n) * q) for k in range(n)]
    poly(ring, colour, world)
    for k in range(n):
        poly([ring[k], ring[(k + 1) % n], tip], colour, world)

def resting(x0, z0, x1, z1, r):
    """The heights of the two ends of a piece of radius r that lies on the ground from (x0, z0) to (x1, z1): each end rests
    on the highest ground under its half, sunk a little into it."""
    t = np.linspace(0.0, 1.0, 24)
    xs, zs = x0 + (x1 - x0) * t, z0 + (z1 - z0) * t
    g = ground(xs, zs)
    return float(g[:12].max()) + 0.85 * r, float(g[12:].max()) + 0.85 * r

FALLEN = []   # (name, a, b, radius): the pieces of the fallen top

def build_fallen_top():
    """B: the tower broke at 26 m and its top fell north. The broken end dropped at the foot of the tower and smashed into
    the north side of the nave; the rest broke on impact into three pieces that lie on the ground, across the forecourt, the
    stage and the seats of the theatre, the spire against the rock at its back. Nothing spans a void: every piece rests on
    the ground along its length. The shaft is hollow, its ends open: a way in from the theatre to the nave."""
    pieces = [("shaft", (0.4, 40.2), (1.6, 57.6), 4.0, 8, True), ("belfry", (2.2, 59.4), (4.6, 68.4), 4.4, 8, False)]
    for name, (x0, z0), (x1, z1), r, n, hollow in pieces:
        y0, y1 = resting(x0, z0, x1, z1, r)
        a, b = (x0, y0, z0), (x1, y1, z1)
        tube(a, b, r, n, STONE_B, "B", caps=(not hollow, not hollow))
        if hollow:
            tube(a, b, r - 0.5, n, (0.32, 0.32, 0.33), "B", caps=(False, False))     # its inside
        FALLEN.append((name, a, b, r))
    x0, z0, x1, z1 = 4.8, 70.0, 2.0, 82.0                                                # the spire, its tip against the rock
    y0, _ = resting(x0, z0, x1, z1, 4.2)
    tip = (x1, float(ground(x1, z1 - 2.0)) + 0.5, z1)
    lying_cone((x0, y0, z0), tip, 4.2, 8, STONE_B, "B")
    FALLEN.append(("spire", (x0, y0, z0), tip, 4.2))
    for x, z, w in ((-3.0, 57.0, 2.4), (6.0, 58.5, 2.0), (7.2, 69.0, 2.2), (-1.5, 66.0, 1.6), (4.2, 45.5, 1.8)):   # rubble
        y = float(ground(x, z))
        prism(rect(x, z, w, w * 0.8, x * 0.3), y - 0.2, y + 0.6 + w * 0.2, DEBRIS, "B")
    poly([(-2.2, 0.6, 39.65), (2.8, 0.6, 39.65), (2.8, 9.0, 39.65), (-2.2, 9.0, 39.65)], (0.15, 0.15, 0.16), "B")   # the hole

def build_square():
    cx, cz = SQUARE
    ring = ngon(cx, cz, POOL_R1, 24)
    for i in range(24):                                                                     # the kerb of the pool
        (ax, az), (bx, bz) = ring[i], ring[(i + 1) % 24]
        poly([(ax, -1.5, az), (bx, -1.5, bz), (bx, 0.05, bz), (ax, 0.05, az)], STONE_DARK)
    outer = ngon(cx, cz, POOL_R1 - 0.05, 32)
    inner = ngon(cx, cz, POOL_R0 + 0.05, 32)
    for i in range(32):                                                                     # the water of A
        j = (i + 1) % 32
        poly([(outer[i][0], -0.3, outer[i][1]), (outer[j][0], -0.3, outer[j][1]), (inner[j][0], -0.3, inner[j][1]), (inner[i][0], -0.3, inner[i][1])],
             WATER, "A", "water")
    ribbon(np.array([[0.0, 18.0], [0.0, 27.4]]), np.array([0.0, 0.0]), 3.0, PAVING, thickness=1.5)   # the causeway
    for a in (0.6, 2.1, 3.7, 5.0):                                                          # trees round the rim
        x, z = cx + (SQUARE_R - 1.5) * math.cos(a), cz + (SQUARE_R - 1.5) * math.sin(a)
        tree(x, z, 0.0)

def build_wall():
    h = ground(WALL[:, 0], WALL[:, 1])
    d = lengths(WALL)
    n = len(WALL)
    for i in range(n - 1):
        t = d[i] / d[-1]
        if GATE_T[0] <= t < GATE_T[1] or t > 1 - GATE_T[1]:
            continue
        a, b = WALL[i], WALL[i + 1]
        if distance_to(STREET["the cliff path"][0], 0.5 * (a[0] + b[0]), 0.5 * (a[1] + b[1]))[0] < 2.6:
            continue                                                                          # the postern of the cliff path
        base = min(h[i], h[i + 1]) - 3.0
        top = max(h[i], h[i + 1]) + 7.0
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 0.9, v[0] * 0.9
        prism([(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)], base, top,
              STONE_DARK, "both", True, STONE)
        if i % 3 == 0:
            hb = (i * 7) % 5 == 0
            prism([(a[0] + nx * 1.4, a[1] + nz * 1.4), (b[0] + nx * 1.4, b[1] + nz * 1.4), (b[0] + nx, b[1] + nz), (a[0] + nx, a[1] + nz)],
                  top, top + 1.0, STONE, "A" if hb else "both")
    for k, (x, z) in enumerate(WALL_POINTS[1:-1]):                                          # towers at the corners
        y = float(ground(x, z))
        prism(ngon(x, z, 2.6, 10), y - 3.0, y + 11.0, STONE, "both" if k % 3 else "A")
        cone(x, z, 3.0, y + 11.0, y + 14.0, ROOF, "A", 10)
        if k % 3 == 0:
            prism(ngon(x, z, 2.6, 10), y - 3.0, y + 6.0 + k % 2, STONE_B, "B")
    gx, gz = WALL[0]                                                                        # the gate and its towers
    yg = float(ground(gx, gz))
    for dx in (-4.0, 4.0):
        prism(rect(gx + dx, gz, 3.6, 4.0, 0.0), yg - 1.0, yg + 10.0, STONE)
        cone(gx + dx, gz, 2.8, yg + 10.0, yg + 13.0, ROOF, "A", 4)
    prism(rect(gx, gz, 4.4, 4.0, 0.0), yg + 5.0, yg + 10.0, STONE, "A")                    # the arch over the gate (A)
    prism(rect(gx, gz - 0.2, 4.0, 0.4, 0.0), yg, yg + 5.0, IRON, "A")                       # the gate (A)
    prism(rect(gx - 1.0, gz - 1.0, 2.6, 2.2, 0.4), yg, yg + 1.3, DEBRIS, "B")              # the breach (B)

def z_in(z, a, b):
    return a <= z <= b

def ship(x, z, ang, length, world, sails=True):
    c, s_ = math.cos(ang), math.sin(ang)
    hull = rect(x, z, length, length * 0.28, ang)
    prism(hull, SEA_LEVEL - 0.6, SEA_LEVEL + 1.4, (0.36, 0.25, 0.17), world)
    if sails:
        for u in (-0.2, 0.2):
            mx, mz = x + c * u * length, z + s_ * u * length
            prism(ngon(mx, mz, 0.15, 4), SEA_LEVEL + 1.4, SEA_LEVEL + 1.4 + 0.75 * length, WOOD, world)
            poly([(mx - s_ * 0.1, SEA_LEVEL + 2.4, mz + c * 0.1), (mx - s_ * 0.1, SEA_LEVEL + 1.2 + 0.7 * length, mz + c * 0.1),
                  (mx + c * 0.28 * length, SEA_LEVEL + 2.4, mz + s_ * 0.28 * length)], (0.95, 0.92, 0.84), world)

def build_harbour():
    """Beat 0. The cove under the town wall: a quay, a mole with a lighthouse, the customs house, the harbour road that climbs
    west along the shore to the gate. A: the evening of the flight: the last ships out at sea under sail, boats at the quay,
    carts left on the road. B: the quay silted, the mole broken, a wreck on the beach, the lighthouse a stump."""
    q0, q1 = 24.0, 44.0
    zq = float(coast_z(np.array([36.0]))[0])
    prism([(q0, zq - 3.0), (q1, zq - 3.0), (q1, zq + 2.0), (q0, zq + 2.0)], SEA_LEVEL - 4.0, SEA_LEVEL + 1.3, STONE_DARK, "both", True, PAVING)
    mole = spline([(q1 + 1.0, zq - 1.0), (q1 + 6.0, zq - 9.0), (q1 + 2.0, zq - 18.0), (q0 + 2.0, zq - 23.0)], 1.0)
    d = lengths(mole)
    for i in range(len(mole) - 1):
        t = d[i] / d[-1]
        world = "A" if 0.55 < t < 0.68 else "both"
        a, b = mole[i], mole[i + 1]
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 1.6, v[0] * 1.6
        prism([(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)], SEA_LEVEL - 4.0,
              SEA_LEVEL + 1.6, STONE_DARK, world, True, PAVING)
    lx, lz = mole[-1]
    prism(ngon(lx, lz, 1.6, 8), SEA_LEVEL + 1.6, SEA_LEVEL + 13.0, STONE, "A")                   # the lighthouse (A) ...
    prism(ngon(lx, lz, 1.9, 8), SEA_LEVEL + 13.0, SEA_LEVEL + 14.5, GOLD, "A")
    cone(lx, lz, 2.0, SEA_LEVEL + 14.5, SEA_LEVEL + 16.5, ROOF, "A", 8)
    prism(ngon(lx, lz, 1.6, 8), SEA_LEVEL + 1.6, SEA_LEVEL + 6.0, STONE_B, "B")                  # ... a stump (B)
    cx, cz = 30.0, zq + 6.5                                                                      # the customs house
    y = float(ground(cx, cz))
    prism(rect(cx, cz, 8.0, 5.0, 0.0), y - 2.0, y + 6.0, (0.82, 0.75, 0.62))
    gable(cx, cz, 8.0, 5.0, 0.0, y + 6.0, 2.2, ROOF)
    for k, (bx, bz) in enumerate(((27.0, zq - 4.5), (33.0, zq - 5.0), (39.0, zq - 4.6))):    # boats at the quay (A)
        ship(bx, bz, 0.1 * k, 5.0, "A", sails=k == 1)
    ship(60.0, -150.0, -0.3, 16.0, "A")                                                         # the last ships, under sail
    ship(-30.0, -215.0, 0.2, 18.0, "A")
    ship(90.0, -260.0, -0.1, 14.0, "A")
    wx, wz = -18.0, float(coast_z(np.array([-18.0]))[0]) + 1.0                                  # B: a wreck on the beach
    prism(rect(wx, wz, 12.0, 3.4, 0.5), SEA_LEVEL - 0.8, SEA_LEVEL + 1.2, (0.30, 0.24, 0.20), "B")
    for u in (-4.0, -1.0, 2.0):
        prism(rect(wx + u * math.cos(0.5), wz + u * math.sin(0.5), 0.3, 3.6, 0.5), SEA_LEVEL + 1.2, SEA_LEVEL + 3.0, (0.30, 0.24, 0.20), "B")
    curve, width, _ = STREET["the harbour road"]                                                # carts and bundles left (A)
    for k, t in enumerate((0.18, 0.37, 0.61, 0.83)):
        p, v = at(curve, t)
        side = 1 if k % 2 else -1
        x, z = p[0] - v[1] * side * (width / 2 - 0.4), p[1] + v[0] * side * (width / 2 - 0.4)
        y = float(street_heights(curve)[int(t * (len(curve) - 1))])
        prism(rect(x, z, 2.2, 1.3, math.atan2(v[1], v[0]) + 0.2 * side), y, y + 0.9, WOOD, "A")
        prism(rect(x + 0.4, z, 0.8, 0.8, 0.3), y + 0.9, y + 1.4, (0.80, 0.72, 0.55), "A")
        prism(rect(x, z, 1.8, 1.0, math.atan2(v[1], v[0]) + 0.6), y, y + 0.35, (0.30, 0.24, 0.20), "B")
    curve, width, _ = STREET["the high street"]
    for k, t in enumerate((0.25, 0.55, 0.8)):
        p, v = at(curve, t)
        x, z = p[0] - v[1] * (width / 2 - 0.5), p[1] + v[0] * (width / 2 - 0.5)
        y = float(street_heights(curve)[int(t * (len(curve) - 1))])
        prism(rect(x, z, 0.7, 0.5, 0.4 * k), y, y + 0.5, (0.80, 0.72, 0.55), "A")

def build_prologue():
    build_harbour()
    # the barbican yard inside the gate, the half-fallen arch (beat 3)
    by = float(ground(1.0, -30.0))
    prism(rect(-4.0, -30.0, 1.4, 1.4, 0.1), by, by + 6.0, STONE)
    prism(rect(5.0, -29.0, 1.4, 1.4, 0.1), by, by + 6.0, STONE)
    beam((-4.0, by + 6.6, -30.0), (5.0, by + 6.6, -29.0), 1.4, 1.0, STONE, "A")
    beam((-4.0, by + 1.4, -31.2), (5.0, by + 0.6, -28.0), 1.6, 1.4, DEBRIS, "B")
    # the dry moat along the wall: drawn by the terrain under the wall (beat 4 at the gate)
    # the wall walk (beat 5): a wall 5 m high from the barbican to the courtyard, broken in the middle
    d = lengths(WALL_WALK)
    for i in range(len(WALL_WALK) - 1):
        t = d[i] / d[-1]
        a, b = WALL_WALK[i], WALL_WALK[i + 1]
        v = (b - a) / max(np.linalg.norm(b - a), 1e-9)
        nx, nz = -v[1] * 1.6, v[0] * 1.6
        out = [(a[0] + nx, a[1] + nz), (b[0] + nx, b[1] + nz), (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)]
        base = float(min(ground(a[0], a[1]), ground(b[0], b[1]))) - 2.0
        if WALK_GAP[0] <= t < WALK_GAP[1]:
            continue
        prism(out, base, WALK_Y, STONE_DARK, "both", True, PAVING)
    p0, _ = at(WALL_WALK, WALK_GAP[0])
    p1, _ = at(WALL_WALK, WALK_GAP[1])
    mid = 0.5 * (p0 + p1)
    beam((p0[0], WALK_Y, p0[1]), (mid[0], WALK_Y - 0.3, mid[1]), 2.4, 0.8, DEBRIS, "B")      # the beam of B
    beam(((mid[0] + p1[0]) / 2, WALK_Y, (mid[1] + p1[1]) / 2), (p1[0], WALK_Y, p1[1]), 3.0, 0.3, WOOD, "A")   # the hoarding of A
    for k in range(3):                                                                       # its posts (A)
        q = ((mid + p1) / 2) + (p1 - (mid + p1) / 2) * k / 2
        prism(ngon(q[0] + 1.3, q[1], 0.15, 4), float(ground(q[0], q[1])), WALK_Y, WOOD, "A")
    # the courtyard (beat 6)
    x0, z0, x1, z1 = COURTYARD
    for out in ([(x0, z0), (x1, z0), (x1, z0 + 0.6), (x0, z0 + 0.6)], [(x0, z1 - 0.6), (-0.8, z1 - 0.6), (-0.8, z1), (x0, z1)],
                [(0.8, z1 - 0.6), (x1, z1 - 0.6), (x1, z1), (0.8, z1)], [(x0, z0), (x0 + 0.6, z0), (x0 + 0.6, z1), (x0, z1)],
                [(x1 - 0.6, z0), (x1, z0), (x1, z1), (x1 - 0.6, z1)]):
        prism(out, -1.0, 4.0, STONE_DARK, "both", True, STONE)
    for bx, bz in ((-2.0, 15.0), (1.0, 17.5), (-0.5, 19.8)):                                # the walls of the puzzle (B)
        prism(rect(bx, bz, 1.0, 1.6, 0.0), 0.0, 2.5, DEBRIS, "B")
    for gx2, gz2 in ((-2.6, 14.0), (2.4, 20.0)):                                            # the grates over the cistern
        poly([(gx2 - 0.8, 0.03, gz2 - 0.8), (gx2 + 0.8, 0.03, gz2 - 0.8), (gx2 + 0.8, 0.03, gz2 + 0.8), (gx2 - 0.8, 0.03, gz2 + 0.8)], IRON)

def build_west():
    cx, cz = WELL
    ring_o = ngon(cx, cz, WELL_R + 1.0, 24)
    ring_i = ngon(cx, cz, WELL_R, 24)
    for i in range(24):                                                                     # the parapet of the well (a ring)
        j = (i + 1) % 24
        poly([(ring_o[i][0], 1.0, ring_o[i][1]), (ring_o[j][0], 1.0, ring_o[j][1]), (ring_i[j][0], 1.0, ring_i[j][1]),
              (ring_i[i][0], 1.0, ring_i[i][1])], STONE)
        poly([(ring_o[i][0], -0.2, ring_o[i][1]), (ring_o[j][0], -0.2, ring_o[j][1]), (ring_o[j][0], 1.0, ring_o[j][1]),
              (ring_o[i][0], 1.0, ring_o[i][1])], STONE_DARK)
    inner = ngon(cx, cz, WELL_R, 24)
    for i in range(24):                                                                     # the shaft
        (ax, az), (bx, bz) = inner[i], inner[(i + 1) % 24]
        poly([(ax, -15.5, az), (bx, -15.5, bz), (bx, 1.0, bz), (ax, 1.0, az)], STONE_DARK)
    poly([(x, -15.5, z) for x, z in ngon(cx, cz, WELL_R, 24)], PAVING)                    # the bottom
    poly([(x, -11.5, z) for x, z in ngon(cx, cz, WELL_R - 0.05, 24)], WATER, "A", "water")  # its water (A)
    n = 64                                                                                  # the spiral, 1.6 turns down
    for k in range(n):
        a0 = 2 * math.pi * 1.6 * k / n
        a1 = 2 * math.pi * 1.6 * (k + 1) / n
        y0 = -14.0 * k / n
        y1 = -14.0 * (k + 1) / n
        r0, r1 = WELL_R - 2.4, WELL_R
        pts = [(cx + r1 * math.cos(a0), y0, cz + r1 * math.sin(a0)), (cx + r1 * math.cos(a1), y1, cz + r1 * math.sin(a1)),
               (cx + r0 * math.cos(a1), y1, cz + r0 * math.sin(a1)), (cx + r0 * math.cos(a0), y0, cz + r0 * math.sin(a0))]
        world = "B" if 0.52 < k / n < 0.62 else "both"                                    # a part of A has fallen in B? (no:
        poly(pts, PAVING if world == "both" else DEBRIS, world)                            #  the floor of A held; B lost it)
        if 0.52 < k / n < 0.62:
            poly(pts, WOOD, "A")
    prism(ngon(cx + 0.6, cz - 0.8, 2.0, 12), -15.5, -13.0, GOLD, "B")                      # the day bell, fallen (B)
    tx, tz = -30.0, 46.0                                                                   # the clock tower of the market
    prism(rect(tx, tz, 5.0, 5.0, 0.15), 0.0, 18.0, STONE)
    gable(tx, tz, 5.0, 5.0, 0.15, 18.0, 3.0, ROOF, "A")
    prism(ngon(tx, tz, 1.5, 12), 14.5, 17.0, GOLD, "A")                                    # the day bell, hanging (A)
    prism(rect(tx, tz, 5.0, 5.0, 0.15), 18.0, 18.6, STONE, "A")
    prism(ngon(cx + 7.0, cz - 4.0, 0.25, 4), -12.0, 1.0, IRON, "A")                        # the bucket lift (A)
    curve, width, _ = STREET["the market street"]                                          # the stalls: awnings (A)
    for t in np.linspace(0.12, 0.88, 6):
        p, v = at(curve, t)
        side = 1 if int(t * 10) % 2 else -1
        nx, nz = -v[1] * side * (width / 2 + 0.6), v[0] * side * (width / 2 + 0.6)
        y = float(ground(p[0], p[1]))
        ang = math.atan2(v[1], v[0])
        prism(rect(p[0] + nx, p[1] + nz, 2.6, 1.6, ang), y, y + 1.0, WOOD)
        gable(p[0] + nx, p[1] + nz, 2.6, 2.4, ang + math.pi / 2, y + 2.6, 0.5, CLOTH if side > 0 else CLOTH2, "A")

def build_east():
    for i, (x0, x1, yq) in enumerate(LOCKS):                                              # the canal, lock by lock
        bed = yq - 2.6
        water = yq - 0.6 if i else yq - 1.0
        z0, z1 = 30.0 - CANAL_HALF, 30.0 + CANAL_HALF
        prism([(x0, z0), (x1, z0), (x1, z1), (x0, z1)], bed - 0.3, bed, PAVING)
        poly([(x0, water, z0), (x1, water, z0), (x1, water, z1), (x0, water, z1)], WATER, "A", "water")
        for zz in (z0, z1):                                                               # its walls
            poly([(x0, bed, zz), (x1, bed, zz), (x1, yq, zz), (x0, yq, zz)], STONE_DARK)
        poly([(x1, bed, z0), (x1, bed, z1), (x1, yq + 0.0, z1), (x0 * 0 + x1, yq, z0)], STONE_DARK)
        prism(rect(x1 - 0.2, 30.0, 0.4, 2 * CANAL_HALF, 0.0), bed, yq + 0.8, WOOD, "A")   # the lock gate (A)
        prism(rect(x1 - 0.2, 30.0, 0.5, 2 * CANAL_HALF, 0.0), bed, bed + 1.0, DEBRIS, "B")
        prism(ngon(x1 - 1.4, 30.0 - CANAL_HALF - 1.0, 0.4, 8), yq, yq + 1.2, GOLD, "A")   # its wheel
        if i == 1:
            prism(rect((x0 + x1) / 2, 30.0, 1.2, 1.2, 0.0), water - 0.4, water + 0.6, WOOD)   # the crate
    ribbon(np.array([[6.5, 34.0], [12.0, 32.0]]), np.array([-1.5, -1.5]), 2.6, WATER, "A")   # the canal pours into the pool
    tx, tz = 59.0, 22.0                                                                    # the water tower
    yt = 11.0
    prism(ngon(tx, tz, 2.8, 10), yt, yt + 16.0, STONE)
    prism(ngon(tx, tz, 3.6, 10), yt + 16.0, yt + 19.0, STONE, "A")
    prism(ngon(tx, tz, 1.5, 12), yt + 13.0, yt + 15.6, GOLD, "A")
    for k in range(5):
        a = 2 * math.pi * k / 5
        prism(rect(tx + 2.6 * math.cos(a), tz + 2.6 * math.sin(a), 1.2, 0.8, a), yt + 16.0, yt + 16.6 + k % 3, STONE_B, "B")
    hs = np.linspace(AQUEDUCT_Y[0], AQUEDUCT_Y[1], len(AQUEDUCT))                         # the aqueduct
    d = lengths(AQUEDUCT)
    for i in range(len(AQUEDUCT) - 1):
        t = d[i] / d[-1]
        world = "A" if AQUEDUCT_BREAK[0] <= t < AQUEDUCT_BREAK[1] else "both"
        a, b = AQUEDUCT[i], AQUEDUCT[i + 1]
        colour = WOOD if world == "A" else STONE
        beam((a[0], hs[i], a[1]), (b[0], hs[i + 1], b[1]), 1.6, 1.2, colour, world)
        poly([(a[0], hs[i] - 0.15, a[1]), (b[0], hs[i + 1] - 0.15, b[1]), (b[0] + 0.01, hs[i + 1] - 0.15, b[1] + 0.6), (a[0] + 0.01, hs[i] - 0.15, a[1] + 0.6)],
             WATER, "A", "water")
        if i % 3 == 0 and world == "both":
            y = float(ground(a[0], a[1]))
            if y < hs[i] - 2.5:
                prism(rect(a[0], a[1], 1.0, 1.2, math.atan2(b[1] - a[1], b[0] - a[0])), y - 1.0, hs[i] - 1.2, STONE)
    ax, az = AQUEDUCT[-1]                                                                  # its waterfall into the pool (A)
    poly([(ax - 0.5, AQUEDUCT_Y[1] - 1.0, az), (ax + 0.5, AQUEDUCT_Y[1] - 1.0, az), (ax + 0.5, -0.3, az - 1.0), (ax - 0.5, -0.3, az - 1.0)], WATER, "A", "water")
    curve, width, _ = STREET["the lock walk"]
    hw = np.array([lock_height(x) for x in curve[:, 0]])
    ribbon(curve, hw + 0.02, width, PAVING)

def build_north():
    cx, cz = THEATRE
    for k in range(8):                                                                     # the seats, rising north
        r0, r1 = 6.5 + 1.3 * k, 7.8 + 1.3 * k
        y = STAGE_Y + 0.65 * (k + 1)
        n = 14
        for j in range(n):
            a0 = math.pi * j / n
            a1 = math.pi * (j + 1) / n
            collapsed = j < 4 and k > 3                                                    # B: the west of the upper seats fell
            world = "A" if collapsed else "both"
            colour = (0.78, 0.70, 0.58)
            pts = [(cx + r0 * math.cos(a0), cz + r0 * math.sin(a0)), (cx + r1 * math.cos(a0), cz + r1 * math.sin(a0)),
                   (cx + r1 * math.cos(a1), cz + r1 * math.sin(a1)), (cx + r0 * math.cos(a1), cz + r0 * math.sin(a1))]
            prism(pts, y - 3.0, y, colour, world)
    prism(ngon(cx, cz, 6.5, 16), STAGE_Y - 1.0, STAGE_Y, PAVING)                          # the orchestra
    prism(rect(cx, cz - 8.5, 18.0, 4.0, 0.0), STAGE_Y - 1.0, STAGE_Y + 0.6, WOOD)         # the stage
    for dx in (-8.0, -4.0, 4.0, 8.0):                                                      # the stage front (scaena), low
        prism(rect(cx + dx, cz - 11.0, 2.4, 0.8, 0.0), STAGE_Y, STAGE_Y + 6.0, STONE, "A")
        if abs(dx) > 5.0:                                                                  # (the middle two: under the shaft)
            prism(rect(cx + dx, cz - 11.0, 2.4, 0.8, 0.0), STAGE_Y, STAGE_Y + 2.0 + abs(dx) / 4, STONE_B, "B")
    beam((cx - 6.0, STAGE_Y + 0.6, cz - 8.0), (cx - 9.0, STAGE_Y + 9.0, cz + 2.0), 2.0, 0.4, WOOD, "A")   # the painted stair (A)
    prism(rect(cx + 11.0, cz - 6.0, 4.0, 3.0, 0.2), STAGE_Y, STAGE_Y + 2.4, DEBRIS, "B")  # N2: the heap
    gy = 22.0                                                                              # the graveyard on the cliff
    for k in range(26):
        x = -14.0 + (k * 37 % 28)
        z = 85.0 + (k * 13 % 18)
        y = float(ground(x, z))
        prism(rect(x, z, 0.8, 0.35, (k % 5) * 0.12), y, y + 1.0 + (k % 3) * 0.2, STONE)
    for k in range(8):
        x, z = -18.0 + k * 5.0, 101.0 + (k % 2) * 2.0
        tree(x, z, float(ground(x, z)), cypress=True)
    nx_, nz_ = 6.0, 98.0                                                                   # the night bell tower
    y = float(ground(nx_, nz_))
    prism(rect(nx_, nz_, 4.0, 4.0, 0.1), y, y + 14.0, STONE)
    cone(nx_, nz_, 3.2, y + 14.0, y + 19.0, ROOF, "both", 4)
    prism(ngon(nx_, nz_, 1.3, 12), y + 10.5, y + 12.8, GOLD)
    prism(rect(10.0, 86.0, 1.0, 2.0, 0.2), gy, gy + 0.4, (0.40, 0.36, 0.30), "B")         # the new grave (B)
    prism(ngon(10.0, 85.0, 0.15, 6), gy + 0.4, gy + 1.2, GOLD, "B")
    curve, width, _ = STREET["the cliff path"]                                             # the stairs up the cliff
    ribbon(curve, street_heights(curve) + 0.04, width, PAVING, thickness=1.0)

def tree(x, z, y, cypress=False):
    if cypress:
        cone(x, z, 0.9, y, y + 7.0, TREE, "both", 6)
        cone(x, z, 0.9, y, y + 4.0, TREE_B, "B", 6)
    else:
        cone(x, z, 2.0, y + 1.5, y + 5.0, TREE, "A", 7)
        prism(ngon(x, z, 0.25, 4), y, y + 1.6, WOOD)
        cone(x, z, 1.2, y + 1.5, y + 3.0, TREE_B, "B", 5)

# ---- houses along the streets ---------------------------------------------------------------------------------------------
def inside_wall(x, z):
    poly2 = WALL
    c = False
    for i in range(len(poly2) - 1):
        (ax, az), (bx, bz) = poly2[i], poly2[i + 1]
        if (az > z) != (bz > z) and x < (bx - ax) * (z - az) / (bz - az) + ax:
            c = not c
    return c

# ---- the districts and the houses -----------------------------------------------------------------------------------------
# Every district has its own house: west ochre plaster under deep red tiles, east whitewash under light tiles, north grey
# stone under slate, south sandstone. The colour of a house is a blend of the four by distance, so a street that runs from
# one district to the next changes colour slowly: there is no border, only a gradient.
DISTRICTS = [
    # name, centre (x, z), wall colour, roof colour
    ("west", (-36.0, 34.0), (0.86, 0.70, 0.52), (0.70, 0.30, 0.20)),
    ("east", (38.0, 30.0), (0.91, 0.89, 0.83), (0.80, 0.47, 0.30)),
    ("north", (0.0, 64.0), (0.70, 0.68, 0.64), (0.42, 0.44, 0.49)),
    ("south", (0.0, -18.0), (0.82, 0.75, 0.62), (0.66, 0.33, 0.24)),
]

def district_weights(x, z):
    w = [math.exp(-((x - cx) ** 2 + (z - cz) ** 2) / (2 * 22.0 ** 2)) for _, (cx, cz), _, _ in DISTRICTS]
    s = sum(w) or 1.0
    return [v / s for v in w]

def district_colours(x, z, k):
    w = district_weights(x, z)
    wall = np.zeros(3)
    roof = np.zeros(3)
    for wi, (_, _, cw, cr) in zip(w, DISTRICTS):
        wall += wi * np.array(cw)
        roof += wi * np.array(cr)
    jitter = ((k * 29) % 9 - 4) * 0.012
    return tuple(np.clip(wall + jitter, 0, 1)), tuple(np.clip(roof - jitter, 0, 1))

KEEP_OUT = []   # (name, x, z, radius): circles the houses keep away from
HOUSES = []     # footprints (4 corners) of the houses placed so far

def rim_radius(x, z):
    """The radius of the rim of the square in the direction of (x, z)."""
    return SQUARE_R * (1.0 + 0.10 * math.sin(math.atan2(z - SQUARE[1], x - SQUARE[0]) * 3.0 + 0.7))

def overlaps(p, q, margin=0.03):
    """True when two convex quads overlap (separating axes), with a small margin so that row houses may touch."""
    for poly_ in (p, q):
        n = len(poly_)
        for i in range(n):
            ex, ez = poly_[(i + 1) % n][0] - poly_[i][0], poly_[(i + 1) % n][1] - poly_[i][1]
            nx, nz = -ez, ex
            a = [x * nx + z * nz for x, z in p]
            b = [x * nx + z * nz for x, z in q]
            length = math.hypot(nx, nz) or 1.0
            if max(a) <= min(b) + margin * length or max(b) <= min(a) + margin * length:
                return False
    return True

def site_free(outline, skip=()):
    """True when a house on `outline` (4 corners) stands clear of streets, landmarks, walls and other houses."""
    xs = [p[0] for p in outline]
    zs = [p[1] for p in outline]
    cx, cz = sum(xs) / 4, sum(zs) / 4
    pts = list(outline) + [((outline[i][0] + outline[(i + 1) % 4][0]) / 2, (outline[i][1] + outline[(i + 1) % 4][1]) / 2)
                           for i in range(4)] + [(cx, cz)]
    px = np.array([p[0] for p in pts])
    pz = np.array([p[1] for p in pts])
    for x, z in pts:
        if not inside_wall(x, z):
            return False
        if math.hypot(x - SQUARE[0], z - SQUARE[1]) < rim_radius(x, z) + 0.2:
            return False
        rt = math.hypot(x - THEATRE[0], z - THEATRE[1])
        if rt < 17.6 and z > THEATRE[1] - 12.5:
            return False                                                                   # the theatre and its forecourt
        if LOCKS[0][0] - 0.5 < x < LOCKS[-1][1] + 0.5 and abs(z - 30.0) < CANAL_HALF + 0.3:
            return False
        x0, z0, x1, z1 = COURTYARD
        if x0 - 1.0 < x < x1 + 1.0 and z0 - 1.0 < z < z1 + 1.0:
            return False
        for name, kx, kz, kr in KEEP_OUT:
            if name not in skip and math.hypot(x - kx, z - kz) < kr:
                return False
    for name, (curve, width, kind) in STREET.items():
        d, _ = distance_to(curve, px, pz)
        if d.min() < width / 2 + 0.15:
            return False
    if distance_to(WALL, px, pz)[0].min() < 2.2 or distance_to(WALL_WALK, px, pz)[0].min() < 2.3:
        return False
    shrunk = [(cx + (x - cx) * 0.98, cz + (z - cz) * 0.98) for x, z in outline]
    return not any(overlaps(shrunk, h) for h in HOUSES)

def house(cx, cz, w, dd, ang, k, height, ridge_along=True, arcade=False):
    """A house of w (along `ang`) x dd, its front on the side -dd/2 rotated by `ang` + 90 degrees. One in three is a ruin in B:
    no roof, walls broken to different heights, rubble and a tree inside. An arcade puts a portico in front of it."""
    out = rect(cx, cz, w, dd, ang)
    if not site_free(out):
        return False
    HOUSES.append(out)
    base = float(np.min(ground(np.array([p[0] for p in out] + [cx]), np.array([p[1] for p in out] + [cz]))))
    wall, roof = district_colours(cx, cz, k)
    top = base + height
    ruin = k % 3 == 0
    c, s = math.cos(ang), math.sin(ang)
    if ruin:
        prism(out, base - 1.0, top, wall, "A")
        heights = [0.45 + 0.1 * ((k * j * 7) % 5) for j in range(1, 5)]                  # B: four walls, broken
        t = 0.35
        for j in range(4):
            a, b = out[j], out[(j + 1) % 4]
            ex, ez = b[0] - a[0], b[1] - a[1]
            length = math.hypot(ex, ez) or 1.0
            nx, nz = -ez / length * t, ex / length * t
            inner = [a, b, (b[0] - nx, b[1] - nz), (a[0] - nx, a[1] - nz)]
            prism(inner, base - 1.0, base + height * heights[j], wall, "B")
            prism([inner[0], ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2), ((a[0] + b[0]) / 2 - nx, (a[1] + b[1]) / 2 - nz), inner[3]],
                  base + height * heights[j], base + height * min(0.95, heights[j] + 0.25), wall, "B")
        prism(rect(cx + 0.3, cz - 0.2, w * 0.45, dd * 0.4, ang + 0.3), base, base + 0.7, DEBRIS, "B")
        cone(cx - 0.4, cz + 0.3, 1.1, base + 0.4, base + 3.2, TREE_B, "B", 5)
    else:
        prism(out, base - 1.0, top, wall)
    rise = 0.35 * min(w, dd) + 0.4
    world = "A" if ruin else "both"
    if ridge_along:
        gable(cx, cz, dd, w, ang + math.pi / 2, top, rise, roof, world)
    else:
        gable(cx, cz, w, dd, ang, top, rise, roof, world)
    if k % 4 == 1:                                                                         # a chimney
        ox, oz = 0.25 * w * c, 0.25 * w * s
        prism(rect(cx + ox, cz + oz, 0.5, 0.5, ang), top, top + rise + 0.6, wall, world)
    if arcade:                                                                             # a portico on the street
        fx, fz = -s, c                                                                     # the front normal
        depth = 2.0
        front = [(cx - fx * dd / 2 + c * w / 2, cz - fz * dd / 2 + s * w / 2), (cx - fx * dd / 2 - c * w / 2, cz - fz * dd / 2 - s * w / 2)]
        outer = [(p[0] - fx * depth, p[1] - fz * depth) for p in front]
        prism([front[0], front[1], outer[1], outer[0]], base + 3.0, base + 3.4, wall, world, True, roof)
        n = max(2, int(w / 1.6))
        for j in range(n + 1):
            u = j / n
            px_ = outer[0][0] + (outer[1][0] - outer[0][0]) * u
            pz_ = outer[0][1] + (outer[1][1] - outer[0][1]) * u
            pillar_world = "A" if (ruin and j % 2) else "both"
            prism(ngon(px_ + fx * 0.2, pz_ + fz * 0.2, 0.18, 6), base, base + 3.0, STONE, pillar_world)
    return True

def street_rows():
    """Row houses along every street: a first row on the street, contiguous (they share walls); a second row behind it."""
    k = 0
    for name, (curve, width, kind) in STREET.items():
        if kind == "stair":
            continue
        d = lengths(curve)
        for side in (-1, 1):
            for row, offset in enumerate((0.0, 7.8)):
                s = 1.5
                while s < d[-1] - 1.5:
                    k += 1
                    w = 3.4 + (k * 7 % 5) * 0.45
                    dd = 5.0 + (k * 11 % 3) * 0.6
                    p, v = at(curve, min(1.0, (s + w / 2) / d[-1]))
                    n = np.array([-v[1], v[0]]) * side
                    ang = math.atan2(v[1], v[0])
                    if side > 0:
                        ang += math.pi                                                     # the front toward the street
                    west = district_weights(p[0], p[1])[0]
                    arcade = row == 0 and west > 0.55 and kind == "street"
                    setback = 2.0 if arcade else 0.0
                    c = p + n * (width / 2 + 0.2 + offset + setback + dd / 2)
                    h = 5.0 + (k * 17 % 4) * 1.1 + (2.5 if math.hypot(c[0] - SQUARE[0], c[1] - SQUARE[1]) < 32 else 0.0)
                    placed = (row == 0 or (k * 13 % 10) < 8) and house(c[0], c[1], w, dd, ang, k, h, ridge_along=(k % 3 != 1), arcade=arcade)
                    s += w if placed else 1.0
    return k

def ring(cx, cz, radius_of, dd, height_of, start_k, sectors=((0.0, 2 * math.pi),), arcade_of=lambda a: False):
    """Houses round a round place, their fronts on it: the square, the well, the theatre."""
    k = start_k
    for a0, a1 in sectors:
        a = a0
        while a < a1:
            k += 1
            r = radius_of(a) + dd / 2
            w = 3.2 + (k * 7 % 4) * 0.4
            ac = arcade_of(a)
            rr = r + (2.0 if ac else 0.0)
            x, z = cx + rr * math.cos(a), cz + rr * math.sin(a)
            ang = a - math.pi / 2                                                          # along the ring, front inward
            placed = house(x, z, w, dd, ang, k, height_of(k), ridge_along=True, arcade=ac)
            a += (w if placed else 0.4) / rr
    return k

def canal_row(k):
    """The houses on the south bank of the locks: their back walls are the wall of the canal."""
    x = LOCKS[0][0] + 2.0
    while x < LOCKS[-1][1] - 2.0:
        k += 1
        w = 3.6 + (k * 7 % 4) * 0.5
        dd = 5.2
        z = 30.0 - CANAL_HALF - 0.35 - dd / 2
        if house(x + w / 2, z, w, dd, 0.0, k, 5.0 + (k * 5 % 3) * 1.2, ridge_along=True):
            x += w
        else:
            x += 1.0
    return k

def build_houses():
    # the fronts of the places first: they make the places; the streets fill in behind them
    k = ring(SQUARE[0], SQUARE[1], lambda a: SQUARE_R * (1.0 + 0.10 * math.sin(a * 3.0 + 0.7)) * 1.03 + 0.9, 6.0,
             lambda k: 8.0 + (k * 7 % 3) * 1.4, 0, arcade_of=lambda a: math.cos(a) < -0.35)       # the square: tall, arcaded west
    k = ring(WELL[0], WELL[1], lambda a: WELL_R + 2.4, 5.4, lambda k: 6.0 + (k * 5 % 3) * 1.5, k)  # round the well
    k = ring(THEATRE[0], THEATRE[1], lambda a: 18.2, 5.0, lambda k: 5.0 + (k * 3 % 3), k,
             sectors=((-0.25, 0.55), (math.pi - 0.55, math.pi + 0.25)))                         # beside the theatre
    k = canal_row(k)
    k += street_rows()
    for gx in np.arange(-58.0, 70.0, 5.5):                                                     # the blocks between
        for gz in np.arange(-36.0, 74.0, 5.5):
            k += 1
            x = gx + ((k * 37) % 10) / 10.0 * 1.5
            z = gz + ((k * 53) % 10) / 10.0 * 1.5
            best, ang = 1e9, 0.0
            for name, (curve, width, kind) in STREET.items():
                dist, i = distance_to(curve, x, z)
                if dist < best:
                    i = int(min(i, len(curve) - 2))
                    v = curve[i + 1] - curve[i]
                    best, ang = dist, math.atan2(v[1], v[0])
            house(x, z, 3.6 + (k * 7 % 5) * 0.4, 4.6 + (k * 11 % 3) * 0.5, ang, k, 4.5 + (k * 17 % 4) * 1.0, ridge_along=(k % 2 == 0))
    plant_town()
    plant_country()

# ---- plants: where they grow, and why ---------------------------------------------------------------------------------------
OLIVE = (0.55, 0.60, 0.42)
PINE = (0.22, 0.36, 0.24)
LEMON = (0.26, 0.45, 0.22)
SHRUB = (0.42, 0.48, 0.34)
REED = (0.58, 0.62, 0.38)
TERRACE_WALL = (0.70, 0.65, 0.55)

def olive(x, z, y):
    prism(ngon(x, z, 0.18, 4), y, y + 1.2, WOOD)
    cone(x, z, 1.7, y + 0.9, y + 2.6, OLIVE, "both", 7)

def umbrella_pine(x, z, y, k):
    h = 7.0 + (k % 3)
    prism(ngon(x, z, 0.22, 5), y, y + h, WOOD)
    cone(x, z, 3.2, y + h - 0.6, y + h + 1.0, PINE, "both", 9)

def lemon(x, z, y):
    prism(ngon(x, z, 0.12, 4), y, y + 0.9, WOOD)
    cone(x, z, 1.0, y + 0.6, y + 2.2, LEMON, "A", 6)
    cone(x, z, 0.7, y + 0.6, y + 1.4, TREE_B, "B", 5)

def shrub(x, z, y):
    cone(x, z, 0.9, y - 0.1, y + 0.8, SHRUB, "both", 5)

def slope_at(x, z):
    e = 1.0
    gx = float(ground(x + e, z) - ground(x - e, z)) / (2 * e)
    gz = float(ground(x, z + e) - ground(x, z - e)) / (2 * e)
    return math.hypot(gx, gz), gx, gz

def near_way(x, z, extra):
    for name, (curve, width, kind) in STREET.items():
        if distance_to(curve, x, z)[0] < width / 2 + extra:
            return True
    return distance_to(WALL, x, z)[0] < 3.0 + extra

def plant_town():
    """Inside the walls trees grow only where people let them: a lemon tree in a yard behind the houses (never on a street),
    plane trees round the square (build_square), a fig in a ruined house in B (house())."""
    k = 0
    for gx in np.arange(-56.0, 70.0, 4.0):
        for gz in np.arange(-34.0, 76.0, 4.0):
            k += 1
            x = gx + ((k * 37) % 10) * 0.15
            z = gz + ((k * 53) % 10) * 0.15
            if not site_free(rect(x, z, 2.0, 2.0, 0.0)) or near_way(x, z, 2.5):
                continue
            behind = sum(1 for h in HOUSES if min(math.hypot(x - px, z - pz) for px, pz in h) < 4.0)
            if behind >= 2:                                                                 # a yard between houses
                lemon(x, z, float(ground(x, z)))
                HOUSES.append(rect(x, z, 1.6, 1.6, 0.0))

def plant_country():
    """Outside the walls: olive groves in rows along the contours of the gentle slopes that face the sun (south to east), each
    row on a low terrace wall; umbrella pines in clumps on the headland and the cliff top, where the wind is; cypresses in
    lines along the road to the graveyard and round it; low maquis on the steep slopes; nothing on rock, sand or the beach."""
    k = 0
    for gx in np.arange(X0 + 4.0, X1 - 4.0, 3.2):
        for gz in np.arange(-60.0, 150.0, 3.2):
            k += 1
            x = gx + ((k * 37) % 10) * 0.12
            z = gz + ((k * 53) % 10) * 0.12
            if inside_wall(x, z) or distance_to(WALL, x, z)[0] < 6.0 or near_way(x, z, 2.0):
                continue
            y = float(ground(x, z))
            if y < SEA_LEVEL + 1.6 or z < float(coast_z(x)) + 3.0:
                continue
            slope, sx, sz = slope_at(x, z)
            if slope > 0.95:
                continue                                                                    # rock
            sunny = sz > 0.02 or sx < -0.04                                                 # falls to the south or the east
            wind = float(fbm(x / 18.0, z / 18.0, 2, 41))
            if (x < -38.0 and z < -20.0) or (z > 74.0 and y > 18.0 and z < 112.0):
                if wind > 0.18 and k % 2 == 0:                                              # pines, in clumps
                    umbrella_pine(x, z, y, k)
                elif slope > 0.45 and k % 3 == 0:
                    shrub(x, z, y)
                continue
            if 0.06 < slope < 0.42 and sunny and z < 70.0:
                band = (y + 40.0) % 2.6
                if band < 0.32 and k % 2 == 0:                                              # a row of olives on its terrace
                    olive(x, z, y)
                    nx, nz = -sz / max(slope, 1e-6), sx / max(slope, 1e-6)
                    prism(rect(x, z, 3.0, 0.4, math.atan2(nz, nx)), y - 0.6, y + 0.15, TERRACE_WALL)
                continue
            if slope > 0.42 and k % 4 == 0:
                shrub(x, z, y)
            elif wind > 0.45 and k % 5 == 0:
                tree(x, z, y)
    for name in ("the cliff path", "the harbour road"):                                     # cypress avenues
        curve, width, kind = STREET[name]
        d = lengths(curve)
        s = 3.0
        while s < d[-1]:
            p, v = at(curve, s / d[-1])
            for side in (-1, 1):
                x, z = p[0] - v[1] * side * (width / 2 + 1.4), p[1] + v[0] * side * (width / 2 + 1.4)
                if not inside_wall(x, z) and distance_to(WALL, x, z)[0] > 2.5 and float(ground(x, z)) > SEA_LEVEL + 1.0:
                    tree(x, z, float(ground(x, z)), cypress=True)
            s += 7.0
    for k in range(10):                                                                     # round the graveyard
        a = 2 * math.pi * k / 10
        x, z = 2.0 + 15.0 * math.cos(a), 94.0 + 9.0 * math.sin(a)
        tree(x, z, float(ground(x, z)), cypress=True)

def build_streets():
    for name, (curve, width, kind) in STREET.items():
        if name in ("the lock walk", "the cliff path"):
            continue
        h = street_heights(curve)
        ribbon(curve, h + 0.04, width, PAVING)

# ---- the terrain as polygons ----------------------------------------------------------------------------------------------
X0, X1, Z0, Z1, STEP = -150.0, 160.0, -100.0, 190.0, 2.5
GX = np.arange(X0, X1 + 0.1, STEP)
GZ = np.arange(Z0, Z1 + 0.1, STEP)
MX, MZ = np.meshgrid(GX, GZ)
HEIGHT = ground(MX, MZ)

def build_terrain():
    inside = np.vectorize(inside_wall)(MX, MZ)
    for j in range(len(GZ) - 1):
        for i in range(len(GX) - 1):
            ys = (HEIGHT[j, i], HEIGHT[j, i + 1], HEIGHT[j + 1, i + 1], HEIGHT[j + 1, i])
            cx, cz = GX[i] + STEP / 2, GZ[j] + STEP / 2
            if math.hypot(cx - WELL[0], cz - WELL[1]) < WELL_R:
                continue                                                                  # the well is open
            if math.hypot(cx - TOWER[0], cz - TOWER[1]) < POOL_R0 - 0.5:
                continue
            if LOCKS[0][0] < cx < LOCKS[-1][1] and abs(cz - 30.0) < CANAL_HALF:
                continue                                                                  # the canal is open
            slope = (max(ys) - min(ys)) / STEP
            if max(ys) < SEA_LEVEL - 0.3:
                continue                                                                  # under the sea
            if inside[j, i]:
                colour = PAVING if math.hypot(cx - SQUARE[0], cz - SQUARE[1]) < SQUARE_R + 1 else (0.60, 0.62, 0.43)
            else:
                dry = float(fbm(cx / 30.0, cz / 30.0, 2, 21))
                colour = SAND if min(ys) < SEA_LEVEL + 1.0 else ((0.66, 0.64, 0.44) if dry > 0.15 else GRASS)
                if cz > 120.0 and max(ys) > 40.0:
                    colour = (0.62, 0.60, 0.56)                                           # bare stone high on the mountain
            if slope > 0.95:
                colour = ROCK
            pts = [(GX[i], ys[0], GZ[j]), (GX[i + 1], ys[1], GZ[j]), (GX[i + 1], ys[2], GZ[j + 1]), (GX[i], ys[3], GZ[j + 1])]
            POLYS.append((np.array(pts), colour, "both", "terrain"))
    # the sea: one plane to the horizon (in the game it follows the camera and fades into the sky at the fog distance)
    poly([(-900.0, SEA_LEVEL, -1200.0), (900.0, SEA_LEVEL, -1200.0), (900.0, SEA_LEVEL, -20.0), (-900.0, SEA_LEVEL, -20.0)], SEA, "both", "water")
    for x, z, w, d, h in ((-150.0, -260.0, 60.0, 28.0, 14.0), (140.0, -330.0, 90.0, 40.0, 22.0), (40.0, -520.0, 140.0, 50.0, 30.0)):
        cone(x, z, w * 0.5, SEA_LEVEL - 1.0, SEA_LEVEL + h, (0.56, 0.60, 0.52), "both", 9)   # islands, far out

# ---- the memory lights and the anchor lamps -------------------------------------------------------------------------------
def on_ground(x, z, dy=0.6):
    return (x, float(ground(x, z)) + dy, z)

def build_lists():
    walk_gap, _ = at(WALL_WALK, 0.5 * (WALK_GAP[0] + WALK_GAP[1]))
    awn, v = at(STREET["the market street"][0], 0.55)
    lights = [
        # number, name, (x, y, z), district, taken in, visit, lesson, fragment that appears in B
        (1, "Inside the gate", (0.0, float(ground(0.0, -37.0)) + 4.4, -37.4), "prologue", "B", "first",
         "the breach is the way: a full jump under the arch", "the stair of the gate tower: a view of the bay"),
        (2, "Under the hoarding", (walk_gap[0] + 1.6, WALK_Y - 1.4, walk_gap[1]), "prologue", "A", "first",
         "under a floor of A, a ledge that the planks hide", "a ladder from the lower lane up to the wall walk"),
        (3, "The floor of the pool", (7.2, -0.9, 37.8), "square", "B", "first",
         "a water of A is a place in B", "a bridge from the rim to the plinth of the tower"),
        (4, "Under the courtyard", (0.0, -6.4, 17.0), "cistern", "B", "first",
         "the first puzzle, from below", "a ladder up through a grate of the courtyard"),
        (5, "The high conduit", (-6.0, -2.4, 22.5), "cistern", "A", "after the dawn bell",
         "the crate floats as high as the water", "a footbridge over the cistern"),
        (6, "On the awnings", (awn[0] - v[1] * 3.2, float(ground(awn[0], awn[1])) + 3.6, awn[1] + v[0] * 3.2), "west", "A", "first",
         "awnings of A, bare frames in B, a switch in the air", "a roof bridge from the market to the clock tower"),
        (7, "In the wall of the well", (WELL[0] - WELL_R + 0.4, -10.0, WELL[1] + 1.0), "west", "B", "first",
         "the orb dropped onto the other socket", "the chain of the bucket, a ladder up the shaft"),
        (8, "In the clockwork", (-30.0, 12.5, 46.0), "west", "A", "after the day bell",
         "machinery runs only in A", "the stair of the clock tower"),
        (9, "The bed of the canal", (18.0, -3.4, 31.4), "east", "B", "first",
         "the lesson of the pool, again", "a footbridge over the canal"),
        (10, "The bottom of the fourth lock", (49.0, 5.8, 28.8), "east", "B", "first",
         "a lock is a room when it is dry", "the fallen part of the aqueduct"),
        (11, "Under the stage", (9.0, STAGE_Y - 0.2, 47.6), "north", "B", "first",
         "one prism, two things", "the upper seats of the theatre, back in B"),
        (12, "The keeper's lantern", on_ground(10.0, 86.0, 1.2), "north", "B", "after the night bell",
         "a light that shines only after dark", "the stair of the night bell tower"),
    ]
    anchors = [
        ("the quay", on_ground(28.0, -38.0, 0.0)), ("the headland", on_ground(-40.0, -56.0, 0.0)),
        ("the gate", on_ground(4.0, -31.0, 0.0)), ("the wall walk", (-4.6, WALK_Y, -12.0)),
        ("the square", (-12.0, 0.0, 24.0)), ("the market", on_ground(-22.0, 32.0, 0.0)),
        ("the well bottom", (WELL[0] + 4.0, -15.5, WELL[1] + 4.0)), ("the canal", (18.0, -1.5, 35.0)),
        ("the water tower", (56.0, 11.0, 25.0)), ("the stage", (-6.0, STAGE_Y + 0.6, 49.0)),
        ("the graveyard", on_ground(-8.0, 84.0, 0.0)), ("the nave", (3.0, 0.4, 31.0)),
    ]
    path = [
        ("0 the quay", 34.0, None, -40.0), ("1 lamp", 14.0, None, -44.0), ("", -4.0, None, -45.0), ("2 gate", 0.0, None, -37.0),
        ("3 arch", 1.0, None, -30.0), ("4 moat", -2.0, None, -27.0), ("5 wall walk", -5.0, WALK_Y, -12.0), ("gap", -3.6, WALK_Y, -3.0),
        ("", 0.0, WALK_Y, 10.0), ("6 courtyard", 0.0, 0.0, 17.0), ("square", 0.0, 0.0, 24.0), ("E1 canal", 16.0, -1.5, 33.0),
        ("E3 locks", 40.0, 4.5, 27.0), ("E4 dawn bell", 59.0, 25.0, 22.0), ("hill lane", 42.0, None, 50.0),
        ("top of the seats", 14.0, None, 66.0), ("graveyard", 12.0, None, 86.0), ("night bell", 6.0, None, 98.0),
        ("back down", 14.0, None, 66.0), ("in the fallen shaft (B)", 1.2, 1.6, 52.0), ("the nave", 0.0, 0.4, 37.0),
        ("T3 stump", 0.0, 30.0, 38.0), ("T5 belfry", 0.0, 46.0, 34.0),
    ]
    path = [(n, x, float(ground(x, z)) if y is None else y, z) for n, x, y, z in path]
    return lights, anchors, path

# ---- the renderer ---------------------------------------------------------------------------------------------------------
def render(world, width, height, basis, centre, scale, sky, fog):
    """An orthographic z-buffer renderer. basis = (right, up, forward) unit vectors. Returns the image, the depth and a
    function that projects (x, y, z) to pixels."""
    right, up, fwd = (np.asarray(b, float) for b in basis)
    sun = np.array([-0.45, 0.80, -0.38])
    sun /= np.linalg.norm(sun)
    c = np.asarray(centre, float)
    img = np.ones((height, width, 3)) * np.asarray(sky)
    depth = np.full((height, width), np.inf)
    ids = np.full((height, width), -1, dtype=np.int64)

    def project(p):
        q = np.asarray(p, float) - c
        return width / 2 + scale * q.dot(right), height / 2 - scale * q.dot(up), q.dot(fwd)

    items = []
    for k, (pts, colour, w, kind) in enumerate(POLYS):
        if w not in ("both", world):
            continue
        items.append((k, pts, colour, kind))
    items.sort(key=lambda it: it[3] == "water")                                          # water last: it blends
    dmin, dmax = None, None
    for k, pts, colour, kind in items:
        if len(pts) < 3:
            continue
        n = np.cross(pts[1] - pts[0], pts[2] - pts[0])
        if np.linalg.norm(n) < 1e-9 and len(pts) > 3:
            n = np.cross(pts[2] - pts[0], pts[3] - pts[0])
        if np.linalg.norm(n) < 1e-9:
            continue
        n = n / np.linalg.norm(n)
        if n.dot(fwd) > 0:
            n = -n
        col = np.asarray(colour, float)
        if world == "B":
            col = np.asarray(tint_b(colour), float)
        light = 0.50 + 0.50 * max(0.0, n.dot(sun)) if world == "A" else 0.62 + 0.30 * max(0.0, n.dot(sun))
        shade = col * light if kind != "water" else col
        sp = np.array([project(p) for p in pts])
        for i in range(1, len(sp) - 1):
            tri = sp[[0, i, i + 1]]
            x0, x1 = max(int(tri[:, 0].min()), 0), min(int(tri[:, 0].max()) + 1, width - 1)
            y0, y1 = max(int(tri[:, 1].min()), 0), min(int(tri[:, 1].max()) + 1, height - 1)
            if x0 > x1 or y0 > y1:
                continue
            gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            (ax, ay, az), (bx, by, bz), (cx2, cy2, cz2) = tri
            den = (by - cy2) * (ax - cx2) + (cx2 - bx) * (ay - cy2)
            if abs(den) < 1e-9:
                continue
            w0 = ((by - cy2) * (gx - cx2) + (cx2 - bx) * (gy - cy2)) / den
            w1 = ((cy2 - ay) * (gx - cx2) + (ax - cx2) * (gy - cy2)) / den
            w2 = 1 - w0 - w1
            inside = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
            zb = w0 * az + w1 * bz + w2 * cz2
            sub = depth[y0:y1 + 1, x0:x1 + 1]
            win = inside & (zb < sub - (0.05 if kind == "water" else 0.0))
            region = img[y0:y1 + 1, x0:x1 + 1]
            if kind == "water":
                region[win] = 0.35 * region[win] + 0.65 * shade
                continue
            sub[win] = zb[win]
            region[win] = shade
            ids[y0:y1 + 1, x0:x1 + 1][win] = -2 if kind == "terrain" else k
    # fog with depth, then edges where a solid meets something else
    finite = np.isfinite(depth)
    if finite.any():
        d0, d1 = np.percentile(depth[finite], 2), np.percentile(depth[finite], 98)
        f = np.clip((depth - d0) / max(d1 - d0, 1e-6), 0, 1) ** 1.5 * fog
        f[~finite] = 0
        img = img * (1 - f[..., None]) + np.asarray(sky) * f[..., None]
    edge = np.zeros_like(ids, dtype=bool)
    edge[:, 1:] |= (ids[:, 1:] != ids[:, :-1]) & ((ids[:, 1:] >= 0) | (ids[:, :-1] >= 0))
    edge[1:, :] |= (ids[1:, :] != ids[:-1, :]) & ((ids[1:, :] >= 0) | (ids[:-1, :] >= 0))
    img[edge] *= 0.72
    return np.clip(img, 0, 1), depth, project

KEEP_COLOURS = {GOLD, WATER, SEA, MEMORY, DEBRIS, IRON, WOOD, CLOTH, CLOTH2, TREE_B, (0.40, 0.36, 0.30), (0.32, 0.32, 0.33),
                (0.15, 0.15, 0.16), STONE_B}

def tint_b(c):
    """World B: the same stone, wet and grey: every colour loses most of its hue and some of its light."""
    table = {GRASS: GRASS_B, TREE: TREE_B, SAND: (0.55, 0.53, 0.46), (0.60, 0.62, 0.43): (0.38, 0.43, 0.34)}
    c = tuple(float(v) for v in c)
    if c in table:
        return table[c]
    if c in KEEP_COLOURS:
        return c
    g = 0.30 * c[0] + 0.59 * c[1] + 0.11 * c[2]
    return tuple(0.74 * (0.68 * g + 0.32 * v) for v in c)

def view_basis(elev, azim):
    e, a = math.radians(elev), math.radians(azim)
    fwd = np.array([math.cos(e) * math.sin(a), -math.sin(e), math.cos(e) * math.cos(a)])
    right = np.cross([0.0, 1.0, 0.0], fwd)
    right /= np.linalg.norm(right)
    up = np.cross(fwd, right)
    return right, up, fwd

def draw_world(world, path, lights):
    sky = (0.86, 0.84, 0.82) if world == "A" else (0.60, 0.63, 0.68)
    img, depth, project = render(world, 1500, 1050, view_basis(30.0, -32.0), (4.0, 6.0, 30.0), 6.6, sky,
                                 0.45 if world == "A" else 0.75)
    fig = plt.figure(figsize=(15, 10.5))
    ax = fig.add_axes([0, 0, 1, 0.96])
    ax.imshow(img)
    for n, name, (x, y, z), d, layer, *_ in lights:
        px, py, pz = project((x, y, z))
        if not (0 <= px < img.shape[1] and 0 <= py < img.shape[0]):
            continue
        hidden = depth[int(py), int(px)] < pz - 1.0
        here = layer == world
        ax.plot(px, py, marker="*", ms=17 if here else 9, color=(0.2, 0.95, 1.0) if here else (0.8, 0.8, 0.8), mec="k",
                alpha=0.5 if hidden else 1.0)
        ax.text(px + 8, py - 8, str(n), fontsize=11 if here else 8, weight="bold",
                bbox=dict(boxstyle="round,pad=0.1", fc="white", alpha=0.7 if here else 0.35))
    ax.set_axis_off()
    title = {"A": "World A - 7:11, the whole city at dusk", "B": "World B - after the storm"}[world]
    ax.set_title(title + "   (model, from over the sea, south; star = memory light, cyan = taken in this world)",
                 fontsize=11)
    fig.savefig(path, dpi=80)
    plt.close(fig)

def draw_aerial(world, path):
    sky = (0.86, 0.84, 0.82) if world == "A" else (0.60, 0.63, 0.68)
    img, _, _ = render(world, 1500, 1000, view_basis(42.0, -75.0), (6.0, 8.0, 40.0), 7.0, sky, 0.35 if world == "A" else 0.7)
    plt.imsave(path, img)

def draw_plan(path, lights, anchors, main_path):
    W, H, scale, centre = 1300, 1480, 6.0, (4.0, 0.0, 16.0)
    basis = (np.array([1.0, 0, 0]), np.array([0, 0, 1.0]), np.array([0, -1.0, 0]))
    img, _, project = render("A", W, H, basis, centre, scale, (0.85, 0.85, 0.85), 0.0)
    fig = plt.figure(figsize=(13, 14.8))
    ax = fig.add_axes([0, 0, 1, 0.97])
    ax.imshow(img)
    px = (GX - centre[0]) * scale + W / 2
    pz = H / 2 - (GZ - centre[2]) * scale
    cs = ax.contour(px, pz, HEIGHT, levels=np.arange(-14, 40, 2), colors="k", linewidths=0.35, alpha=0.35)
    ax.clabel(cs, levels=np.arange(-10, 40, 10), fontsize=7, fmt="%d m")
    for n, name, (x, y, z), d, layer, visit, *_ in lights:
        qx, qy, _ = project((x, y, z))
        ax.plot(qx, qy, marker="*", ms=20, color=(0.2, 0.95, 1.0), mec="k")
        ax.text(qx + 10, qy - 10, "%d %s (%s%s)" % (n, name, layer, "" if visit == "first" else ", " + visit), fontsize=8,
                weight="bold", bbox=dict(boxstyle="round,pad=0.15", fc="white", alpha=0.85))
    for name, p in anchors:
        qx, qy, _ = project(p)
        ax.plot(qx, qy, marker="^", ms=10, color=(1.0, 0.72, 0.15), mec="k")
    q = np.array([project((x, y, z))[:2] for _, x, y, z in main_path])
    ax.plot(q[:, 0], q[:, 1], color=(0.85, 0.1, 0.5), lw=2.2, ls="--", alpha=0.85, label="a main path (one order of the bells)")
    for label, x, z in (("PROLOGUE: the harbour, the gate", 8, -58), ("THE SQUARE", 14, 46), ("WEST: the sinking market", -66, 18),
                        ("EAST: the stepped locks", 26, 14), ("NORTH: theatre", -26, 74), ("the graveyard", -30, 106),
                        ("the sea", -40, -70), ("cistern (under)", -14, 6)):
        qx, qy, _ = project((x, 0.0, z))
        ax.text(qx, qy, label, fontsize=12, weight="bold", color=(0.1, 0.1, 0.1),
                bbox=dict(boxstyle="round,pad=0.2", fc=(1, 1, 1), alpha=0.55, ec="none"))
    cis = [project(p) for p in ((-12, 0, 2), (12, 0, 2), (12, 0, 24), (-12, 0, 24), (-12, 0, 2))]
    ax.plot([c[0] for c in cis], [c[1] for c in cis], color=(0.1, 0.3, 0.9), lw=1.5, ls=":")
    ax.legend(loc="lower right", fontsize=10)
    ax.set_xlim(0, W)
    ax.set_ylim(H, 0)
    ax.set_axis_off()
    ax.set_title("Plan (world A from above): contour lines every 2 m; star = memory light, triangle = anchor lamp; "
                 "dotted blue = the cistern under the courtyard", fontsize=11)
    fig.savefig(path, dpi=70)
    plt.close(fig)

def draw_section(path, main_path):
    fig, ax = plt.subplots(figsize=(15, 5))
    dist = [0.0]
    for a, b in zip(main_path, main_path[1:]):
        dist.append(dist[-1] + math.hypot(b[1] - a[1], b[3] - a[3]))
    ys = [p[2] for p in main_path]
    ax.plot(dist, ys, color=(0.2, 0.2, 0.2), lw=2)
    ax.fill_between(dist, -18, ys, color=(0.85, 0.80, 0.70), alpha=0.5)
    for d, (label, *_), y in zip(dist, main_path, ys):
        if label:
            ax.plot(d, y, "o", color=(0.85, 0.1, 0.5))
            ax.annotate(label, (d, y), textcoords="offset points", xytext=(0, 8), ha="center", fontsize=8, rotation=35)
    ax.axhline(0, color="k", lw=0.5, ls=":")
    ax.set_xlabel("distance along a main path (m)")
    ax.set_ylabel("height (m)")
    ax.set_title("Section along a main path: down from the ridge, along the wall walk, up the locks, over the hill to the cliff, "
                 "back through the fallen shaft into the tower, up to the belfry")
    ax.set_ylim(-18, 60)
    ax.grid(alpha=0.25)
    fig.tight_layout()
    fig.savefig(path, dpi=80)
    plt.close(fig)

# ---- checks on the model --------------------------------------------------------------------------------------------------
def check(lights, anchors):
    problems = []
    for name, (curve, width, kind) in STREET.items():
        d = lengths(curve)
        h = street_heights(curve) if name != "the lock walk" else np.array([lock_height(x) for x in curve[:, 0]])
        if name in ("the lock walk",):
            continue                                                                         # (stairs at the lock gates)
        g = np.degrees(np.arctan2(np.abs(np.diff(h)), np.maximum(np.diff(d), 1e-6)))
        limit = 32.0 if kind == "stair" else 27.0
        if g.max() > limit + 0.5:
            problems.append("%s too steep: %.0f degrees" % (name, g.max()))
        run, longest = 0.0, 0.0                                                              # no long straight way
        for i in range(1, len(curve) - 1):
            a = curve[i] - curve[i - 1]
            b = curve[i + 1] - curve[i]
            turn = abs(math.degrees(math.atan2(a[0] * b[1] - a[1] * b[0], a.dot(b))))
            run = 0.0 if turn > 1.2 else run + np.linalg.norm(b)
            longest = max(longest, run)
        if longest > 15.0:
            problems.append("%s runs straight for %.0f m" % (name, longest))
    if len(lights) != 12:
        problems.append("%d memory lights, not 12" % len(lights))
    for district in ("prologue", "square", "cistern", "west", "east", "north"):
        if not any(light[3] == district for light in lights):
            problems.append("no memory light in " + district)
    later = [light[5] for light in lights if light[5] != "first"]
    if sorted(later) != sorted(["after the dawn bell", "after the day bell", "after the night bell"]):
        problems.append("one light after each bell expected")
    if len(anchors) != 12:
        problems.append("%d anchor lamps, not 12" % len(anchors))
    return problems

def main():
    os.makedirs(OUT, exist_ok=True)
    build_terrain()
    build_streets()
    build_square()
    build_tower()
    build_wall()
    build_prologue()
    build_west()
    build_east()
    build_north()
    KEEP_OUT.extend([("well", WELL[0], WELL[1], WELL_R + 2.3), ("clock tower", -30.0, 46.0, 3.6), ("water tower", 59.0, 22.0, 4.0),
                     ("night bell", 6.0, 98.0, 4.0), ("customs house", 30.0, -33.0, 6.0), ("barbican", 1.0, -29.5, 6.5),
                     ("fountain stair", 9.0, 33.0, 3.0)])
    build_houses()
    lights, anchors, main_path = build_lists()
    problems = check(lights, anchors)
    for p in problems:
        print("PROBLEM:", p)
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    if only in ("", "a"):
        draw_world("A", os.path.join(OUT, "city_a.png"), lights)
    if only in ("", "b"):
        draw_world("B", os.path.join(OUT, "city_b.png"), lights)
    if only in ("", "aerial"):
        draw_aerial("A", os.path.join(OUT, "city_aerial_a.png"))
        draw_aerial("B", os.path.join(OUT, "city_aerial_b.png"))
    if only in ("", "plan"):
        draw_plan(os.path.join(OUT, "city_plan.png"), lights, anchors, main_path)
    if only in ("", "section"):
        draw_section(os.path.join(OUT, "city_section.png"), main_path)
    print("%d polygons, %d houses, %d memory lights, %d anchor lamps, %s" % (
        len(POLYS), len(HOUSES), len(lights), len(anchors), "no problems" if not problems else "%d problems" % len(problems)))
    return 1 if problems else 0

if __name__ == "__main__":
    sys.exit(main())
