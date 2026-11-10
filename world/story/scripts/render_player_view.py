"""Render a third-person 'player view' of a generated location with Daya on screen.

blender -b <location.blend> --python render_player_view.py -- <daya.glb> <out.png> [seed]
Finds a lane with a long clear view (ray casts), stands Daya in it, puts the camera over his right
shoulder (2.4 m back, 0.55 m right, 1.65 m up), golden low sun + humid haze, renders EEVEE.
"""
import math
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
GLB, OUT = argv[0], argv[1]
SEED = int(argv[2]) if len(argv) > 2 else 0
scene = bpy.context.scene
# collision hulls are invisible but block ray casts: remove them for this render
for o in [o for o in scene.objects if o.name.startswith("UCX_")]:
    bpy.data.objects.remove(o, do_unlink=True)
dg = bpy.context.evaluated_depsgraph_get()
GROUND_NAMES = ("Terrain", "Road", "Ground", "Platform", "Footway")

# ---- bounds of the buildings, to search near the middle
blds = [o for o in scene.objects if o.type == "MESH" and (o.name.startswith("Bldg") or o.name.startswith("B_"))]
if not blds:
    blds = [o for o in scene.objects if o.type == "MESH"]
xs = [o.location.x for o in blds]; ys = [o.location.y for o in blds]
cx, cy = sum(xs) / len(xs), sum(ys) / len(ys)


def cast(o, d, dist):
    hit, loc, *_ = scene.ray_cast(dg, o, d, distance=dist)
    return (loc - o).length if hit else dist


def ground(x, y):
    """Height of walkable ground at x,y, or None if the first thing below the sky is a building."""
    hit, loc, _n, _i, obj, _m = scene.ray_cast(dg, Vector((x, y, 500)), Vector((0, 0, -1)), distance=1000)
    if not hit:
        return 0.0
    return loc.z if obj.name.startswith(GROUND_NAMES) else None


best = None
step = 6
for gx in range(-150, 151, step):
    for gy in range(-150, 151, step):
        x, y = cx + gx, cy + gy
        z = ground(x, y)
        if z is None:
            continue                      # on a roof
        o = Vector((x, y, z + 1.6))
        # must be outdoors: nothing above
        hit_up, *_ = scene.ray_cast(dg, o, Vector((0, 0, 1)), distance=60)
        if hit_up:
            continue
        for k in range(8):
            a = math.radians(k * 45 + (SEED * 17) % 45)
            d = Vector((math.cos(a), math.sin(a), 0))
            s = Vector((-d.y, d.x, 0))
            fwd = cast(o, d, 120)
            l, r = cast(o, s, 25), cast(o, -s, 25)
            if fwd < 35 or min(l, r) < 1.5 or l + r > 16:
                continue                      # want a real lane: 3-16 m wide, long view
            score = fwd - 3 * abs(l - r) - (l + r) * 0.5 - 0.002 * (gx * gx + gy * gy)
            if best is None or score > best[0]:
                best = (score, Vector((x, y, z)), d, s, l, r)
_, P, d, s, l, r = best
P = P + s * (l - r) / 2            # centre of the lane
print(f"spot {P} lane width {l + r:.1f} m")

# ---- Daya
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=GLB)
daya = [o for o in bpy.data.objects if o not in before and o.type == "MESH"][0]
daya.rotation_mode = "XYZ"
h = daya.dimensions.z
daya.scale = [1.78 / h] * 3
bpy.context.view_layer.update()
daya.location = P + d * 3.0
gz = ground(daya.location.x, daya.location.y)
daya.location.z = gz if gz is not None else P.z
# glTF Daya faces -Y in Blender after import; rotate so he faces down the lane (away from camera)
yaw = math.atan2(d.y, d.x) - math.radians(-90)
daya.rotation_euler = (0, 0, yaw)

# ---- camera over the right shoulder
cam = scene.camera or bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
if cam.name not in scene.collection.objects and not cam.users_collection:
    scene.collection.objects.link(cam)
scene.camera = cam
head = daya.location + Vector((0, 0, 1.6))
cam.location = head - d * 2.4 - s * 0.55 + Vector((0, 0, 0.05))
target = head + d * 30 + Vector((0, 0, -0.8)) - s * 0.4
cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
cam.data.lens = 24
cam.data.clip_start = 0.05; cam.data.clip_end = 3000

# ---- light: low golden sun from behind-left, humid haze
for o in [o for o in scene.objects if o.type == "LIGHT"]:
    bpy.data.objects.remove(o)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 4.2; sun.data.color = (1.0, 0.86, 0.68); sun.data.angle = math.radians(1.2)
sa = math.atan2(d.y, d.x) + math.radians(150)
sun.rotation_euler = (math.radians(72), 0, sa + math.radians(90))
scene.collection.objects.link(sun)
world = scene.world or bpy.data.worlds.new("World"); scene.world = world; world.use_nodes = True
nt = world.node_tree
bg = nt.nodes.get("Background")
bg.inputs["Color"].default_value = (0.78, 0.74, 0.66, 1); bg.inputs["Strength"].default_value = 0.9
# (World volumes black out EEVEE in Blender 5.x; haze is added as depth-based mist in compositing below.)
scene.view_layers[0].use_pass_mist = True
scene.world.mist_settings.start = 15.0
scene.world.mist_settings.depth = 220.0
scene.world.mist_settings.falloff = "QUADRATIC"

scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1920, 1080
scene.view_settings.view_transform = "AgX"
try:
    scene.view_settings.look = "AgX - Medium High Contrast"
except Exception:
    pass
for attr, val in (("use_raytracing", True), ("use_shadows", True), ("volumetric_tile_size", "4"),
                  ("volumetric_end", 400.0), ("use_volumetric_shadows", True)):
    try:
        setattr(scene.eevee, attr, val)
    except Exception:
        pass
scene.render.filepath = OUT
bpy.ops.render.render(write_still=True)
# save the mist pass alongside for the haze composite (done in Python/PIL afterwards)
mist_out = OUT.replace(".png", "_mist.exr")
try:
    rr = bpy.data.images["Render Result"]
    rr.save_render(filepath=mist_out)
except Exception as e:
    print("mist save skipped", e)
print("RENDERED", OUT)
