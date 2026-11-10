"""Editor-time setup for the ambient crowd (run once, safe to re-run).

  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file>" -unattended -nop4

Needs the OldDelhi module compiled (row structs, UDhCrowdAgentTrait) and, for the mannequin copy, one of:
  * /Game/Characters/Mannequins   (Third Person template content, the player agent usually brings it), or
  * the engine's "Mover Examples" plugin content (/MoverExamples/...)  [Edit > Plugins > "Mover Examples"].

What it does (each step is independent and reports instead of failing):
  1. DataTables  /Game/Crowd/Data   from Data/DT_Archetypes_Karachi.csv, DT_Archetypes_DelhiNCR.csv,
                                    DT_StreetDensity_Story.csv (and DT_Archetypes.csv for Old Delhi)
  2. Mannequin   /Game/Crowd/Mannequin  Manny (and Quinn when present) mesh + walk / idle loops
  3. Materials   /Game/Crowd/Materials  M_CrowdSkeletal + MI_CrowdSkeletal (BodyTint / SkinTone),
                                    M_CrowdInstanced + MI_CrowdInstanced (per-instance custom data 0-3)
  4. Proxy mesh  /Game/Crowd/Meshes/SM_CrowdProxy  (170 cm person-shaped low-poly, pivot at the feet; Blender 5.2)
  5. Mass config /Game/Crowd/Mass/DA_DhCrowdEntityConfig  with UDhCrowdAgentTrait + MassCrowd visualisation
                                    (best effort; see docs/CROWD.md for the manual recipe)
Lane JSON is separate: Tools/ue/build_zonegraph.py (lanes mode).
"""
import os
import subprocess
import tempfile
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[3]
PROJECT = REPO / "OldDelhi"
DATA = PROJECT / "Data"
G = "/Game/Crowd"
BLENDER = Path(os.environ.get("BLENDER", r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"))

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def log(*a):
    unreal.log("[crowd-setup] " + " ".join(str(x) for x in a))


def warn(*a):
    unreal.log_warning("[crowd-setup] " + " ".join(str(x) for x in a))


def setp(obj, names, value):
    for n in names if isinstance(names, (list, tuple)) else [names]:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception:
            continue
    warn("could not set", names, "on", obj.get_class().get_name())
    return False


def ensure_dir(p):
    if not EAL.does_directory_exist(p):
        EAL.make_directory(p)


# ------------------------------------------------------------------ 1. data tables
def import_table(csv_name, asset_name, struct_path):
    dest = f"{G}/Data"
    if EAL.does_asset_exist(f"{dest}/{asset_name}"):
        log("exists:", asset_name)
        return True
    csv_path = DATA / csv_name
    if not csv_path.exists():
        warn("missing CSV", csv_path)
        return False
    try:
        struct = unreal.load_object(None, struct_path)
        factory = unreal.CSVImportFactory()
        settings = factory.get_editor_property("automated_import_settings")
        settings.set_editor_property("import_row_struct", struct)
        settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)
        task = unreal.AssetImportTask()
        task.filename = str(csv_path)
        task.destination_path = dest
        task.destination_name = asset_name
        task.factory = factory
        task.automated = True
        task.save = True
        task.replace_existing = True
        AT.import_asset_tasks([task])
        ok = EAL.does_asset_exist(f"{dest}/{asset_name}")
        log("DataTable", asset_name, "OK" if ok else "FAILED")
        return ok
    except Exception as e:  # noqa: BLE001
        warn("DataTable import failed for", csv_name, e, "(runtime falls back to the CSV in Data/)")
        return False


def step_tables():
    ensure_dir(f"{G}/Data")
    row = "/Script/OldDelhi.DelhiArchetypeRow"
    dens = "/Script/OldDelhi.DelhiStreetDensityRow"
    import_table("DT_Archetypes_Karachi.csv", "DT_Archetypes_Karachi", row)
    import_table("DT_Archetypes_DelhiNCR.csv", "DT_Archetypes_DelhiNCR", row)
    import_table("DT_StreetDensity_Story.csv", "DT_StreetDensity_Story", dens)
    import_table("DT_Archetypes.csv", "DT_Archetypes_OldDelhi", row)


# ------------------------------------------------------------------ 2. mannequins
MANNY_SOURCES = [
    "/Game/Characters/Mannequins",
    "/MoverExamples/Characters/Mannequins",
    "/MoverTests/Characters/Mannequins",
]
COPY = [  # (relative source, destination name)
    ("Meshes/SKM_Manny_Simple", "SKM_Manny_Simple"),
    ("Meshes/SKM_Quinn_Simple", "SKM_Quinn_Simple"),
    ("Animations/Manny/MM_Walk_InPlace", "MM_Walk_InPlace"),
    ("Animations/Manny/MM_Idle", "MM_Idle"),
]


def step_mannequins():
    dest = f"{G}/Mannequin"
    ensure_dir(dest)
    for root in MANNY_SOURCES:
        if not EAL.does_asset_exist(f"{root}/Meshes/SKM_Manny_Simple"):
            continue
        log("mannequin source:", root)
        for rel, name in COPY:
            src = f"{root}/{rel}"
            dst = f"{dest}/{name}"
            if EAL.does_asset_exist(dst):
                continue
            if EAL.does_asset_exist(src):
                if EAL.duplicate_asset(src, dst):
                    EAL.save_asset(dst)
                    log("copied", name)
                else:
                    warn("duplicate failed", src)
        warn("note: the copies still reference the source skeleton/materials; keep", root,
             "available, or Migrate the mannequin folder properly from the Content Browser")
        return True
    warn("no Manny/Quinn source found. Enable the 'Mover Examples' plugin or add the Third Person template content, "
         "then re-run. Settings candidates already include those paths, so the crowd may still find them.")
    return False


# ------------------------------------------------------------------ 3. materials
def make_skeletal_master():
    path = f"{G}/Materials/M_CrowdSkeletal"
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    m = AT.create_asset("M_CrowdSkeletal", f"{G}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    tint = E(unreal.MaterialExpressionVectorParameter, -700, -100)
    setp(tint, "parameter_name", "BodyTint")
    setp(tint, "default_value", unreal.LinearColor(0.35, 0.3, 0.25, 1))
    skin = E(unreal.MaterialExpressionScalarParameter, -700, 100)
    setp(skin, "parameter_name", "SkinTone")
    setp(skin, "default_value", 0.5)
    # brightness variation 0.85..1.15 from SkinTone (subtle on a single-colour stand-in)
    k = E(unreal.MaterialExpressionConstant, -700, 200); setp(k, "r", 0.3)
    mul_k = E(unreal.MaterialExpressionMultiply, -500, 150)
    MEL.connect_material_expressions(skin, "", mul_k, "A"); MEL.connect_material_expressions(k, "", mul_k, "B")
    base_k = E(unreal.MaterialExpressionConstant, -500, 250); setp(base_k, "r", 0.85)
    add = E(unreal.MaterialExpressionAdd, -300, 200)
    MEL.connect_material_expressions(mul_k, "", add, "A"); MEL.connect_material_expressions(base_k, "", add, "B")
    out = E(unreal.MaterialExpressionMultiply, -150, 0)
    MEL.connect_material_expressions(tint, "", out, "A"); MEL.connect_material_expressions(add, "", out, "B")
    rough = E(unreal.MaterialExpressionConstant, -300, 350); setp(rough, "r", 0.72)
    MEL.connect_material_property(out, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    setp(m, "used_with_skeletal_mesh", True)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def make_instanced_master():
    path = f"{G}/Materials/M_CrowdInstanced"
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    m = AT.create_asset("M_CrowdInstanced", f"{G}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    chans = []
    for i in range(4):
        c = E(unreal.MaterialExpressionPerInstanceCustomData, -900, -200 + 120 * i)
        setp(c, "data_index", i)
        setp(c, "const_default_value", 0.5)
        chans.append(c)
    a1 = E(unreal.MaterialExpressionAppendVector, -650, -150)
    MEL.connect_material_expressions(chans[0], "", a1, "A"); MEL.connect_material_expressions(chans[1], "", a1, "B")
    a2 = E(unreal.MaterialExpressionAppendVector, -450, -120)
    MEL.connect_material_expressions(a1, "", a2, "A"); MEL.connect_material_expressions(chans[2], "", a2, "B")
    k = E(unreal.MaterialExpressionConstant, -650, 150); setp(k, "r", 0.3)
    mk = E(unreal.MaterialExpressionMultiply, -450, 120)
    MEL.connect_material_expressions(chans[3], "", mk, "A"); MEL.connect_material_expressions(k, "", mk, "B")
    b = E(unreal.MaterialExpressionConstant, -450, 220); setp(b, "r", 0.85)
    br = E(unreal.MaterialExpressionAdd, -250, 150)
    MEL.connect_material_expressions(mk, "", br, "A"); MEL.connect_material_expressions(b, "", br, "B")
    out = E(unreal.MaterialExpressionMultiply, -80, -50)
    MEL.connect_material_expressions(a2, "", out, "A"); MEL.connect_material_expressions(br, "", out, "B")
    rough = E(unreal.MaterialExpressionConstant, -250, 300); setp(rough, "r", 0.75)
    MEL.connect_material_property(out, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    setp(m, "used_with_instanced_static_meshes", True)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def make_mi(name, parent):
    path = f"{G}/Materials/{name}"
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(
        name, f"{G}/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    EAL.save_loaded_asset(mi)
    return mi


def step_materials():
    ensure_dir(f"{G}/Materials")
    try:
        make_mi("MI_CrowdSkeletal", make_skeletal_master())
        make_mi("MI_CrowdInstanced", make_instanced_master())
        log("materials ready")
    except Exception as e:  # noqa: BLE001
        warn("material setup failed:", e)


# ------------------------------------------------------------------ 4. proxy mesh
BLENDER_SCRIPT = r'''
import bpy, bmesh, sys
from mathutils import Matrix
out = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bm = bmesh.new()
bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=10, radius1=0.22, radius2=0.17, depth=1.45,
                      matrix=Matrix.Translation((0, 0, 0.725)))
bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=0.11, matrix=Matrix.Translation((0, 0, 1.59)))
me = bpy.data.meshes.new("SM_CrowdProxy"); bm.to_mesh(me); bm.free()
ob = bpy.data.objects.new("SM_CrowdProxy", me); bpy.context.scene.collection.objects.link(ob)
bpy.ops.export_scene.fbx(filepath=out, use_selection=False, object_types={"MESH"}, apply_unit_scale=True, global_scale=1.0,
                         axis_forward="-Y", axis_up="Z")
'''


def step_proxy_mesh():
    dest = f"{G}/Meshes"
    ensure_dir(dest)
    if EAL.does_asset_exist(f"{dest}/SM_CrowdProxy"):
        log("exists: SM_CrowdProxy")
        return
    if not BLENDER.exists():
        warn("Blender not found; skipping SM_CrowdProxy (the cylinder fallback is used)")
        return
    src_dir = PROJECT / "Content" / "Crowd" / "Source"
    src_dir.mkdir(parents=True, exist_ok=True)
    fbx = src_dir / "SM_CrowdProxy.fbx"
    with tempfile.NamedTemporaryFile("w", suffix=".py", delete=False) as f:
        f.write(BLENDER_SCRIPT)
        script = f.name
    try:
        subprocess.run([str(BLENDER), "-b", "--factory-startup", "-P", script, "--", str(fbx)], check=True, timeout=180)
    except Exception as e:  # noqa: BLE001
        warn("Blender proxy export failed:", e)
        return
    finally:
        os.unlink(script)
    ui = unreal.FbxImportUI()
    setp(ui, "import_mesh", True); setp(ui, "import_materials", False); setp(ui, "import_textures", False)
    setp(ui, "import_as_skeletal", False)
    setp(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    d = ui.get_editor_property("static_mesh_import_data")
    setp(d, "combine_meshes", True); setp(d, "auto_generate_collision", False)
    setp(d, "generate_lightmap_u_vs", False); setp(d, "build_nanite", False)
    t = unreal.AssetImportTask()
    t.filename = str(fbx); t.destination_path = dest; t.destination_name = "SM_CrowdProxy"
    t.automated = True; t.save = True; t.replace_existing = True; t.options = ui
    AT.import_asset_tasks([t])
    mesh = unreal.load_asset(f"{dest}/SM_CrowdProxy")
    mi = unreal.load_asset(f"{G}/Materials/MI_CrowdInstanced")
    if mesh and mi:
        mesh.set_material(0, mi)
        EAL.save_loaded_asset(mesh)
    log("SM_CrowdProxy imported" if mesh else "SM_CrowdProxy import FAILED")


# ------------------------------------------------------------------ 5. Mass entity config
def find_class(*names):
    for n in names:
        c = getattr(unreal, n, None)
        if c is not None:
            return c
    return None


def step_mass_config():
    dest = f"{G}/Mass"
    ensure_dir(dest)
    path = f"{dest}/DA_DhCrowdEntityConfig"
    if EAL.does_asset_exist(path):
        log("exists: DA_DhCrowdEntityConfig")
        return
    asset_cls = find_class("MassEntityConfigAsset")
    agent_trait = find_class("DhCrowdAgentTrait")
    if asset_cls is None or agent_trait is None:
        warn("MassEntityConfigAsset / DhCrowdAgentTrait not exposed to Python. Enable the MassGameplay, MassCrowd and "
             "MassSimulation plugins, compile the project, then re-run. Manual recipe is in docs/CROWD.md.")
        return
    try:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", asset_cls)
        asset = AT.create_asset("DA_DhCrowdEntityConfig", dest, asset_cls, factory)
        cfg = asset.get_editor_property("config")
        traits = [unreal.new_object(agent_trait, outer=asset)]
        # MassCrowd visualisation is what draws the entities (stand-in static mesh now, VAT / MetaHuman crowd later)
        for name in ("MassCrowdVisualizationTrait", "MassCrowdMemberTrait"):
            c = find_class(name)
            if c is not None:
                traits.append(unreal.new_object(c, outer=asset))
            else:
                warn("trait class not exposed:", name)
        cfg.set_editor_property("traits", traits)
        asset.set_editor_property("config", cfg)
        EAL.save_loaded_asset(asset)
        log("Mass entity config created with", len(traits), "traits. Open it and set the visualization mesh (SM_CrowdProxy "
            "or a VAT mesh) under Crowd Visualization > Static Mesh Instances.")
    except Exception as e:  # noqa: BLE001
        warn("Mass config creation failed:", e, "(runtime falls back to the pooled backend)")


def main():
    log("project", PROJECT)
    for step in (step_tables, step_mannequins, step_materials, step_proxy_mesh, step_mass_config):
        try:
            step()
        except Exception as e:  # noqa: BLE001
            warn(step.__name__, "failed:", e)
    log("done. Next: run Tools/ue/build_zonegraph.py (lanes mode) for each location, then PIE and use "
        "'dh.crowd.stats' / 'dh.crowd.warm' in the console.")


main()
