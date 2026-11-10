"""Set up the principal cast inside Unreal Engine 5.8 (headless or in-editor).

  UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file>" -unattended -nop4

Steps
  1. Create /Game/Characters/Cast/DT_Cast from OldDelhi/Data/DT_Cast.csv (row struct FCastMemberRow).
  2. Create /Game/Characters/Cast/DA_Cast (UDhurandharCastDataAsset): table link, default Daya anchors,
     UE mannequin fallback meshes when the Manny/Quinn assets exist, and the story-invariant check.
  3. If the MetaHumanCharacter Python API exists in this engine build, create one MetaHuman Character asset
     per recipe (Daya gets three: MH_Daya_A1, MH_Daya_A2, MH_Daya_A6), apply the skin tone through the
     editor subsystem and store the full recipe on the asset as metadata. Everything that the Python API
     does not expose (face sculpting, parametric body constraints, grooms) is logged clearly and stays a
     documented manual step (OldDelhi/docs/CHARACTERS.md section 5). Each step is guarded.

Run outside the editor (plain Python) for a dry run that only validates the CSV and the recipes:
  python setup_characters.py
"""
import csv
import json
import os
import sys
from pathlib import Path

try:
    import unreal
    IN_EDITOR = True
except ImportError:  # dry run
    unreal = None
    IN_EDITOR = False

REPO = Path(os.environ.get("DHURANDHAR_REPO") or Path(__file__).resolve().parents[3])
CSV_PATH = REPO / "OldDelhi" / "Data" / "DT_Cast.csv"
RECIPE_DIR = REPO / "characters" / "cast"

CAST_DIR = "/Game/Characters/Cast"
MH_DIR = CAST_DIR + "/MetaHuman"
TABLE_PATH = CAST_DIR + "/DT_Cast"
DATA_ASSET_NAME = "DA_Cast"

# Where the UE5 third-person template keeps the mannequins; the first that exists is used.
MANNEQUIN_CANDIDATES = {
    "SKM_Manny": ["/Game/Characters/Mannequins/Meshes/SKM_Manny", "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"],
    "SKM_Quinn": ["/Game/Characters/Mannequins/Meshes/SKM_Quinn", "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"],
}

ERRORS = []


def log(*a):
    msg = "[characters] " + " ".join(str(x) for x in a)
    if IN_EDITOR:
        unreal.log(msg)
    else:
        print(msg)


def warn(*a):
    msg = "[characters] WARN " + " ".join(str(x) for x in a)
    if IN_EDITOR:
        unreal.log_warning(msg)
    else:
        print(msg)


def setp(obj, names, value):
    """Set the first property name that exists (python names drift between engine versions)."""
    for n in names if isinstance(names, (list, tuple)) else [names]:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception:
            continue
    warn("could not set", names, "on", obj.get_class().get_name() if hasattr(obj, "get_class") else obj)
    return False


