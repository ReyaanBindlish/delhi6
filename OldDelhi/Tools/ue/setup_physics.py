"""Create Old Delhi physical materials and assign them to material instances.

  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file>" -unattended -nop4

Needs the Physical Surfaces list in Config/DefaultEngine.ini first (docs/PHYSICS.md section 2) so SurfaceType1..13
carry the right names; the script warns if they are missing but still sets the surface type numbers.

Creates /Game/Delhi/Physics/PM_<Surface> (friction / restitution / density from implementation.md 4.1) and sets
material instance `phys_material` for every MI_* in /Game/Delhi/Materials by exact name, then by keyword.
Safe to re-run (updates values in place).  Env: DELHI_PHYS_DRYRUN=1 only logs what it would assign.
"""
import os

import unreal

G = "/Game/Delhi"
PM_DIR = f"{G}/Physics"
MAT_DIR = f"{G}/Materials"
DRY = os.environ.get("DELHI_PHYS_DRYRUN", "0") == "1"

AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary


def log(*a):
    unreal.log("[delhi-phys] " + " ".join(str(x) for x in a))


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


# index == EDelhiSurface value == SurfaceTypeN.  Keep in sync with DelhiSurfaceLibrary.cpp.
# (name, friction, restitution, density g/cm3, multiply_combine)
SURFACES = [
    ("LakhoriBrick",      0.65, 0.10, 1.9, False),   # 1
    ("LimePlaster",       0.60, 0.05, 1.6, False),   # 2
    ("RedSandstone",      0.60, 0.10, 2.3, False),   # 3
    ("KotaStone",         0.55, 0.10, 2.7, False),   # 4
    ("CorrugatedShutter", 0.40, 0.30, 7.8, False),   # 5
    ("Glass",             0.30, 0.20, 2.5, False),   # 6
    ("Asphalt",           0.75, 0.05, 2.3, False),   # 7
    ("Mud",               0.45, 0.00, 1.7, True),    # 8  Friction Combine = Multiply so vehicles slide
    ("Wood",              0.50, 0.25, 0.7, False),   # 9
    ("SheetMetal",        0.35, 0.25, 7.8, False),   # 10
    ("Water",             0.05, 0.00, 1.0, False),   # 11
    ("Concrete",          0.70, 0.05, 2.4, False),   # 12
    ("Flesh",             0.60, 0.05, 1.0, False),   # 13
]

# Exact names used by build_vertical_slice.py.
EXACT = {
    "MI_PaintedPlaster": "LimePlaster",
    "MI_DamagedPlaster": "LimePlaster",
    "MI_Brick": "LakhoriBrick",
    "MI_Concrete": "Concrete",
    "MI_RoofConcrete": "Concrete",
    "MI_RustyMetal": "SheetMetal",
    "MI_RollingShutter": "CorrugatedShutter",
    "MI_Glass": "Glass",
    "MI_PVCTank": "Concrete",
    "MI_Signboard": "SheetMetal",
    "MI_ACUnit": "SheetMetal",
    "MI_Cable": "SheetMetal",
    "MI_Asphalt": "Asphalt",
    "MI_Ground": "Mud",
}

# Fallback keyword rules for later materials (first match wins, lower-case substring of the asset name).
KEYWORDS = [
    ("sandstone", "RedSandstone"), ("redstone", "RedSandstone"), ("chandni", "RedSandstone"),
    ("kota", "KotaStone"), ("shutter", "CorrugatedShutter"), ("corrug", "CorrugatedShutter"),
    ("glass", "Glass"), ("window", "Glass"),
    ("asphalt", "Asphalt"), ("road", "Asphalt"), ("tar", "Asphalt"),
    ("mud", "Mud"), ("slush", "Mud"), ("dirt", "Mud"), ("ground", "Mud"),
    ("brick", "LakhoriBrick"), ("lakhori", "LakhoriBrick"),
    ("plaster", "LimePlaster"), ("lime", "LimePlaster"), ("paint", "LimePlaster"),
    ("concrete", "Concrete"), ("cement", "Concrete"),
    ("wood", "Wood"), ("timber", "Wood"), ("door", "Wood"),
    ("metal", "SheetMetal"), ("steel", "SheetMetal"), ("iron", "SheetMetal"), ("tin", "SheetMetal"),
    ("water", "Water"), ("skin", "Flesh"), ("flesh", "Flesh"),
]


def surface_enum(index):
    return getattr(unreal.PhysicalSurface, f"SURFACE_TYPE{index}")


def make_pm(index, row):
    name, fric, rest, dens, multiply = row
    path = f"{PM_DIR}/PM_{name}"
    if EAL.does_asset_exist(path):
        pm = unreal.load_asset(path)
    else:
        pm = AT.create_asset(f"PM_{name}", PM_DIR, unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
    setp(pm, "friction", fric)
    setp(pm, "static_friction", fric)
    setp(pm, "restitution", rest)
    setp(pm, "density", dens)
    setp(pm, "surface_type", surface_enum(index))
    if multiply:
        try:
            setp(pm, "friction_combine_mode", unreal.FrictionCombineMode.MULTIPLY)
            setp(pm, "override_friction_combine_mode", True)
        except Exception as e:  # enum name drift
            log("WARN friction combine", e)
    EAL.save_loaded_asset(pm)
    return pm


def check_surface_names():
    try:
        ps = unreal.PhysicsSettings.get_default_object()
        names = [str(s.get_editor_property("name")) for s in ps.get_editor_property("physical_surfaces")]
        want = [s[0] for s in SURFACES]
        missing = [w for w in want if w not in names]
        if missing:
            log("WARN Physical Surfaces missing in project settings:", missing,
                "- add the PhysicalSurfaces lines from docs/PHYSICS.md to DefaultEngine.ini and restart")
        else:
            log("physical surface names OK")
    except Exception as e:
        log("could not read PhysicsSettings:", e)


def pick_surface(asset_name):
    if asset_name in EXACT:
        return EXACT[asset_name]
    low = asset_name.lower()
    for key, surf in KEYWORDS:
        if key in low:
            return surf
    return None


def main():
    if not EAL.does_directory_exist(PM_DIR):
        EAL.make_directory(PM_DIR)
    check_surface_names()

    pms = {}
    for i, row in enumerate(SURFACES, start=1):
        pms[row[0]] = make_pm(i, row)
    log("physical materials ready:", len(pms))

    assigned, skipped = 0, []
    for path in EAL.list_assets(MAT_DIR, recursive=True, include_folder=False):
        asset_name = path.rsplit("/", 1)[-1].split(".")[0]
        if not asset_name.startswith("MI_"):
            continue
        surf = pick_surface(asset_name)
        if surf is None:
            skipped.append(asset_name)
            continue
        if DRY:
            log("would assign", asset_name, "->", surf)
            continue
        mi = unreal.load_asset(path)
        if mi is None:
            continue
        if setp(mi, "phys_material", pms[surf]):
            EAL.save_loaded_asset(mi)
            assigned += 1
            log("assigned", asset_name, "->", f"PM_{surf}")
    log(f"assigned {assigned} material instances; no rule for: {skipped}")


main()
