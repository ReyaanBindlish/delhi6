"""Build the Old Delhi vertical slice level inside Unreal Engine 5.8 (headless or in-editor).

  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file>" -unattended -nop4

Steps: import CC0 textures -> master material + MPC_Weather -> material instances -> import terrain tiles,
1,329 building FBX (with UCX collision, Nanite), roads, overhead wires -> new level placed from the
manifests -> sun / sky / Delhi haze / exposure -> camera bookmarks -> save.
World frame: Blender (X east, Y north, Z up, metres) -> Unreal (X east, Y south, Z up, cm).
"""
import json
import os
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[3]
TEX_DIR = REPO / "world" / "textures_polyhaven_cc0"
SLICE_DIR = REPO / "world" / "delhi" / "export" / "old_delhi_vs"
TERRAIN_DIR = REPO / "world" / "delhi" / "export" / "terrain"
CAMERAS = SLICE_DIR / "cameras.json"
LIMIT = int(os.environ.get("DELHI6_LIMIT", "0"))  # import only N buildings when testing

G = "/Game/Delhi"
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ACT = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LVL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(*a):
    unreal.log("[delhi6] " + " ".join(str(x) for x in a))


def setp(obj, names, value):
    """Set the first property name that exists (API names drift between engine versions)."""
    for n in names if isinstance(names, (list, tuple)) else [names]:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception:
            continue
    log("WARN could not set", names, "on", obj.get_class().get_name())
    return False


def to_ue(m):
    return unreal.Vector(m[0] * 100.0, -m[1] * 100.0, m[2] * 100.0)