# ------------------------------------------------------------------------------ data (works without unreal)
def load_rows():
    with open(CSV_PATH, encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def load_recipes():
    out = {}
    for p in sorted(RECIPE_DIR.glob("cast_*.json")):
        with open(p, encoding="utf-8") as f:
            d = json.load(f)
        out[d["id"]] = d
    return out


def check_invariants(rows, recipes):
    """Bible and brief invariants. Returns a list of problems (empty = fine)."""
    problems = []
    by = {r["Name"]: r for r in rows}
    need = ["Daya", "Sukhi", "Gurnoor", "Bibi", "Bablu", "Mangal", "Khanna", "Ravi", "Qureshi", "Sattar", "Shahzaib"]
    for n in need:
        if n not in by:
            problems.append(f"missing cast row {n}")
        if n not in recipes:
            problems.append(f"missing recipe cast_{n.lower()}.json")
    if problems:
        return problems
    w = lambda n: float(by[n]["WeightKg"])
    if abs((w("Bablu") - w("Daya")) - 20.0) > 0.5:
        problems.append("Bablu must be 20 kg heavier than Daya")
    if int(by["Gurnoor"]["AgeAtStart"]) != 9:
        problems.append("Gurnoor must be 9")
    if int(by["Bibi"]["AgeAtStart"]) != 84:
        problems.append("Bibi must be 84")
    states = {s["state"]: s for s in recipes["Daya"].get("body_states", [])}
    if set(states) != {"Healthy2019", "Beggar", "Diseased"}:
        problems.append("Daya needs exactly three body states")
    else:
        if abs(w("Daya") - states["Beggar"]["weight_kg"] - 14) > 0.5:
            problems.append("Daya Act 2 must be 14 kg below Act 1")
        if abs(w("Daya") - states["Diseased"]["weight_kg"] - 29) > 0.5:
            problems.append("Daya Act 5-6 must be 29 kg below Act 1")
    if float(by["Daya"]["HeightCm"]) < 177 or float(by["Daya"]["HeightCm"]) > 179:
        problems.append("Daya must be 5 ft 10 (about 178 cm)")
    if by["Mangal"]["Gait"] != "WheeledBoard":
        problems.append("Mangal must use the WheeledBoard gait")
    return problems


# ------------------------------------------------------------------------------ unreal steps
def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_table():
    """Create DT_Cast from the CSV with FCastMemberRow."""
    struct = getattr(unreal, "CastMemberRow", None)
    if struct is None:
        ERRORS.append("unreal.CastMemberRow not found: compile the OldDelhi module first (Characters/CastTypes.h)")
        warn(ERRORS[-1])
        return None
    ensure_dir(CAST_DIR)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    eal = unreal.EditorAssetLibrary
    table = eal.load_asset(TABLE_PATH) if eal.does_asset_exist(TABLE_PATH) else None
    if table is None:
        factory = unreal.DataTableFactory()
        setp(factory, "struct", struct.static_struct())
        table = at.create_asset("DT_Cast", CAST_DIR, unreal.DataTable, factory)
    if table is None:
        ERRORS.append("could not create DT_Cast")
        warn(ERRORS[-1])
        return None
    try:
        ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, str(CSV_PATH))
        log("DT_Cast filled from CSV:", ok)
    except Exception as e:
        # Fallback: the interchange/CSV import factory.
        warn("fill_data_table_from_csv_file failed (", e, "), trying CSVImportFactory")
        try:
            f = unreal.CSVImportFactory()
            f.automated_import_settings.import_row_struct = struct.static_struct()
            f.automated_import_settings.import_type = unreal.CSVImportType.ECSV_DATA_TABLE
            t = unreal.AssetImportTask()
            t.filename = str(CSV_PATH)
            t.destination_path = CAST_DIR
            t.destination_name = "DT_Cast"
            t.automated = True
            t.save = True
            t.replace_existing = True
            t.factory = f
            at.import_asset_tasks([t])
            table = eal.load_asset(TABLE_PATH)
        except Exception as e2:
            ERRORS.append(f"CSV import failed: {e2}")
            warn(ERRORS[-1])
            return None
    eal.save_loaded_asset(table)
    try:
        names = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)]
        log("DT_Cast rows:", len(names), names)
    except Exception:
        pass
    return table


def find_mannequin(key):
    eal = unreal.EditorAssetLibrary
    for p in MANNEQUIN_CANDIDATES.get(key, []):
        if eal.does_asset_exist(p):
            return eal.load_asset(p)
    return None


def create_data_asset(table, mh_assets):
    eal = unreal.EditorAssetLibrary
    cls = getattr(unreal, "DhurandharCastDataAsset", None)
    if cls is None:
        ERRORS.append("unreal.DhurandharCastDataAsset not found: compile the OldDelhi module first")
        warn(ERRORS[-1])
        return None
    path = f"{CAST_DIR}/{DATA_ASSET_NAME}"
    if eal.does_asset_exist(path):
        da = eal.load_asset(path)
    else:
        at = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        setp(factory, "data_asset_class", cls)
        da = at.create_asset(DATA_ASSET_NAME, CAST_DIR, cls, factory)
    if da is None:
        ERRORS.append("could not create DA_Cast")
        warn(ERRORS[-1])
        return None
    if table is not None:
        setp(da, "cast_table", table)

    # Per-character visuals: mannequin fallback + MetaHuman asset (first one for Daya).
    rows = load_rows()
    visuals = {}
    missing = set()
    for r in rows:
        v = unreal.CastVisualSet()
        mannequin = find_mannequin(r["FallbackMesh"])
        if mannequin is not None:
            setp(v, "fallback_mesh", mannequin)
        else:
            missing.add(r["FallbackMesh"])
        assets = mh_assets.get(r["Name"], [])
        if assets:
            setp(v, ["meta_human_character", "metahuman_character"], assets[0])
        visuals[unreal.Name(r["Name"])] = v
    for m in sorted(missing):
        warn("mannequin", m, "not found under /Game/Characters/Mannequins (add the UE third-person template content, "
             "then re-run this script); fallback_mesh left empty")
    setp(da, "visuals", visuals)

    # Run the story-invariant check implemented in C++.
    try:
        res = da.validate_cast_invariants()
        log("ValidateCastInvariants ->", res)
    except Exception as e:
        warn("validate_cast_invariants not callable from Python:", e)
    eal.save_loaded_asset(da)
    return da


