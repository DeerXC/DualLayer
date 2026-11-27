"""town_style.py - one palette and one set of rules for the whole town (docs/DESIGN.md 6.9).

Every material of the town is defined here, once, by a role: what it is made of, which family of colours it belongs to, and how
it ages. A is written by hand (sRGB); B is DERIVED from A by the rule of its class, so that the present is the same town seen
through the same eyes (cool, damp, value-compressed) and a material that is added later has its B without anyone choosing it.
`audit_palette()` checks the palette against the rules of the style; `audit_town()` checks a built town (the polygons of the
model) against the same rules by area. Both can be run without the game:
    python tools/layout/town_style.py                 the palette
Needs numpy only.

The rules (each one is checked):
  1. The warm family (limestone, lime wash, clay) is most of what is built, the cool family (leaf, sage, verdigris) a fifth, the
     accents (sea blue, oxblood, awnings, flowers) a few per cent: small, and there. Measured by the area of the built, coloured surfaces.
  2. Value before colour: in greyscale the roof is darker than the wall by L* 18 or more, and the ground is darker than the lightest
     walls, in A and in B.
  3. Neighbouring houses differ: any two plasters differ by Delta E 9 or more (the colours that name a house).
  4. Gold means use: gold is the only saturated yellow-orange that is not plaster or tile, and nothing but handles, bells and
     the lantern uses it.
  5. B is derived: B = class tint x (value scale x luma of A) + hue memory x (A - luma of A); never a free choice.
"""
import math

import numpy as np

# ---- colour maths ---------------------------------------------------------------------------------------------------------


def srgb_to_lin(c):
    c = np.asarray(c, float)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def lin_to_srgb(c):
    c = np.clip(np.asarray(c, float), 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * c ** (1 / 2.4) - 0.055)


def luma(c):
    """Relative luminance (linear light) of an sRGB colour."""
    l = srgb_to_lin(c)
    return float(0.2126 * l[0] + 0.7152 * l[1] + 0.0722 * l[2])


def lstar(c):
    y = luma(c)
    f = y ** (1 / 3) if y > 0.008856 else 7.787 * y + 16 / 116
    return 116 * f - 16


def lab(c):
    l = srgb_to_lin(c)
    M = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = M @ l / np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.array([116 * f[1] - 16, 500 * (f[0] - f[1]), 200 * (f[1] - f[2])])


def delta_e(a, b):
    return float(np.linalg.norm(lab(a) - lab(b)))


def chroma(c):
    L = lab(c)
    return float(math.hypot(L[1], L[2]))


def hue_deg(c):
    L = lab(c)
    return math.degrees(math.atan2(L[2], L[1])) % 360.0


