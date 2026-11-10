"""Import a built story location (world/story/export/<loc>/manifest.json) into a new level /Game/Maps/<Map_Name>.

  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file> gulbahar_lines" -unattended -nop4
  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file> nizamuddin"   -unattended -nop4

Location can also come from env OLDDELHI_LOCATION. Other env switches:
  OLDDELHI_LIMIT=N          import only N buildings (smoke test)
  OLDDELHI_NO_LIGHTING=1    skip the sun / sky / fog / post-process actors (use the weather subsystem instead)
  OLDDELHI_CESIUM_REF=1     spawn an EDITOR-ONLY Cesium + Google Photorealistic 3D Tiles reference overlay
                            (needs the Cesium for Unreal plugin and CESIUM_ION_TOKEN; see world docs WORLD.md)

Patterns reused from Tools/ue/build_vertical_slice.py (not modified, not imported: it runs main() on import):
texture import, master material + MPC_Weather, material instances, FBX import with UCX collision + Nanite,
terrain tiles with complex-as-simple collision, level spawn from manifest, lighting.
Asset layout: /Game/Story/<Map_Name>/{Terrain,Buildings,Station,Streets}; materials shared in /Game/Delhi/Materials when
build_vertical_slice.py has already run there, otherwise created in /Game/World/Materials.
World frame: manifest metres (X east, Y north, Z up) -> Unreal (X east, Y south, Z up) in cm.
"""
import json
import os
import sys
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[3]
TEX_DIR = REPO / "world" / "textures_polyhaven_cc0"
EXPORT_ROOT = REPO / "world" / "story" / "export"
LIMIT = int(os.environ.get("OLDDELHI_LIMIT", "0"))
NO_LIGHTING = os.environ.get("OLDDELHI_NO_LIGHTING") == "1"
CESIUM_REF = os.environ.get("OLDDELHI_CESIUM_REF") == "1"

AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ACT = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LVL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(*a):
    unreal.log("[import_location] " + " ".join(str(x) for x in a))


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


def comp(actor, *names):
    for n in names:
        try:
            c = actor.get_editor_property(n)
            if c:
                return c
        except Exception:
            continue
    return None


# ------------------------------------------------------------------ materials (shared with the vertical slice)
SHARED = "/Game/Delhi/Materials"
OWN = "/Game/World/Materials"
TEXG = "/Game/Delhi/Textures"


def import_textures():
    tasks = []
    for f in sorted(TEX_DIR.glob("*.jpg")):
        if EAL.does_asset_exist(f"{TEXG}/T_{f.stem}"):
            continue
        t = unreal.AssetImportTask()
        t.filename = str(f); t.destination_path = TEXG; t.destination_name = f"T_{f.stem}"
        t.automated = True; t.save = True; t.replace_existing = True
        tasks.append(t)
    if tasks:
        AT.import_asset_tasks(tasks)
    for f in TEX_DIR.glob("*.jpg"):
        tex = unreal.load_asset(f"{TEXG}/T_{f.stem}")
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


def make_mpc():
    for base in (SHARED, OWN):
        if EAL.does_asset_exist(f"{base}/MPC_Weather"):
            return unreal.load_asset(f"{base}/MPC_Weather")
    mpc = AT.create_asset("MPC_Weather", OWN, unreal.MaterialParameterCollection,
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
    for base, nm in ((SHARED, "M_Delhi_Master"), (OWN, "M_World_Master")):
        if EAL.does_asset_exist(f"{base}/{nm}"):
            return unreal.load_asset(f"{base}/{nm}")
    m = AT.create_asset("M_World_Master", OWN, unreal.Material, unreal.MaterialFactoryNew())
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
    try:  # wetness from MPC_Weather: darken porous albedo ~30%, pull roughness toward 0.08
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
    # railway / station set (build_location.py)
    "M_PlatformSlab": ("concrete_floor_worn_001", (0.95, 0.93, 0.9), 0, 1.0, 0.0),
    "M_Ballast": ("concrete_floor_worn_001", (0.3, 0.28, 0.26), 0, 1.0, 0.0),
    "M_Rail": ("rusty_metal", (0.22, 0.2, 0.19), 0, 0.8, 0.8),
    "M_CanopyMetal": ("rusted_shutter", (0.75, 0.78, 0.82), 0, 1.0, 0.5),
    "M_SafetyLine": (None, (0.9, 0.72, 0.05), 0, 0.7, 0.0),
    "M_Sleeper": ("concrete_wall_008", (0.5, 0.49, 0.47), 0, 1.0, 0.0),
}


def make_instances(master):
    out = {}
    for name, (tid, tint, vt, rmul, metal) in MI.items():
        shared = f"{SHARED}/MI_{name[2:]}"
        path = shared if EAL.does_asset_exist(shared) else f"{OWN}/MI_{name[2:]}"
        if EAL.does_asset_exist(path):
            out[name] = unreal.load_asset(path)
            continue
        mi = AT.create_asset(f"MI_{name[2:]}", OWN, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, master)
        if tid:
            for p, suf in (("BaseColorTex", "diff"), ("NormalTex", "nor"), ("RoughTex", "rough")):
                tex = unreal.load_asset(f"{TEXG}/T_{tid}_{suf}")
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
def fbx_options():
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
        if not Path(f).exists():
            log("WARN missing", f)
            continue
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


