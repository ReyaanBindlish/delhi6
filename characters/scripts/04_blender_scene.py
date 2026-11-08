"""Assemble both textured characters in one .blend, export FBX for Unreal, render a check image.

blender --background --python 04_blender_scene.py
"""
import math
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent.parent
NAMES = ["set1_casual", "set2_traditional"]

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"

for i, name in enumerate(NAMES):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(ROOT / "meshes" / f"{name}_textured.glb"))
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    ob = new[0]
    ob.name = name
    ob.rotation_mode = "XYZ"  # glTF importer uses quaternions
    # Hunyuan normalises to ~2 units tall; scale to a 1.78 m adult
    h = ob.dimensions.z
    ob.scale = [1.78 / h] * 3
    bpy.context.view_layer.update()
    ob.location.x = (i - 0.5) * 1.4
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.shade_smooth()
    # FBX for Unreal (cm units handled by apply_unit_scale; -Y forward is UE's default import)
    (ROOT / "export").mkdir(exist_ok=True)
    loc = ob.location.copy(); ob.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=str(ROOT / "export" / f"{name}.fbx"), use_selection=True,
                             apply_unit_scale=True, path_mode="COPY", embed_textures=True,
                             axis_forward="-Y", axis_up="Z")
    bpy.ops.export_scene.gltf(filepath=str(ROOT / "export" / f"{name}.glb"), use_selection=True)
    ob.location = loc

# remove empties the glTF importer adds
for o in list(bpy.data.objects):
    if o.type == "EMPTY":
        bpy.data.objects.remove(o)

# floor, lights, camera
bpy.ops.mesh.primitive_plane_add(size=8)
floor = bpy.context.active_object; floor.name = "Floor"
fm = bpy.data.materials.new("Floor"); fm.diffuse_color = (0.35, 0.35, 0.35, 1)
floor.data.materials.append(fm)
for loc, energy in (((3, -4, 5), 600), ((-4, -2, 3), 250), ((0, 4, 4), 400)):
    L = bpy.data.objects.new("Light", bpy.data.lights.new("Light", "AREA"))
    L.data.energy = energy; L.data.size = 3; L.location = loc
    d = -L.location
    L.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(L)
world = bpy.data.worlds.new("World"); scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.2, 0.2, 0.22, 1)
cam = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
cam.location = (0, -5.2, 1.1); cam.rotation_euler = (math.radians(88), 0, 0)
cam.data.lens = 40
scene.collection.objects.link(cam); scene.camera = cam

scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1400, 900
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "characters.blend"))


def shot(path, angle):
    for n in NAMES:
        bpy.data.objects[n].rotation_euler.z = math.radians(angle)
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


shot(ROOT / "preview_front.png", 0)
shot(ROOT / "preview_back.png", 180)
shot(ROOT / "preview_side.png", 90)
for n in NAMES:
    bpy.data.objects[n].rotation_euler.z = 0
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "characters.blend"))
print("SCENE DONE")