# ---- the palette ----------------------------------------------------------------------------------------------------------
# name: (A in sRGB, class, family, role)
#   class: how B is derived (see class_b); family: 'warm' (limestone, lime wash, clay, earth), 'cool' (sea, leaf, shade) or 'accent'
#   (small, saturated, and used on purpose); 'light' (what makes light) and 'dark' (iron, wood, doors) are outside the 70/20/10.
PALETTE = {
    # limestone: one stone for walls, stairs and streets, weathered to honey (Dubrovnik is built of one stone)
    "stone": ((0.86, 0.76, 0.60), "stone", "warm", "limestone of walls and stairs"),
    "stone_dark": ((0.63, 0.51, 0.40), "stone", "warm", "shaded masonry: plinths, jambs, arches"),
    "rock": ((0.66, 0.56, 0.47), "stone", "warm", "the cliff and the rock"),
    "paving": ((0.78, 0.69, 0.56), "stone", "warm", "streets and squares: darker than the walls"),
    "sand": ((0.86, 0.77, 0.58), "stone", "warm", "beaches, dust, the shore"),
    "debris": ((0.58, 0.49, 0.43), "stone", "warm", "rubble and fallen things"),
    # lime wash in the earth pigments of the coast: eight colours, each of them names a house
    "plaster0": ((0.96, 0.86, 0.68), "plaster", "warm", "cream"),
    "plaster1": ((0.89, 0.71, 0.40), "plaster", "warm", "yellow ochre"),
    "plaster2": ((0.88, 0.62, 0.50), "plaster", "warm", "terracotta rose"),
    "plaster3": ((0.96, 0.95, 0.90), "plaster", "warm", "ivory lime wash"),
    "plaster4": ((0.95, 0.87, 0.60), "plaster", "warm", "pale yellow"),
    "plaster5": ((0.92, 0.72, 0.64), "plaster", "warm", "salmon"),
    "plaster6": ((0.73, 0.77, 0.63), "plaster", "cool", "faded sage"),
    "plaster7": ((0.70, 0.76, 0.80), "plaster", "cool", "dusty blue-grey"),
    "chimney": ((0.80, 0.62, 0.50), "plaster", "warm", "chimney stacks"),
    # clay tile: four ages of the same tile (the patchwork of a real roof)
    "roof": ((0.70, 0.31, 0.19), "tile", "warm", "terracotta"),
    "roof2": ((0.74, 0.40, 0.22), "tile", "warm", "new tile, pale orange"),
    "roof3": ((0.52, 0.24, 0.18), "tile", "warm", "old tile, dark red-brown"),
    "roof4": ((0.64, 0.42, 0.32), "tile", "warm", "faded tile, dusty"),
    "copper": ((0.24, 0.56, 0.50), "metal", "cool", "verdigris: civic domes and cupolas only"),
    # the accents of a house: shutters, awnings, flowers
    "shutter": ((0.36, 0.52, 0.44), "paint", "cool", "sage green"),
    "shutter2": ((0.27, 0.45, 0.62), "paint", "accent", "sea blue"),
    "shutter3": ((0.52, 0.24, 0.22), "paint", "accent", "oxblood"),
    "cloth": ((0.80, 0.28, 0.22), "paint", "accent", "red awning"),
    "cloth2": ((0.72, 0.50, 0.16), "paint", "accent", "mustard awning"),
    "cloth3": ((0.30, 0.50, 0.66), "paint", "accent", "blue awning"),
    "cloth4": ((0.93, 0.89, 0.79), "paint", "warm", "white cloth, laundry"),
    "bloom": ((0.78, 0.30, 0.48), "paint", "accent", "bougainvillea: pergolas and balconies only"),
    # wood, iron and doors: dark neutrals that carry the drawing
    "wood": ((0.54, 0.36, 0.23), "wood", "dark", "timber"),
    "door": ((0.42, 0.25, 0.17), "wood", "dark", "doors"),
    "iron": ((0.30, 0.28, 0.30), "iron", "dark", "iron"),
    # the ground and the plants
    "grass": ((0.52, 0.59, 0.33), "green", "cool", "grass: olive, not lime"),
    "field": ((0.80, 0.64, 0.36), "green", "warm", "tilled earth, stubble"),
    "tree": ((0.40, 0.50, 0.29), "green", "cool", "olive and fig crowns"),
    "cypress": ((0.22, 0.34, 0.25), "green", "cool", "cypress, pine"),
    "moss": ((0.50, 0.53, 0.30), "green", "cool", "moss and scrub"),
    # water and light
    "water": ((0.20, 0.56, 0.68), "water", "cool", "harbour and canal water"),
    "sea": ((0.22, 0.44, 0.66), "water", "cool", "the sea"),
    "gold": ((0.96, 0.70, 0.26), "gold", "light", "use: handles, bells, the lantern's brass"),
    "window": ((1.00, 0.82, 0.46), "light", "light", "a lit window (dark in B)"),
    "lantern": ((1.00, 0.82, 0.46), "light", "light", "a lit lantern (warm in B too)"),
    "memory": ((0.80, 0.95, 1.00), "light", "light", "the cold light of what was"),
    "glow": ((1.00, 0.80, 0.46), "light", "light", "the pool of a lantern on the ground"),
}

