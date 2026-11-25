"""Render repeatable review cameras from the town source model, without writing game data."""

import argparse
import configparser
from pathlib import Path

import numpy as np

import city_model as city

ROOT = Path(__file__).resolve().parents[2]
SIZE = (1440, 810)
ACCENT = '#ffd781'


def camera(feet, aim, reach):
    """Use the runtime's nominal orbit distance and pivot, without its collision solver."""
    pivot = np.asarray(feet, dtype=float) + (0, 1.35, 0)
    direction = np.asarray(aim) - pivot
    direction /= np.linalg.norm(direction)
    return pivot - reach * direction, aim


def markup(feet=None, route=(), plan=False):
    """Project annotations from world coordinates. Dashed lines are proposals, not geometry."""
    def draw(ax, eye, right, up, forward, focal, width, height, ortho):
        def project(point):
            delta = np.asarray(point) - eye
            depth = delta @ forward
            if ortho is None and depth < 0.6:
                return None
            scale = ortho if ortho is not None else focal / depth
            return width / 2 + scale * (delta @ right), height / 2 - scale * (delta @ up)

        if plan:
            # Connect the source model's route anchors, not a collision-verified trace.
            points = [project((x, 0, z)) for _, x, z, _ in city.MAIN_PATH[:16]]
            points.append(project(route[0]))
            ax.plot(*zip(*points), color='#ee6f45', lw=3, zorder=5)
            for number, index in enumerate((0, 1, 4, 8, 12, 15), 1):
                x, y = points[index]
                ax.text(x, y, str(number), color='white', ha='center', va='center', weight='bold',
                        bbox=dict(boxstyle='circle,pad=.3', fc='#9c3d26', ec='white'), zorder=8)
        for first, second in zip(route, route[1:]):
            a, b = project(first), project(second)
            if a is not None and b is not None:
                ax.plot((a[0], b[0]), (a[1], b[1]), '--', color=ACCENT, linewidth=2.4, zorder=7)
        if route and (end := project(route[-1])) is not None:
            ax.scatter(*end, s=130, facecolors='none', edgecolors=ACCENT, linewidths=2, zorder=8)
        if feet is not None:
            # A schematic 1.7 m scale marker, not the rendered player asset.
            x, y, z = feet
            def body(side, height):
                return project((x + side * right[0], y + height, z + side * right[2]))

            points = [(-.12, 0), (-.19, .73), (-.34, .82), (-.29, 1.34), (-.12, 1.42),
                      (-.14, 1.57), (-.08, 1.7), (.08, 1.7), (.14, 1.57), (.12, 1.42),
                      (.29, 1.34), (.34, .82), (.19, .73), (.12, 0), (.015, 0), (0, .67), (-.015, 0)]
            projected = [body(*point) for point in points]
            if all(point is not None for point in projected):
                ax.fill(*zip(*projected), color='#17202c', ec='#fff4d9', lw=1.1, zorder=9)
    return draw


def review(output, shot):
    settings = configparser.ConfigParser(inline_comment_prefixes=('#', ';'))
    settings.read(ROOT / 'config' / 'default.ini', encoding='utf-8')
    fov = float(np.clip(settings.getfloat('camera', 'fov_deg', fallback=70), 60, 90))
    reach = float(np.clip(settings.getfloat('camera', 'orbit_distance', fallback=3.8), 2, 6))
    city.build_all()
    # Pictures use initial puzzle states; no changes to game export or route tests.
    city.POLYS.extend(city.thing_polys('shut'))
    output.mkdir(parents=True, exist_ok=True)
    market = (-31.3, city.ROCK_Y, -0.4)
    north = (0, city.THEATRE_Y, city.THEATRE[1] + 1)
    door = (city.TOWER[0], city.ROCK_Y + 2.0, city.TOWER[1] + 8.7)
    proposal = ((4, city.THEATRE_Y + .3, 35), (5, 20.8, 32),
                (5, 17.8, 22), (3, 14.3, 13), door)
    shots = {
        '01_city': ((-122, 106, -179), (4, 13, -6), 56, False, None, None, 'City from the coast'),
        '02_lookback': ((100, 92, 150), (5, 8, -10), 52, False, None, None, 'City from the northern ridge'),
        '03_market': (*camera(market, (0, 20, 4), reach), fov, True, markup(market), None, 'West bastion | nominal player camera'),
        '04_gorge': ((-41, 37, 35), (-5, 7, -8), 65, True, None, None, 'Gorge floor -6 m / rock plaza +14 m'),
        '05_north': (*camera(north, (0, 19.0, 10), reach), fov, True, markup(north, proposal), None,
                     'North approach | dashed = proposed, not walkable'),
        '06_plan': ((4, 300, -2), (4, 0, -2.0001), 0, False, markup(route=proposal, plan=True), 3.45,
                    'North-up plan | orange = baseline route anchors | dashed = proposal'),
    }
    for key, (eye, target, angle, vertical, overlay, ortho, label) in shots.items():
        if shot not in ('all', key):
            continue
        for world in 'AB':
            path = output / f'{key}_{world}.png'
            count = city.render(world, path, eye, target, angle, SIZE, f'{world} | {label}', overlay, ortho,
                                vertical_fov=vertical)
            if count == 0:
                raise RuntimeError(f'No visible geometry in {key} {world}')
            print(f'{path.name}: {count} visible polygons')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'build' / 'town-review')
    parser.add_argument('--shot', default='all', choices=('all', '01_city', '02_lookback', '03_market',
                                                         '04_gorge', '05_north', '06_plan'))
    args = parser.parse_args()
    review(args.out, args.shot)


if __name__ == '__main__':
    main()
