"""Render the west-loop review. Does not export or modify runtime town data.

Place beside city_model.py, then run: python tools/layout/west_loop_preview.py
Pass --output to change the default build/west_loop_preview.png destination.
"""

import argparse
import math
from pathlib import Path
from tempfile import TemporaryDirectory

from PIL import Image, ImageDraw, ImageFont
import city_model as town

GOLD = '#ffc36a'
TEAL = '#73f0d3'


def arc(degrees, y, radius=20.0):
    a = math.radians(degrees)
    return (radius * math.cos(a), y, radius * math.sin(a))


WEST = (-27.0, 14.0, 0.0)
LANDING = arc(90, -5.8)
HOUSE = (town.LIFT_X, -3.4, town.LIFT_HOUSE_Z)
UPPER = (-1.0, 14.0, -14.5)
WRECK = [WEST] + [arc(174 - 7 * i, 14 - 1.5 * (i + 1)) for i in range(13)] + [LANDING]
FLOOR = [LANDING, *[arc(a, -5.8) for a in range(96, 253, 6)], HOUSE]
# Spatial proposal only: the centerline is not a terrain cut or collision mesh.
GALLERY = [(-27, 14, 0), (-23.1, 13.8, 2.0)] + [
    arc(a, 13.8 - 19.6 * i / 11, 22.0) for i, a in enumerate(range(174, 85, -8))
] + [LANDING]


def route_overlay(section=False):
    def draw(ax, eye, right, up, forward, focal, width, height, ortho):
        def project(point):
            x, y, z = point
            if section:
                # Elevation diagram: unwind the route into a non-metric sequence.
                return x, y
            delta = town.np.asarray((x, y, z)) - eye
            return width / 2 + ortho * (delta @ right), height / 2 - ortho * (delta @ up)

        def line(points, color, style):
            xs, ys = zip(*map(project, points))
            ax.plot(xs, ys, style, color='#111b24', lw=6, zorder=7, solid_capstyle='round')
            ax.plot(xs, ys, style, color=color, lw=3, zorder=8, solid_capstyle='round')

        if section:
            stations = [(75, 125, 1), (310, 415, 2), (535, 379, 3), (770, 125, 4)]
            line([(x, y, 0) for x, y, _ in stations], GOLD, '-')
            line([(75, 125, 0), (190, 158, 0), (240, 234, 0), (310, 415, 0)], TEAL, '--')
            ax.text(185, 112, 'PROPOSED GALLERY', color=TEAL, fontsize=10, weight='bold')
            ax.text(513, 267, 'SWITCH TO A AT WINCH', color=GOLD, fontsize=10, weight='bold')
            ax.text(575, 312, 'RETURN TO B TO CLIMB', color=GOLD, fontsize=10, weight='bold')
            for yy, label in [(125, '+14 m: bridgehead / plaza'), (415, '-6 m: gorge floor')]:
                ax.axhline(yy, color='#485966', lw=0.7, zorder=1)
                ax.text(32, yy + 31, label, color='#dbe6ed', fontsize=10)
        else:
            line(WRECK, GOLD, '-')
            line(FLOOR, GOLD, '-')
            line([HOUSE, UPPER], GOLD, '-')
            line(GALLERY, TEAL, '--')
            stations = [(*project(a), n) for n, a in enumerate((WEST, LANDING, HOUSE, UPPER), 1)]
        for x, y, label in stations:
            ax.text(x, y, str(label), ha='center', va='center', color='white', weight='bold', fontsize=11,
                    bbox=dict(boxstyle='circle,pad=.25', fc='#152635', ec=GOLD), zorder=9)
    return draw


def render(path, world, eye, target, *, fov=68, ortho=None, open_lift=False, overlay=None):
    baseline = len(town.POLYS)
    things = town.THINGS[:]
    try:
        town.THINGS[:] = [t for t in things if t[0] != 'lift' or not open_lift]
        town.POLYS.extend(town.thing_polys('shut'))
        if open_lift:
            town.THINGS[:] = [t for t in things if t[0] == 'lift']
            town.POLYS.extend(town.thing_polys('open'))
        town.render(world, str(path), eye, target, fov, (900, 505), '', overlay, ortho)
    finally:
        town.THINGS[:] = things
        del town.POLYS[baseline:]


def elevation(path):
    from matplotlib import pyplot as plt
    fig = plt.figure(figsize=(9, 5.05), dpi=100, facecolor='#172430')
    ax = fig.add_axes((0, 0, 1, 1))
    ax.set(xlim=(0, 900), ylim=(505, 0))
    ax.axis('off')
    route_overlay(section=True)(ax, None, None, None, None, None, 900, 505, None)
    fig.savefig(path, dpi=100, facecolor=fig.get_facecolor())
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('build/west_loop_preview.png'))
    output = parser.parse_args().output
    town.build_all()
    if town.check():
        raise SystemExit('Baseline town layout check failed; preview not written.')
    with TemporaryDirectory() as directory:
        temp = Path(directory)
        panels = [
            ('01  WEST APPROACH OVERVIEW / A', 'Same camera: intact span, closed far gate.'),
            ('02  WEST APPROACH OVERVIEW / B', 'Same camera: the fallen span becomes a way down.'),
            ('03  LIFT-HOUSE ROOF / B', 'Player-scale view toward the lift before the winch is used.'),
            ('04  LIFT AND UPPER STATION / B', 'South oblique: lift frame after lowering the cage.'),
            ('05  PLAN / B', 'Gold: route intent. Dashed teal: proposed maintenance gallery.'),
            ('06  ELEVATION / SCHEMATIC', '1 bridgehead  /  2 landing  /  3 winch  /  4 upper station'),
        ]
        paths = [temp / f'{i}.png' for i in range(len(panels))]
        bridge_eye, bridge_target = (-54.0, 44.0, -25.0), (-10, 16, 1)
        render(paths[0], 'A', bridge_eye, bridge_target, fov=78)
        render(paths[1], 'B', bridge_eye, bridge_target, fov=78)
        render(paths[2], 'B', (-5.8, -1.3, -20.0), (-3.0, 6.0, -15.5), fov=81)
        render(paths[3], 'B', (24.0, 34.0, -42.0), (-5.0, 9.0, -15.0), fov=78, open_lift=True)
        render(paths[4], 'B', (-3.0, 110.0, -0.1), (-3.0, 0.0, 0.0), ortho=10.2,
               open_lift=True, overlay=route_overlay())
        elevation(paths[5])
        sheet = Image.new('RGB', (1884, 1855), '#101922')
        ink = ImageDraw.Draw(sheet)
        def font(size):
            try:
                return ImageFont.truetype('DejaVuSans.ttf', size)
            except OSError:
                return ImageFont.load_default()
        ink.text((28, 19), 'DUALLAYER  /  THE WEST LOOP', fill='white', font=font(39))
        ink.text((31, 73), 'Layout model review  |  A: past  /  B: present  |  Gallery is a proposal, not playable geometry',
                 fill='#9eb7c7', font=font(23))
        for i, ((title, note), image) in enumerate(zip(panels, paths)):
            x, y = 28 + (i % 2) * 928, 120 + (i // 2) * 569
            with Image.open(image) as panel:
                sheet.paste(panel.convert('RGB'), (x, y))
            ink.text((x + 10, y + 508), title, fill='#eef6ff', font=font(22))
            ink.text((x + 10, y + 536), note, fill='#afc4d1', font=font(18))
        output.parent.mkdir(parents=True, exist_ok=True)
        sheet.save(output, optimize=True)
    print(output)


if __name__ == '__main__':
    main()