# ---------------------------------------------------------------------------- MetaHuman
def metahuman_api_available():
    needed = ["MetaHumanCharacter", "MetaHumanCharacterFactoryNew"]
    missing = [n for n in needed if not hasattr(unreal, n)]
    if missing:
        warn("MetaHuman Character Python API not available in this build, missing:", missing,
             "- enable plugins MetaHumanCharacter and MetaHumanCharacterEditor, or create the assets by hand "
             "following docs/CHARACTERS.md section 5. Recipes are in characters/cast/*.json")
        return False
    return True


def apply_skin(character, recipe):
    """Set the skin tone U/V through the editor subsystem (the only part of a recipe fully scriptable)."""
    sub_cls = getattr(unreal, "MetaHumanCharacterEditorSubsystem", None)
    if sub_cls is None:
        warn("MetaHumanCharacterEditorSubsystem not exposed; skin tone must be set in Creator by hand")
        return False
    try:
        sub = unreal.get_editor_subsystem(sub_cls)
        skin_settings = character.get_editor_property("skin_settings")
        skin = skin_settings.get_editor_property("skin")
        skin.set_editor_property("u", float(recipe["metahuman"]["skin"]["U"]))
        skin.set_editor_property("v", float(recipe["metahuman"]["skin"]["V"]))
        skin.set_editor_property("roughness", float(recipe["metahuman"]["skin"].get("roughness", 1.06)))
        skin_settings.set_editor_property("skin", skin)
        added = sub.try_add_object_to_edit(character)
        sub.commit_skin_settings(character, skin_settings)
        if added:
            sub.remove_object_to_edit(character)
        return True
    except Exception as e:
        warn("skin tone could not be applied by script:", e)
        return False


def create_metahumans(recipes):
    """Returns {cast_id: [asset, ...]} for whatever was created."""
    created = {}
    if not metahuman_api_available():
        return created
    ensure_dir(MH_DIR)
    eal = unreal.EditorAssetLibrary
    at = unreal.AssetToolsHelpers.get_asset_tools()
    for cast_id, recipe in recipes.items():
        for asset_name in recipe["metahuman"]["assets"]:
            path = f"{MH_DIR}/{asset_name}"
            if eal.does_asset_exist(path):
                asset = eal.load_asset(path)
                log("exists", path)
            else:
                try:
                    asset = at.create_asset(asset_name, MH_DIR, unreal.MetaHumanCharacter, unreal.MetaHumanCharacterFactoryNew())
                except Exception as e:
                    warn("could not create", asset_name, e)
                    asset = None
            if asset is None:
                continue
            state_note = ""
            body = recipe["metahuman"]["body_measurements_cm_start"]
            if cast_id == "Daya":
                key = {"MH_Daya_A1": "Healthy2019", "MH_Daya_A2": "Beggar", "MH_Daya_A6": "Diseased"}[asset_name]
                st = next(s for s in recipe["body_states"] if s["state"] == key)
                body = st["body_measurements_cm_start"]
                state_note = f"{key}: {st['weight_kg']} kg; {st['notes']}"
            # Always record the full recipe on the asset so the artist has it in the editor.
            try:
                eal.set_metadata_tag(asset, "CastRecipeFile", str(RECIPE_DIR / f"cast_{cast_id.lower()}.json"))
                eal.set_metadata_tag(asset, "CastBodyMeasurementsCm", json.dumps(body))
                eal.set_metadata_tag(asset, "CastFaceNotes", recipe["metahuman"]["face_notes"])
                eal.set_metadata_tag(asset, "CastGroom", json.dumps(recipe["metahuman"]["groom"]))
                if state_note:
                    eal.set_metadata_tag(asset, "CastBodyState", state_note)
            except Exception as e:
                warn("metadata tags failed on", asset_name, e)
            apply_skin(asset, recipe)
            eal.save_loaded_asset(asset)
            created.setdefault(cast_id, []).append(asset)
            log("MetaHuman Character ready:", path, "(face, body constraints and grooms: manual, see docs)")
    return created


# ----------------------------------------------------------------------------------- main
def main():
    rows = load_rows()
    recipes = load_recipes()
    problems = check_invariants(rows, recipes)
    for p in problems:
        warn("INVARIANT:", p)
    log("cast rows:", len(rows), "recipes:", len(recipes), "invariants ok:", not problems)
    if not IN_EDITOR:
        log("dry run (no unreal module): nothing was created")
        return 0 if not problems else 1

    table = import_table()
    mh = create_metahumans(recipes)
    da = create_data_asset(table, mh)
    if ERRORS:
        warn("finished with problems:", ERRORS)
    else:
        log("done: DT_Cast, DA_Cast", "and MetaHuman assets" if mh else "(MetaHuman assets skipped)")
    return 0


if __name__ == "__main__":
    sys.exit(main() if not IN_EDITOR else (main() or 0))
