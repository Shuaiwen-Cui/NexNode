"""
Split ENCLOSURE-3D-BODY.stl into a base + four side walls for the web exploded view.

    python -m pip install trimesh mapbox-earcut shapely rtree networkx
    python DOC/tools/split_enclosure.py

Cuts (model mm, Z-up, same frame as the Fusion export):
  * mounting ears  x < 0 and x > 100          -> follow the left / right walls
  * base           0 < x < 100, z < 3         (floor plate incl. wall footings)
  * left / right   wall slabs x < 2.5, x > 97.5 above the floor
  * front / back   wall slabs y < 2.5, y > 72.5 between the side walls
  * interior bosses follow the wall they grow out of: the four BMS slot ribs go with
    the left wall, battery-ledge blocks with the front/back walls; only the free-standing
    PCB standoffs stay on the base.
All cuts are capped, so every piece is a closed solid. Re-run after re-exporting the STL.
"""
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/HARDWARE-ESP/FUSION/ENCLOSURE-3D-BODY.stl"
OUT = ROOT / "docs/static/liftnode-viewer/models"

X0, X1, Y0, Y1, WALL, FLOOR = 0.0, 100.0, 0.0, 75.0, 2.5, 3.0


def cut(mesh, normal, origin):
    """Keep the part of `mesh` on the +normal side of the plane, capped."""
    return trimesh.intersections.slice_mesh_plane(mesh, plane_normal=normal, plane_origin=origin, cap=True)


def main():
    m = trimesh.load(SRC)
    m.merge_vertices(digits_vertex=3)

    ear_l = cut(m, [-1, 0, 0], [X0, 0, 0])
    ear_r = cut(m, [1, 0, 0], [X1, 0, 0])
    center = cut(cut(m, [1, 0, 0], [X0, 0, 0]), [-1, 0, 0], [X1, 0, 0])

    base = cut(center, [0, 0, -1], [0, 0, FLOOR])
    upper = cut(center, [0, 0, 1], [0, 0, FLOOR])
    left = cut(upper, [-1, 0, 0], [X0 + WALL, 0, 0])
    right = cut(upper, [1, 0, 0], [X1 - WALL, 0, 0])
    mid = cut(cut(upper, [1, 0, 0], [X0 + WALL, 0, 0]), [-1, 0, 0], [X1 - WALL, 0, 0])
    front = cut(mid, [0, -1, 0], [0, Y0 + WALL, 0])
    back = cut(mid, [0, 1, 0], [0, Y1 - WALL, 0])
    inner = cut(cut(mid, [0, 1, 0], [0, Y0 + WALL, 0]), [0, -1, 0], [0, Y1 - WALL, 0])

    pieces = {"base": [base], "wall-left": [ear_l, left], "wall-right": [ear_r, right],
              "wall-front": [front], "wall-back": [back]}
    # Interior features follow the wall they grow out of (e.g. the BMS slot ribs on the
    # left wall, battery-ledge blocks on the front/back walls); only free-standing floor
    # features (PCB standoffs) stay on the base.
    eps = 0.05
    for comp in inner.split(only_watertight=False):
        (x0, y0, z0), (x1, y1, z1) = comp.bounds
        if x0 < X0 + WALL + eps:          # left first: the corner rib also touches the front wall
            pieces["wall-left"].append(comp)
        elif y0 < Y0 + WALL + eps:
            pieces["wall-front"].append(comp)
        elif y1 > Y1 - WALL - eps:
            pieces["wall-back"].append(comp)
        elif x1 > X1 - WALL - eps:
            pieces["wall-right"].append(comp)
        else:
            pieces["base"].append(comp)

    OUT.mkdir(parents=True, exist_ok=True)
    total = 0.0
    for name, parts in pieces.items():
        mesh = trimesh.util.concatenate(parts)
        mesh.export(OUT / f"body-{name}.stl")
        total += mesh.volume
        print(f"body-{name}.stl  faces={len(mesh.faces):5d}  volume={mesh.volume:9.0f}")
    print(f"total volume {total:.0f} (source {m.volume:.0f})")


if __name__ == "__main__":
    main()