# ------------------------------------------------------------------ textures
def import_textures():
    tasks = []
    for f in sorted(TEX_DIR.glob("*.jpg")):
        if EAL.does_asset_exist(f"{G}/Textures/T_{f.stem}"):
            continue
        t = unreal.AssetImportTask()
        t.filename = str(f); t.destination_path = f"{G}/Textures"; t.destination_name = f"T_{f.stem}"
        t.automated = True; t.save = True; t.replace_existing = True
        tasks.append(t)
    if tasks:
        AT.import_asset_tasks(tasks)
    for f in TEX_DIR.glob("*.jpg"):
        tex = unreal.load_asset(f"{G}/Textures/T_{f.stem}")
        if tex is None:
            continue
        if f.stem.endswith("_nor"):
            setp(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            setp(tex, "srgb", False)
        elif f.stem.endswith("_rough"):
            setp(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            setp(tex, "srgb", False)
        EAL.save_loaded_asset(tex)
    log("textures ready")


# ------------------------------------------------------------------ MPC + master material
def make_mpc():
    path = f"{G}/Materials/MPC_Weather"
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    mpc = AT.create_asset("MPC_Weather", f"{G}/Materials", unreal.MaterialParameterCollection,
                          unreal.MaterialParameterCollectionFactoryNew())
    params = []
    for name, val in (("Wetness", 0.0), ("Dust", 0.25), ("Smog", 0.45), ("WindX", 1.4), ("WindY", 1.4),
                      ("WindStrength", 2.0)):
        p = unreal.CollectionScalarParameter()
        setp(p, "parameter_name", name); setp(p, "default_value", val)
        params.append(p)
    setp(mpc, "scalar_parameters", params)
    EAL.save_loaded_asset(mpc)
    return mpc


def make_master(mpc):
    path = f"{G}/Materials/M_Delhi_Master"
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    m = AT.create_asset("M_Delhi_Master", f"{G}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)

    base = E(unreal.MaterialExpressionTextureSampleParameter2D, -1400, -300)
    setp(base, "parameter_name", "BaseColorTex")
    setp(base, "texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    nrm = E(unreal.MaterialExpressionTextureSampleParameter2D, -1400, 300)
    setp(nrm, "parameter_name", "NormalTex")
    setp(nrm, "texture", unreal.load_asset("/Engine/EngineMaterials/DefaultNormal"))
    setp(nrm, "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    rgh = E(unreal.MaterialExpressionTextureSampleParameter2D, -1400, 0)
    setp(rgh, "parameter_name", "RoughTex")
    setp(rgh, "texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    setp(rgh, "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

    tint = E(unreal.MaterialExpressionVectorParameter, -1100, -500)
    setp(tint, "parameter_name", "Tint"); setp(tint, "default_value", unreal.LinearColor(1, 1, 1, 1))
    usev = E(unreal.MaterialExpressionScalarParameter, -1100, -380)
    setp(usev, "parameter_name", "UseVertexTint"); setp(usev, "default_value", 0.0)
    vcol = E(unreal.MaterialExpressionVertexColor, -1100, -260)
    one = E(unreal.MaterialExpressionConstant3Vector, -1100, -160)
    setp(one, "constant", unreal.LinearColor(1, 1, 1, 1))
    vl = E(unreal.MaterialExpressionLinearInterpolate, -850, -300)
    MEL.connect_material_expressions(one, "", vl, "A")
    MEL.connect_material_expressions(vcol, "", vl, "B")
    MEL.connect_material_expressions(usev, "", vl, "Alpha")
    m1 = E(unreal.MaterialExpressionMultiply, -650, -350)
    MEL.connect_material_expressions(base, "RGB", m1, "A")
    MEL.connect_material_expressions(tint, "", m1, "B")
    m2 = E(unreal.MaterialExpressionMultiply, -450, -300)
    MEL.connect_material_expressions(m1, "", m2, "A")
    MEL.connect_material_expressions(vl, "", m2, "B")

    rmul = E(unreal.MaterialExpressionScalarParameter, -1100, 60)
    setp(rmul, "parameter_name", "RoughnessMul"); setp(rmul, "default_value", 1.0)
    r1 = E(unreal.MaterialExpressionMultiply, -850, 0)
    MEL.connect_material_expressions(rgh, "R", r1, "A")
    MEL.connect_material_expressions(rmul, "", r1, "B")
    metal = E(unreal.MaterialExpressionScalarParameter, -450, 150)
    setp(metal, "parameter_name", "Metallic"); setp(metal, "default_value", 0.0)

    base_out, rough_out = m2, r1
    # wetness from MPC_Weather: darken porous albedo ~30%, pull roughness toward 0.08
    try:
        wet = E(unreal.MaterialExpressionCollectionParameter, -1100, 200)
        setp(wet, "collection", mpc); setp(wet, "parameter_name", "Wetness")
        por = E(unreal.MaterialExpressionScalarParameter, -1100, 260)
        setp(por, "parameter_name", "Porosity"); setp(por, "default_value", 0.7)
        wp = E(unreal.MaterialExpressionMultiply, -850, 220)
        MEL.connect_material_expressions(wet, "", wp, "A"); MEL.connect_material_expressions(por, "", wp, "B")
        dark = E(unreal.MaterialExpressionConstant, -650, -150); setp(dark, "r", 0.7)
        onec = E(unreal.MaterialExpressionConstant, -650, -100); setp(onec, "r", 1.0)
        dl = E(unreal.MaterialExpressionLinearInterpolate, -450, -150)
        MEL.connect_material_expressions(onec, "", dl, "A"); MEL.connect_material_expressions(dark, "", dl, "B")
        MEL.connect_material_expressions(wp, "", dl, "Alpha")
        bw = E(unreal.MaterialExpressionMultiply, -250, -300)
        MEL.connect_material_expressions(m2, "", bw, "A"); MEL.connect_material_expressions(dl, "", bw, "B")
        wr = E(unreal.MaterialExpressionConstant, -650, 100); setp(wr, "r", 0.08)
        rl = E(unreal.MaterialExpressionLinearInterpolate, -450, 50)
        MEL.connect_material_expressions(r1, "", rl, "A"); MEL.connect_material_expressions(wr, "", rl, "B")
        MEL.connect_material_expressions(wp, "", rl, "Alpha")
        base_out, rough_out = bw, rl
    except Exception as e:
        log("WARN wetness hookup skipped:", e)

    MEL.connect_material_property(base_out, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough_out, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    setp(m, "used_with_nanite", True)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


# name -> (texture id or None, tint RGB, vertex tint?, roughness mul, metallic)
MI = {
    "M_PaintedPlaster": ("painted_plaster_wall", (1, 1, 1), 1, 1.0, 0.0),
    "M_DamagedPlaster": ("damaged_plaster", (1, 1, 1), 1, 1.0, 0.0),
    "M_Brick": ("brick_wall_001", (1, 1, 1), 0, 1.0, 0.0),
    "M_Concrete": ("concrete_wall_008", (1, 1, 1), 0, 1.0, 0.0),
    "M_RoofConcrete": ("concrete_floor_worn_001", (1, 1, 1), 0, 1.0, 0.0),
    "M_RustyMetal": ("rusty_metal", (0.3, 0.3, 0.32), 0, 1.0, 0.6),
    "M_RollingShutter": ("rusted_shutter", (1, 1, 1), 0, 1.0, 0.5),
    "M_Glass": (None, (0.04, 0.05, 0.06), 0, 0.05, 0.0),
    "M_PVCTank": (None, (0.02, 0.02, 0.02), 0, 0.45, 0.0),
    "M_Signboard": (None, (1, 1, 1), 1, 0.6, 0.0),
    "M_ACUnit": (None, (0.8, 0.8, 0.78), 0, 0.5, 0.0),
    "M_Cable": (None, (0.01, 0.01, 0.01), 0, 0.4, 0.0),
    "M_Asphalt": ("asphalt_02", (1, 1, 1), 0, 1.0, 0.0),
    "M_Ground": ("concrete_floor_worn_001", (0.85, 0.8, 0.72), 0, 1.0, 0.0),
}


def make_instances(master):
    out = {}
    for name, (tid, tint, vt, rmul, metal) in MI.items():
        path = f"{G}/Materials/MI_{name[2:]}"
        mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(
            f"MI_{name[2:]}", f"{G}/Materials", unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, master)
        if tid:
            for p, suf in (("BaseColorTex", "diff"), ("NormalTex", "nor"), ("RoughTex", "rough")):
                tex = unreal.load_asset(f"{G}/Textures/T_{tid}_{suf}")
                if tex:
                    MEL.set_material_instance_texture_parameter_value(mi, p, tex)
        MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
        MEL.set_material_instance_scalar_parameter_value(mi, "UseVertexTint", float(vt))
        MEL.set_material_instance_scalar_parameter_value(mi, "RoughnessMul", rmul)
        MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
        EAL.save_loaded_asset(mi)
        out[name] = mi
    log("material instances:", len(out))
    return out


# ------------------------------------------------------------------ meshes
def fbx_options(complex_collision=False):
    ui = unreal.FbxImportUI()
    setp(ui, "import_mesh", True); setp(ui, "import_textures", False); setp(ui, "import_materials", False)
    setp(ui, "import_as_skeletal", False)
    setp(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    d = ui.get_editor_property("static_mesh_import_data")
    setp(d, "combine_meshes", True)
    setp(d, "auto_generate_collision", False)
    setp(d, "one_convex_hull_per_ucx", True)
    setp(d, "generate_lightmap_u_vs", False)
    setp(d, "build_nanite", True)
    setp(d, "import_uniform_scale", 1.0)
    return ui


def import_fbx(files, dest):
    tasks = []
    for f in files:
        name = Path(f).stem
        if EAL.does_asset_exist(f"{dest}/{name}"):
            continue
        t = unreal.AssetImportTask()
        t.filename = str(f); t.destination_path = dest; t.destination_name = name
        t.automated = True; t.save = True; t.replace_existing = True
        t.options = fbx_options()
        tasks.append(t)
    for i in range(0, len(tasks), 100):
        AT.import_asset_tasks(tasks[i:i + 100])
        log(f"imported {min(i + 100, len(tasks))}/{len(tasks)} into {dest}")


def assign(mesh, mis, complex_as_simple=False):
    mats = mesh.get_editor_property("static_materials")
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property("material_slot_name"))
        key = slot.split(".")[0]
        if key in mis:
            mesh.set_material(i, mis[key])
    try:
        ns = mesh.get_editor_property("nanite_settings"); setp(ns, "enabled", True)
        mesh.set_editor_property("nanite_settings", ns)
    except Exception:
        pass
    if complex_as_simple:
        bs = mesh.get_editor_property("body_setup")
        if bs:
            setp(bs, "collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    EAL.save_loaded_asset(mesh)


# ------------------------------------------------------------------ level + lighting
def comp(actor, *names):
    for n in names:
        try:
            c = actor.get_editor_property(n)
            if c:
                return c
        except Exception:
            continue
    return None


def lighting():
    # Late-October afternoon, ~16:00 IST: sun ~25 deg up from WSW (az ~235) => light travels toward NE.
    sun = ACT.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 50000),
                                     unreal.Rotator(roll=0, pitch=-25, yaw=-35))
    sc = comp(sun, "directional_light_component", "light_component")
    setp(sc, "intensity", 45.0)                       # lux; haze-attenuated afternoon sun
    setp(sc, ["atmosphere_sun_light", "b_atmosphere_sun_light"], True)
    setp(sc, "light_source_angle", 0.53)
    setp(sc, "dynamic_shadow_distance_movable_light", 30000.0)
    sky = ACT.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    skc = comp(sky, "sky_atmosphere_component", "component")
    setp(skc, "mie_scattering_scale", 0.012)          # ~3x default -> Delhi haze
    setp(skc, "mie_absorption_scale", 0.006)
    setp(skc, "mie_anisotropy", 0.8)
    setp(skc, "rayleigh_scattering_scale", 0.03)
    sl = ACT.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000))
    slc = comp(sl, "light_component", "sky_light_component")
    setp(slc, "real_time_capture", True)
    setp(slc, "mobility", unreal.ComponentMobility.MOVABLE)
    fog = ACT.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = comp(fog, "component", "exponential_height_fog_component")
    setp(fc, "fog_density", 0.035)
    setp(fc, "fog_height_falloff", 0.08)
    setp(fc, ["fog_inscattering_luminance", "fog_inscattering_color"], unreal.LinearColor(0.55, 0.50, 0.40, 1))
    setp(fc, "volumetric_fog", True)
    setp(fc, "volumetric_fog_albedo", unreal.Color(r=200, g=190, b=175, a=255))
    setp(fc, "volumetric_fog_extinction_scale", 1.5)
    ppv = ACT.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    setp(ppv, "unbound", True)
    s = ppv.get_editor_property("settings")
    for k, v in (("auto_exposure_min_brightness", 9.0), ("auto_exposure_max_brightness", 14.5),
                 ("auto_exposure_bias", 0.3), ("bloom_intensity", 0.4), ("vignette_intensity", 0.25),
                 ("film_grain_intensity", 0.08), ("white_temp", 6200.0)):
        setp(s, "override_" + k, True); setp(s, k, v)
    ppv.set_editor_property("settings", s)
    log("lighting ready")


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    import_textures()
    mpc = make_mpc()
    mis = make_instances(make_master(mpc))

    slice_man = json.load(open(SLICE_DIR / "manifest.json"))
    terr_man = json.load(open(TERRAIN_DIR / "terrain_manifest.json"))
    blds = slice_man["buildings"][:LIMIT] if LIMIT else slice_man["buildings"]

    import_fbx([TERRAIN_DIR / t["fbx"] for t in terr_man["tiles"]], f"{G}/Terrain")
    import_fbx([SLICE_DIR / b["fbx"] for b in blds], f"{G}/Buildings/OldDelhiVS")
    import_fbx([SLICE_DIR / "Roads.fbx", SLICE_DIR / "Overhead_Wires.fbx"], f"{G}/Streets/OldDelhiVS")

    LVL.new_level("/Game/Maps/OldDelhi_VS")
    placed = 0
    for t in terr_man["tiles"]:
        mesh = unreal.load_asset(f"{G}/Terrain/{t['id']}")
        if mesh:
            assign(mesh, {"M_Ground": mis["M_Ground"]}, complex_as_simple=True)
            for i in range(len(mesh.get_editor_property("static_materials"))):
                mesh.set_material(i, mis["M_Ground"])
            a = ACT.spawn_actor_from_object(mesh, to_ue(t["location_m"]))
            a.set_folder_path("Terrain")
    for b in blds:
        mesh = unreal.load_asset(f"{G}/Buildings/OldDelhiVS/{b['id']}")
        if not mesh:
            continue
        assign(mesh, mis)
        a = ACT.spawn_actor_from_object(mesh, to_ue(b["location_m"]))
        a.set_folder_path("Buildings")
        placed += 1
    for name in ("Roads", "Overhead_Wires"):
        mesh = unreal.load_asset(f"{G}/Streets/OldDelhiVS/{name}")
        if mesh:
            assign(mesh, mis)
            if name == "Roads":
                for i in range(len(mesh.get_editor_property("static_materials"))):
                    mesh.set_material(i, mis["M_Asphalt"])
            a = ACT.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0))
            a.set_folder_path("Streets")
    lighting()

    if CAMERAS.exists():
        for i, c in enumerate(json.load(open(CAMERAS))):
            cam = ACT.spawn_actor_from_class(unreal.CineCameraActor, to_ue(c["location_m"]),
                                             unreal.Rotator(roll=0, pitch=c["pitch"], yaw=c["yaw"]))
            cam.set_actor_label(f"CAM_{c['name']}")
            cam.set_folder_path("Cameras")
            cc = comp(cam, "camera_component", "cine_camera_component")
            setp(cc, "current_focal_length", c.get("focal_mm", 20.0))
    ps = ACT.spawn_actor_from_class(unreal.PlayerStart, to_ue(json.load(open(CAMERAS))[0]["location_m"])
                                    if CAMERAS.exists() else unreal.Vector(0, 0, 300))
    LVL.save_current_level()
    log(f"DONE: {placed} buildings placed in /Game/Maps/OldDelhi_VS")


main()