# how B is derived: B = tint * (vscale * Y) + memory * (A - Y)   (per channel, sRGB; Y = the grey of A)
#   tint: the colour of the present, vscale: how much of the value is kept, memory: how much of A's hue is kept (so that neighbours stay
#   different and the same place is recognised in greyscale and in colour)
# B is the same town after the storm and the years, in the light of the moon: the stone is weathered and DULL, not blue. The cold of the night
# comes from the light (the moon, the sky light, the haze: LIGHT["B"] of the model, src/render/LightingSettings.h StyleLookOf), so what
# is lit by the moon looks silver and what is lit by a lamp looks amber. A material of B therefore keeps most of the hue of its A (what
# a stone is does not change in a night) and loses value and chroma; the tint is only a little cool.
#   CLASS_B_COLD: the B of passes 1 to 5 (every material tinted toward blue: a night of ice)
#   CLASS_B_AGED: weathered stone in moonlight (tint near neutral, most of the hue of A kept)
#   B_AGE: 0 = cold, 1 = aged, in between = both. TUNE it here and export again (python tools/layout/city_model.py export).
B_AGE = 0.65
CLASS_B_COLD = {
    "stone": dict(tint=(0.76, 0.90, 1.06), vscale=0.85, memory=0.30),
    "plaster": dict(tint=(0.84, 0.96, 1.08), vscale=0.76, memory=0.34),
    "tile": dict(tint=(0.68, 0.84, 1.06), vscale=0.80, memory=0.10),
    "metal": dict(tint=(0.92, 1.00, 1.00), vscale=1.00, memory=1.00),     # verdigris is already cold: it brightens a little
    "paint": dict(tint=(0.76, 0.90, 1.06), vscale=0.70, memory=0.10),     # paint fades to grey
    "wood": dict(tint=(0.68, 0.78, 0.90), vscale=0.90, memory=0.15),
    "iron": dict(tint=(0.68, 0.78, 0.90), vscale=1.00, memory=0.30),
    "green": dict(tint=(0.60, 0.90, 0.98), vscale=0.70, memory=0.40),     # green stays green: the present grows
    "water": dict(tint=(0.90, 0.94, 1.00), vscale=0.80, memory=0.50),
    "gold": dict(tint=(1.00, 0.90, 0.72), vscale=0.70, memory=0.60),      # gold dims in B but stays warm: use is still use
    "light": None,                                                         # set by hand: see LIGHT_B
}
CLASS_B_AGED = {
    "stone": dict(tint=(0.97, 0.96, 0.96), vscale=0.80, memory=0.62),
    "plaster": dict(tint=(0.97, 0.96, 0.97), vscale=0.74, memory=0.62),
    "tile": dict(tint=(0.92, 0.90, 0.94), vscale=0.76, memory=0.50),       # the old tile keeps its rust, only darker
    "metal": dict(tint=(0.92, 1.00, 1.00), vscale=1.00, memory=1.00),
    "paint": dict(tint=(0.92, 0.93, 0.96), vscale=0.66, memory=0.40),
    "wood": dict(tint=(0.92, 0.91, 0.92), vscale=0.84, memory=0.40),
    "iron": dict(tint=(0.88, 0.90, 0.95), vscale=1.00, memory=0.35),
    "green": dict(tint=(0.78, 0.95, 0.88), vscale=0.72, memory=0.55),
    "water": dict(tint=(0.90, 0.94, 1.00), vscale=0.80, memory=0.50),
    "gold": dict(tint=(1.00, 0.90, 0.72), vscale=0.70, memory=0.60),
    "light": None,
}


def class_b(cls):
    """The rule of a class: the cold and the aged rule, mixed by B_AGE."""
    cold, aged = CLASS_B_COLD[cls], CLASS_B_AGED[cls]
    if cold is None:
        return None
    k = min(max(B_AGE, 0.0), 1.0)
    return {key: (tuple(c + (a - c) * k for c, a in zip(cold[key], aged[key])) if key == "tint" else cold[key] + (aged[key] - cold[key]) * k)
            for key in cold}


LIGHT_B = {                                                                # what is lit in A and in B is a decision, not a rule
    "window": (0.10, 0.12, 0.16),                                          # dark: nobody is home
    "lantern": (1.00, 0.70, 0.36),                                         # the one warm light of B
    "memory": (0.62, 0.92, 1.00),
    "glow": (0.78, 0.58, 0.36),
}
PAINTED_CLASSES = ("stone", "plaster", "tile")
PAINTED_NAMES = ("grass", "field")                                         # and the ground: the shader paints blotches on what it is told is the city
EMISSIVE = {"window": (True, False), "lantern": (True, True), "memory": (True, True), "glow": (True, True)}