def assign(mesh, mis, force=None, complex_as_simple=False):
    """Slot-name -> material instance (FBX slot names are the Blender material names). force: one MI for all slots."""
    mats = mesh.get_editor_property("static_materials")
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property("material_slot_name"))
        key = slot.split(".")[0]
        if force is not None:
            mesh.set_material(i, force)
        elif key in mis:
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


# ------------------------------------------------------------------ lighting
LIGHT = {   # style -> (sun pitch, sun yaw, lux, mie scale, fog density, fog inscatter RGB, white temp)
    "delhi": (-25, -35, 45.0, 0.012, 0.035, (0.55, 0.50, 0.40), 6200.0),      # late-Oct afternoon haze
    "karachi": (-32, 60, 60.0, 0.006, 0.020, (0.50, 0.55, 0.60), 6500.0),     # coastal humid afternoon
}


def lighting(style):
    pitch, yaw, lux, mie, dens, fogc, temp = LIGHT.get(style, LIGHT["delhi"])
    sun = ACT.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 50000),
                                     unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
    sc = comp(sun, "directional_light_component", "light_component")
    setp(sc, "intensity", lux)
    setp(sc, ["atmosphere_sun_light", "b_atmosphere_sun_light"], True)
    setp(sc, "light_source_angle", 0.53)
    setp(sc, "dynamic_shadow_distance_movable_light", 30000.0)
    sky = ACT.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    skc = comp(sky, "sky_atmosphere_component", "component")
    setp(skc, "mie_scattering_scale", mie)
    setp(skc, "mie_absorption_scale", mie / 2)
    setp(skc, "mie_anisotropy", 0.8)
    setp(skc, "rayleigh_scattering_scale", 0.03)
    sl = ACT.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000))
    slc = comp(sl, "light_component", "sky_light_component")
    setp(slc, "real_time_capture", True)
    setp(slc, "mobility", unreal.ComponentMobility.MOVABLE)
    fog = ACT.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = comp(fog, "component", "exponential_height_fog_component")
    setp(fc, "fog_density", dens)
    setp(fc, "fog_height_falloff", 0.08)
    setp(fc, ["fog_inscattering_luminance", "fog_inscattering_color"], unreal.LinearColor(*fogc, 1))
    setp(fc, "volumetric_fog", True)
    ppv = ACT.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    setp(ppv, "unbound", True)
    s = ppv.get_editor_property("settings")
    for k, v in (("auto_exposure_min_brightness", 9.0), ("auto_exposure_max_brightness", 14.5),
                 ("auto_exposure_bias", 0.3), ("bloom_intensity", 0.4), ("vignette_intensity", 0.25),
                 ("film_grain_intensity", 0.08), ("white_temp", temp)):
        setp(s, "override_" + k, True); setp(s, k, v)
    ppv.set_editor_property("settings", s)
    log("lighting ready (placeholder: DelhiWeatherSubsystem drives sun/sky/fog at runtime)")


# ------------------------------------------------------------------ editor-only Cesium + Google 3D Tiles reference
def cesium_reference(man):
    """Reference overlay only. Google imagery/3D data must never be copied into or shipped with the game:
    both actors are flagged editor-only (excluded from cook), hidden in game, tagged EditorReferenceOnly."""
    if not hasattr(unreal, "CesiumGeoreference") or not hasattr(unreal, "Cesium3DTileset"):
        log("WARN Cesium for Unreal plugin classes not found; reference overlay skipped")
        return
    lat, lon = man["slice_center_latlon"]
    base = man["terrain"]["base_elev_m"] if man.get("terrain") else 0.0
    try:
        geo = ACT.spawn_actor_from_class(unreal.CesiumGeoreference, unreal.Vector(0, 0, 0))
        geo.set_actor_label("REF_CesiumGeoreference_EditorOnly")
        # origin height = metres ASL of the level's Z=0 (terrain minimum); tune CESIUM_HEIGHT_OFFSET so the
        # tiles meet our terrain (geoid vs ellipsoid differ by tens of metres).
        setp(geo, "origin_latitude", lat); setp(geo, "origin_longitude", lon)
        setp(geo, "origin_height", base + float(os.environ.get("CESIUM_HEIGHT_OFFSET", "0")))
        ts = ACT.spawn_actor_from_class(unreal.Cesium3DTileset, unreal.Vector(0, 0, 0))
        ts.set_actor_label("REF_Google3DTiles_EditorOnly")
        setp(ts, "ion_asset_id", 2275207)                         # Google Photorealistic 3D Tiles (Cesium ion)
        tok = os.environ.get("CESIUM_ION_TOKEN", "")
        if tok:
            setp(ts, "ion_access_token", tok)
        for a in (geo, ts):
            setp(a, ["is_editor_only_actor", "b_is_editor_only_actor"], True)
            setp(a, ["hidden_in_game", "b_hidden", "actor_hidden_in_game"], True)
            a.tags = [unreal.Name("EditorReferenceOnly")]
            a.set_folder_path("ReferenceOnly")
        log("Cesium reference overlay spawned; toggle the ReferenceOnly folder (eye icon) to compare")
    except Exception as e:
        log("WARN cesium reference failed:", e)


