"""Render street-level + aerial previews of the Lyari slice.
blender --background lyari_slice_detailed.blend --python lyari_render.py -- <slice.json> <out_prefix>"""
import json, math, sys
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
data = json.load(open(argv[0])); pre = argv[1]
scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1600, 900
try:
    scene.eevee.use_shadows = True
    scene.eevee.use_raytracing = True
except AttributeError:
    pass
scene.view_settings.view_transform = "AgX"
cam = scene.camera

# pick a street spot with a clear view: ray-cast forward (long) and sideways (buildings close)
dg = bpy.context.evaluated_depsgraph_get()


def free(o, dvec, maxd):
    hit, loc, *_ = scene.ray_cast(dg, o, dvec, distance=maxd)
    return (loc - o).length if hit else maxd


best = None
for r in data["roads"]:
    for p0, q0 in zip(r["pts"], r["pts"][1:]):
        p0, q0 = Vector((*p0, 0)), Vector((*q0, 0))
        seg = q0 - p0
        if seg.length < 5 or ((p0 + q0) / 2).length > 120:
            continue
        dd = seg.normalized(); sd = Vector((-dd.y, dd.x, 0))
        for k in range(0, int(seg.length), 3):
            o = p0 + dd * k + Vector((0, 0, 1.7))
            fwd = free(o, dd, 80)
            l, rr = free(o, sd, 30), free(o, -sd, 30)
            if l < 1.0 or rr < 1.0:
                continue
            score = fwd - 2 * abs(l - rr) - (l + rr)  # long view, canyon-like street, centred
            if best is None or score > best[0]:
                best = (score, o.copy(), dd, sd, fwd, l, rr)
_, p, d, side, fwd, l, rr = best
print("street spot", p, "forward clear", fwd, "sides", l, rr)
p = p - Vector((0, 0, 1.7)) + side * (l - rr) / 2


def shot(name, loc, target, lens):
    cam.location = loc
    cam.rotation_euler = (target - loc).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = lens
    scene.render.filepath = f"{pre}_{name}.png"
    bpy.ops.render.render(write_still=True)


shot("street", p + Vector((0, 0, 1.7)), p + d * 40 + Vector((0, 0, 3)), 20)
shot("street_up", p + Vector((0, 0, 1.7)), p + d * 10 + side * 4 + Vector((0, 0, 8)), 16)
shot("aerial", Vector((-170, -170, 120)), Vector((0, 0, 0)), 35)
print("RENDERED", best[0])