def derive_b(name, a=None):
    """B of a material, from its A and the rule of its class."""
    entry = PALETTE[name]
    a = np.asarray(entry[0] if a is None else a, float)
    cls = entry[1]
    if cls == "light":
        return LIGHT_B[name]
    p = class_b(cls)
    y = luma_s(a)
    b = np.asarray(p["tint"]) * (p["vscale"] * y) + p["memory"] * (a - y)
    return tuple(float(x) for x in np.clip(b, 0.0, 1.0))


def luma_s(c):
    """The sRGB grey of an sRGB colour (the weights of Rec.709 on the gamma values: close enough for a tint)."""
    c = np.asarray(c, float)
    return float(0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2])


def build_mat():
    """The MAT table of the model: name -> (A, B)."""
    return {n: (tuple(PALETTE[n][0]), derive_b(n)) for n in PALETTE}


def is_painted(name):
    e = PALETTE.get(name)
    return e is not None and (e[1] in PAINTED_CLASSES or name in PAINTED_NAMES)


# ---- the audit of the palette --------------------------------------------------------------------------------------------------
PLASTERS = tuple(n for n in PALETTE if n.startswith("plaster"))
ROOFS = tuple(n for n in PALETTE if n.startswith("roof"))
SHUTTERS = ("shutter", "shutter2", "shutter3")
AWNINGS = ("cloth", "cloth2", "cloth3")


def audit_palette(verbose=True):
    mat = build_mat()
    out = []

    def check(cond, what):
        out.append((bool(cond), what))
    for world, idx in (("A", 0), ("B", 1)):
        L = lambda n: lstar(mat[n][idx])
        walls = [L(n) for n in PLASTERS] + [L("stone")]
        roofs = [L(n) for n in ROOFS]
        check(max(roofs) <= min(walls) - (18.0 if world == "A" else 8.0), "%s: the lightest roof (L* %.0f) is darker than the darkest wall (L* %.0f) by %.0f or more" % (world, max(roofs), min(walls), 18 if world == "A" else 8))
        check(L("paving") <= max(walls) - 6.0, "%s: the paving (L* %.0f) is darker than the lightest wall (L* %.0f) by 6 or more" % (world, L("paving"), max(walls)))
        check(L("stone_dark") <= L("stone") - 8.0, "%s: shaded masonry (L* %.0f) is darker than lit stone (L* %.0f) by 8 or more" % (world, L("stone_dark"), L("stone")))
        check(L("grass") <= L("paving") + 4.0 and abs(L("grass") - L("paving")) >= 3.0 or world == "B", "%s: grass and paving are not the same value (L* %.0f, %.0f)" % (world, L("grass"), L("paving")))
    lo = min(delta_e(mat[a][0], mat[b][0]) for i, a in enumerate(PLASTERS) for b in PLASTERS[i + 1:])
    check(lo >= 9.0, "any two plasters differ by Delta E %.1f (9 or more)" % lo)
    lo_b = min(delta_e(mat[a][1], mat[b][1]) for i, a in enumerate(PLASTERS) for b in PLASTERS[i + 1:])
    check(lo_b >= 4.0, "in B any two plasters still differ by Delta E %.1f (4 or more: the same houses can be told apart)" % lo_b)
    lo_r = min(delta_e(mat[a][0], mat[b][0]) for i, a in enumerate(ROOFS) for b in ROOFS[i + 1:])
    check(lo_r >= 9.0, "any two tiles differ by Delta E %.1f (9 or more: a patchwork)" % lo_r)
    # one stone: the stones and the streets are the same family in hue
    hs = [hue_deg(mat[n][0]) for n in ("stone", "paving", "sand", "stone_dark", "rock")]
    check(max(hs) - min(hs) < 25.0, "the stones are one family: hues %.0f to %.0f degrees" % (min(hs), max(hs)))
    # shutters and awnings: cool or saturated, and different from every wall
    for n in SHUTTERS + AWNINGS:
        d = min(delta_e(mat[n][0], mat[p][0]) for p in PLASTERS)
        check(d >= 20.0, "%s differs from every plaster by Delta E %.0f (20 or more)" % (n, d))
    # gold is the one saturated warm light: nothing else is that yellow at that chroma and that light
    g = mat["gold"][0]
    others = [n for n in PALETTE if n not in ("gold", "window", "lantern", "glow") and chroma(mat[n][0]) >= 0.8 * chroma(g) and lstar(mat[n][0]) >= lstar(g) - 8 and abs(hue_deg(mat[n][0]) - hue_deg(g)) < 20]
    check(not others, "gold is alone: nothing else is as yellow, as saturated and as light %s" % others)
    # B is derived: no material of B is lighter than A except the lights, the copper and the plants (which brighten a little by memory)
    bad = [n for n in PALETTE if PALETTE[n][1] not in ("light", "metal", "green", "water") and lstar(mat[n][1]) > lstar(mat[n][0]) + 0.5]
    check(not bad, "B is darker than A (it is the present): %s" % bad)
    if verbose:
        for ok, what in out:
            print("  %s %s" % ("ok  " if ok else "FAIL", what))
    return [w for ok, w in out if not ok]


