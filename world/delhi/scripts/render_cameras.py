"""Render cameras.json bookmarks of a slice in Blender (EEVEE). blender -b <slice.blend> --python render_cameras.py -- <cameras.json> <out_prefix>"""
import json, math, sys
import bpy
from mathutils import Vector
argv = sys.argv[sys.argv.index("--") + 1:]
cams = json.load(open(argv[0])); pre = argv[1]
sc = bpy.context.scene
sc.render.engine = "BLENDER_EEVEE"; sc.render.resolution_x, sc.render.resolution_y = 1600, 900
sc.view_settings.view_transform = "AgX"
cam = sc.camera
for c in cams:
    if c["name"] not in ("Street1_residential", "Street2_residential", "Street3_tertiary", "Street1_LookUp", "Aerial"):
        continue
    y, p = math.radians(c["yaw"]), math.radians(c["pitch"])
    d = Vector((math.cos(p) * math.cos(y), -math.cos(p) * math.sin(y), math.sin(p)))
    cam.location = Vector(c["location_m"])
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = c["focal_mm"]; cam.data.clip_start = 0.1; cam.data.clip_end = 5000
    sc.render.filepath = f"{pre}_{c['name']}.png"
    bpy.ops.render.render(write_still=True)
print("RENDERED")
