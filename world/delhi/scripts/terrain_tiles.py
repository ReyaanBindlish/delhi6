"""Export the Old Delhi bare-earth DTM as Nanite-ready terrain mesh tiles for Unreal.

blender --background --python terrain_tiles.py -- <terrain_dir> <out_dir> [tile_m=400] [res_m=2] [radius_m=900]

Tiles are centred on the terrain origin (same world frame as the city slices: X east, Y north,
Z = metres above terrain base). Only tiles within radius_m of the origin are exported.
In Unreal, enable "Use Complex Collision As Simple" on these meshes (ground collision).
"""
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:]
TDIR, OUT = Path(argv[0]), Path(argv[1])
TILE = float(argv[2]) if len(argv) > 2 else 400.0
RES = float(argv[3]) if len(argv) > 3 else 2.0
RADIUS = float(argv[4]) if len(argv) > 4 else 900.0
OUT.mkdir(parents=True, exist_ok=True)

meta = json.load(open(TDIR / "terrain.json"))
dtm = np.load(TDIR / "old_delhi_dtm.npy")
half, px, base, n = meta["extent_m"] / 2, meta["pixel_m"], meta["base_elev_m"], dtm.shape[0]


def h(x, y):
    c = min(max((x + half) / px, 0), n - 1.001); r = min(max((half - y) / px, 0), n - 1.001)
    c0, r0 = int(c), int(r); fc, fr = c - c0, r - r0
    return float(dtm[r0, c0] * (1 - fc) * (1 - fr) + dtm[r0, c0 + 1] * fc * (1 - fr)
                 + dtm[r0 + 1, c0] * (1 - fc) * fr + dtm[r0 + 1, c0 + 1] * fc * fr) - base


bpy.ops.wm.read_factory_settings(use_empty=True)
mat = bpy.data.materials.new("M_Ground")
k = int(math.ceil(RADIUS / TILE))
tiles = []
steps = int(TILE / RES)
for ty in range(-k, k):
    for tx in range(-k, k):
        x0, y0 = tx * TILE, ty * TILE
        if math.hypot(x0 + TILE / 2, y0 + TILE / 2) > RADIUS + TILE * 0.71:
            continue
        cx, cy = x0 + TILE / 2, y0 + TILE / 2
        bm = bmesh.new()
        uv = bm.loops.layers.uv.new("UVMap")
        grid = [[bm.verts.new((x0 + i * RES - cx, y0 + j * RES - cy, h(x0 + i * RES, y0 + j * RES)))
                 for i in range(steps + 1)] for j in range(steps + 1)]
        for j in range(steps):
            for i in range(steps):
                f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                for l in f.loops:
                    l[uv].uv = ((l.vert.co.x + cx) / 4.0, (l.vert.co.y + cy) / 4.0)  # 4 m texture tiling
        name = f"Terrain_{tx + k:02d}_{ty + k:02d}"
        me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
        me.materials.append(mat)
        ob = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(ob)
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        ob.select_set(True); bpy.context.view_layer.objects.active = ob
        bpy.ops.export_scene.fbx(filepath=str(OUT / f"{name}.fbx"), use_selection=True, apply_unit_scale=True,
                                 apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                                 mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z")
        tiles.append({"id": name, "location_m": [cx, cy, 0.0], "fbx": f"{name}.fbx"})
json.dump({"tile_m": TILE, "res_m": RES, "tiles": tiles, "terrain": meta}, open(OUT / "terrain_manifest.json", "w"),
          indent=1)
print(f"TERRAIN TILES {len(tiles)} x {TILE:.0f} m @ {RES} m -> {OUT}")
