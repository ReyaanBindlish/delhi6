"""Find good camera bookmarks in a built slice (.blend) and write cameras.json in world metres,
with Unreal yaw/pitch (Unreal Y = -Blender Y).

blender --background <slice.blend> --python find_cameras.py -- <slice.json> <out cameras.json>
"""
import json
import math
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
data = json.load(open(argv[0])); OUT = argv[1]
scene = bpy.context.scene
dg = bpy.context.evaluated_depsgraph_get()
roads = bpy.data.objects.get("Roads")
ox = oy = 0.0
b0 = bpy.data.objects.get(data["buildings"][0]["id"])
if b0:  # recover slice->world offset from the first building's placement
    fp = data["buildings"][0]["footprint"]
    ox = b0.location.x - sum(p[0] for p in fp) / len(fp)
    oy = b0.location.y - sum(p[1] for p in fp) / len(fp)


def free(o, d, maxd):
    hit, loc, *_ = scene.ray_cast(dg, o, d, distance=maxd)
    return (loc - o).length if hit else maxd


def ground_z(x, y):
    hit, loc, *_ = scene.ray_cast(dg, Vector((x, y, 300)), Vector((0, 0, -1)), distance=600)
    return loc.z if hit else 0.0


cands = []
for r in data["roads"]:
    for p0, q0 in zip(r["pts"], r["pts"][1:]):
        p0 = Vector((p0[0] + ox, p0[1] + oy, 0)); q0 = Vector((q0[0] + ox, q0[1] + oy, 0))
        seg = q0 - p0
        if seg.length < 5 or ((p0 + q0) / 2 - Vector((ox, oy, 0))).length > 200:
            continue
        d = seg.normalized(); s = Vector((-d.y, d.x, 0))
        for k in range(0, int(seg.length), 4):
            x, y = (p0 + d * k).x, (p0 + d * k).y
            o = Vector((x, y, ground_z(x, y) + 1.7))
            fwd = free(o, d, 120); l = free(o, s, 30); rr = free(o, -s, 30)
            if min(l, rr) < 1.2:
                continue
            cands.append((fwd - 2 * abs(l - rr) - 0.5 * (l + rr), o + s * (l - rr) / 2, d, s, r["type"]))
cands.sort(key=lambda c: -c[0])
picked = []
for c in cands:
    if all((c[1] - p[1]).length > 60 for p in picked):
        picked.append(c)
    if len(picked) == 3:
        break


def ue_rot(v):
    yaw = math.degrees(math.atan2(-v.y, v.x))
    pitch = math.degrees(math.asin(max(-1, min(1, v.z / v.length))))
    return yaw, pitch


cams = []
for i, (_, o, d, s, typ) in enumerate(picked):
    look = (d * 40 + Vector((0, 0, 3.0)))
    yaw, pitch = ue_rot(look)
    cams.append({"name": f"Street{i + 1}_{typ}", "location_m": [o.x, o.y, o.z], "yaw": yaw, "pitch": pitch,
                 "focal_mm": 18.0})
    up = d * 10 + s * 4 + Vector((0, 0, 8))
    yaw, pitch = ue_rot(up)
    cams.append({"name": f"Street{i + 1}_LookUp", "location_m": [o.x, o.y, o.z], "yaw": yaw, "pitch": pitch,
                 "focal_mm": 14.0})
c = Vector((ox, oy, 0))
aer = Vector((ox - 180, oy - 180, 130))
yaw, pitch = ue_rot(c - aer)
cams.append({"name": "Aerial", "location_m": [aer.x, aer.y, aer.z], "yaw": yaw, "pitch": pitch, "focal_mm": 28.0})
json.dump(cams, open(OUT, "w"), indent=1)
print("CAMERAS", len(cams), [x["name"] for x in cams])
