"""GUI startup helper: Shading workspace, Material Preview in every 3D view, street-level camera
view, and the nearest building selected so its shader nodes show in the Shader Editor.
blender <file.blend> --python open_textured.py"""
import math

import bpy
from mathutils import Vector

STREET = Vector((-15.8, 113.8, 1.7))  # clear lane found by the ray-cast search in lyari_render.py


def stage1():
    win = bpy.context.window_manager.windows[0]
    ws = bpy.data.workspaces.get("Shading")
    if ws:
        win.workspace = ws
    scene = bpy.context.scene
    cam = scene.camera
    dg = bpy.context.evaluated_depsgraph_get()
    best = None
    for ang in range(0, 360, 10):
        d = Vector((math.cos(math.radians(ang)), math.sin(math.radians(ang)), 0))
        hit, loc, *_ = scene.ray_cast(dg, STREET, d, distance=200)
        dist = (loc - STREET).length if hit else 200
        if best is None or dist > best[0]:
            best = (dist, d)
    cam.location = STREET
    tgt = STREET + best[1] * 40 + Vector((0, 0, 2.5))
    cam.rotation_euler = (tgt - STREET).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 20
    # select the building nearest the camera so the Shader Editor shows its material
    near = min((o for o in scene.objects if o.name.startswith("Bldg_")),
               key=lambda o: (o.matrix_world @ Vector(o.bound_box[0]) - STREET).length)
    for o in scene.objects:
        o.select_set(False)
    near.select_set(True)
    bpy.context.view_layer.objects.active = near
    near.active_material_index = 0
    return None


def stage2():
    win = bpy.context.window_manager.windows[0]
    for area in win.screen.areas:
        if area.type != "VIEW_3D":
            continue
        sp = area.spaces.active
        sp.shading.type = "MATERIAL"
        sp.shading.use_scene_lights = True
        sp.shading.use_scene_world = True
        sp.clip_start = 0.05; sp.clip_end = 3000
        # free-fly perspective at eye height (not locked to the camera, so you can navigate)
        cam = bpy.context.scene.camera
        r3d = sp.region_3d
        r3d.view_perspective = "PERSP"
        fwd = cam.matrix_world.to_quaternion() @ Vector((0, 0, -1))
        r3d.view_distance = 3.0
        r3d.view_rotation = cam.matrix_world.to_quaternion()
        r3d.view_location = cam.location + fwd * 3.0
        sp.lens = 24
    # frame the shader nodes
    for area in win.screen.areas:
        if area.type == "NODE_EDITOR":
            region = next(r for r in area.regions if r.type == "WINDOW")
            with bpy.context.temp_override(window=win, area=area, region=region):
                bpy.ops.node.view_all()
    return None


bpy.app.timers.register(stage1, first_interval=1.0)
bpy.app.timers.register(stage2, first_interval=2.5)
