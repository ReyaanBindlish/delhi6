"""Quick Workbench previews of a built location (top-down + oblique).

blender --background export/<loc>/_work/<loc>.blend --python render_preview.py -- <out_dir> [cx cy cz yaw pitch ortho_scale]
"""
import math
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
out = argv[0]
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"
sc.render.resolution_x, sc.render.resolution_y = 1600, 1200
sc.display.shading.light = "STUDIO"
sc.display.shading.color_type = "MATERIAL"
for o in bpy.data.objects:
    if o.name.startswith("UCX_") or o.name == "TerrainPreview":
        o.hide_render = True
cam = sc.camera
cam.data.type = "ORTHO"; cam.data.ortho_scale = float(argv[6]) if len(argv) > 6 else 900
cam.location = Vector((float(argv[1]) if len(argv) > 1 else 0, float(argv[2]) if len(argv) > 2 else 0, 600))
cam.rotation_euler = (0, 0, 0)
cam.data.clip_end = 5000
sc.render.filepath = out + "_top.png"
bpy.ops.render.render(write_still=True)
cam.data.type = "PERSP"; cam.data.lens = 24
cx, cy = (float(argv[1]), float(argv[2])) if len(argv) > 2 else (0, 0)
yaw = math.radians(float(argv[4])) if len(argv) > 4 else math.radians(30)
cam.location = Vector((cx - 140 * math.sin(yaw), cy - 140 * math.cos(yaw), float(argv[3]) if len(argv) > 3 else 40))
cam.rotation_euler = (math.radians(float(argv[5]) if len(argv) > 5 else 72), 0, yaw)
sc.render.filepath = out + "_obl.png"
bpy.ops.render.render(write_still=True)