# ---- the audit of a built town -------------------------------------------------------------------------------------------------
def poly_area(P):
    P = np.asarray(P, float)
    if len(P) < 3:
        return 0.0
    a = 0.0
    for i in range(1, len(P) - 1):
        a += 0.5 * float(np.linalg.norm(np.cross(P[i] - P[0], P[i + 1] - P[0])))
    return a


BACKDROP = ("sea", "water", "rock")      # not the town: the sea, the water and the cliffs are the landscape the town stands in


def audit_town(polys, verbose=True):
    """polys: (points, material, world, kind) of the model. The area of what is built (solid surfaces and decals, in A; not the sea,
    the water, the cliffs or the open ground) by family and by role; the grey of the roofs, the walls and the ground, in A and in B.
    Returns (shares in percent, mean L* by role, area by material, the list of failed rules)."""
    mat = build_mat()
    share = {"warm": 0.0, "cool": 0.0, "accent": 0.0, "dark": 0.0, "light": 0.0}
    role_y = {"wall": [0.0, 0.0, 0.0], "roof": [0.0, 0.0, 0.0], "ground": [0.0, 0.0, 0.0]}      # area, sum of L* in A, sum of L* in B
    per = {}
    for P, m, w, kind in polys:
        if m not in PALETTE or kind not in ("solid", "overlay") or w == "B" or m in BACKDROP:
            continue
        ar = poly_area(P)
        if ar <= 0.0:
            continue
        fam = PALETTE[m][2]
        share[fam] += ar
        per[m] = per.get(m, 0.0) + ar
        role = "roof" if m.startswith("roof") else "wall" if (m.startswith("plaster") or m in ("stone", "chimney")) else "ground" if m in ("paving", "grass", "field") else None
        if role:
            r = role_y[role]
            r[0] += ar
            r[1] += ar * lstar(mat[m][0])
            r[2] += ar * lstar(mat[m][1])
    seen = share["warm"] + share["cool"] + share["accent"]
    pct = {k: (100.0 * share[k] / seen if seen > 0 else 0.0) for k in ("warm", "cool", "accent")}
    fails = []

    def check(cond, what):
        if verbose:
            print("  %s %s" % ("ok  " if cond else "FAIL", what))
        if not cond:
            fails.append(what)
    check(65.0 <= pct["warm"] <= 92.0, "warm family %.0f%% of the built, coloured area (65 to 92)" % pct["warm"])
    check(5.0 <= pct["cool"] <= 30.0, "cool family %.0f%% (5 to 30)" % pct["cool"])
    check(0.3 <= pct["accent"] <= 10.0, "accents %.1f%% (0.3 to 10: small, and there)" % pct["accent"])
    mean = {k: (v[1] / v[0], v[2] / v[0]) for k, v in role_y.items() if v[0] > 0}
    if len(mean) == 3:
        for i, world in ((0, "A"), (1, "B")):
            check(mean["wall"][i] - mean["roof"][i] >= (14.0 if i == 0 else 7.0), "%s: walls L* %.0f, roofs L* %.0f (walls lighter by %.0f)" % (world, mean["wall"][i], mean["roof"][i], mean["wall"][i] - mean["roof"][i]))
            check(mean["wall"][i] - mean["ground"][i] >= 3.0, "%s: ground L* %.0f is darker than walls L* %.0f" % (world, mean["ground"][i], mean["wall"][i]))
    return pct, mean, per, fails


if __name__ == "__main__":
    import sys
    print("palette:")
    f = audit_palette(True)
    print("\n%d rules failed" % len(f))
    sys.exit(1 if f else 0)