# ------------------------------------------------------------------ main
def import_location(loc):
    root = EXPORT_ROOT / loc
    man = json.load(open(root / "manifest.json"))
    mapn = man["map_name"]
    G = f"/Game/Story/{mapn}"
    log(f"importing {loc} -> /Game/Maps/{mapn} ({len(man['buildings'])} buildings, {len(man['station'])} station pieces)")

    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    import_textures()
    mpc = make_mpc()
    mis = make_instances(make_master(mpc))

    blds = man["buildings"][:LIMIT] if LIMIT else man["buildings"]
    import_fbx([root / t["fbx"] for t in man["terrain_tiles"]], f"{G}/Terrain")
    import_fbx([root / b["fbx"] for b in blds], f"{G}/Buildings")
    import_fbx([root / s["fbx"] for s in man["station"]], f"{G}/Station")
    import_fbx([root / "Roads.fbx", root / "Overhead_Wires.fbx"], f"{G}/Streets")

    LVL.new_level(f"/Game/Maps/{mapn}")
    for t in man["terrain_tiles"]:
        mesh = unreal.load_asset(f"{G}/Terrain/{t['id']}")
        if mesh:
            assign(mesh, mis, force=mis["M_Ground"], complex_as_simple=True)   # ground collision
            a = ACT.spawn_actor_from_object(mesh, to_ue(t["location_m"]))
            a.set_folder_path("Terrain")
    placed = 0
    for b in blds:
        mesh = unreal.load_asset(f"{G}/Buildings/{b['id']}")
        if not mesh:
            continue
        assign(mesh, mis)
        a = ACT.spawn_actor_from_object(mesh, to_ue(b["location_m"]))
        a.set_folder_path("Buildings/" + ("Religious" if b.get("religious") else "Houses"))
        a.tags = [unreal.Name(f"Kind_{b.get('kind', 'house')}")] + ([unreal.Name("NoCombat")] if b.get("religious") else [])
        if b.get("name"):
            a.set_actor_label(f"{b['id']}_{b['name']}"[:60])
        placed += 1
    for s in man["station"]:
        mesh = unreal.load_asset(f"{G}/Station/{s['id']}")
        if mesh:
            assign(mesh, mis)
            a = ACT.spawn_actor_from_object(mesh, to_ue(s["location_m"]))
            a.set_folder_path(f"Station/{s['kind']}")
            a.set_actor_label(f"{s['id']}" + (f"_{s['name']}" if s.get("name") else ""))
    for name in ("Roads", "Overhead_Wires"):
        mesh = unreal.load_asset(f"{G}/Streets/{name}")
        if mesh:
            assign(mesh, mis, force=mis["M_Asphalt"] if name == "Roads" else None)
            a = ACT.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0))
            a.set_folder_path("Streets")
    if not NO_LIGHTING:
        lighting(man["style"])
    # player starts + nav bounds
    for ps in man.get("player_starts", []) or [{"name": "Default", "location_m": [0, 0, 5], "yaw_deg": 0}]:
        p = ACT.spawn_actor_from_class(unreal.PlayerStart, to_ue(ps["location_m"]) + unreal.Vector(0, 0, 100),
                                       unreal.Rotator(roll=0, pitch=0, yaw=ps.get("yaw_deg", 0)))
        p.set_actor_label("PlayerStart_" + ps["name"])
    try:
        nav = ACT.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 3000))
        s = man["size_m"] * 100.0 / 200.0 * 1.05          # default brush is 200 cm
        nav.set_actor_scale3d(unreal.Vector(s, s, 30.0))  # ~60 m tall slab
        nav.set_folder_path("Navigation")
    except Exception as e:
        log("WARN nav volume skipped:", e)
    if CESIUM_REF:
        cesium_reference(man)
    LVL.save_current_level()
    log(f"DONE {loc}: {placed} buildings, {len(man['station'])} station pieces in /Game/Maps/{mapn}. "
        f"Landscape alternative: {man['landscape_json']}")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    loc = os.environ.get("OLDDELHI_LOCATION") or (args[-1] if args else "")
    locs = ["gulbahar_lines", "nizamuddin"] if loc in ("", "all") else [loc]
    for l in locs:
        if (EXPORT_ROOT / l / "manifest.json").exists():
            import_location(l)
        else:
            log(f"ERROR no manifest for '{l}' under {EXPORT_ROOT}")


main()
