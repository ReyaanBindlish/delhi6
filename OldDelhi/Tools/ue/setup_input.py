"""
Creates the Enhanced Input assets for Daya under /Game/Input and fills IMC_Default with
keyboard+mouse and gamepad (Xbox / PlayStation) mappings.

Run headless:
  "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
     C:\\Users\\Rayaan\\Desktop\\game\\OldDelhi\\OldDelhi.uproject ^
     -run=pythonscript -script=C:\\Users\\Rayaan\\Desktop\\game\\OldDelhi\\Tools\\ue\\setup_input.py

Idempotent: existing assets are loaded and rebuilt, not duplicated. Needs the EnhancedInput and
PythonScriptPlugin plugins enabled. Device scaling lives here (mouse deg/count, stick deg/s*dt) so the
C++ only applies the user sensitivity multiplier (ADayaCharacter::LookSensitivity).

Axis2D convention: X = right, Y = forward (move) / up (look).
"""
import unreal

DIR = "/Game/Input"

# Tunables -------------------------------------------------------------------------------------------
MOUSE_DEG_PER_COUNT = 0.10
STICK_LOOK_DEG_PER_SEC = 140.0
MOVE_DEADZONE = (0.22, 1.0)
LOOK_DEADZONE = (0.18, 1.0)

AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log("[setup_input] " + str(msg))


def warn(msg):
    unreal.log_warning("[setup_input] " + str(msg))


def find_factory(*names):
    for n in names:
        cls = getattr(unreal, n, None)
        if cls is not None:
            return cls()
    return None


def get_or_create(name, asset_class, factory_names):
    path = "%s/%s" % (DIR, name)
    if EAL.does_asset_exist(path):
        asset = EAL.load_asset(path)
        if asset:
            return asset
    factory = find_factory(*factory_names)
    if factory is None:
        raise RuntimeError("No factory found among %s; is the EnhancedInput plugin enabled?" % (factory_names,))
    asset = AT.create_asset(name, DIR, asset_class, factory)
    if asset is None:
        raise RuntimeError("create_asset failed for " + path)
    log("created " + path)
    return asset


# Input actions ----------------------------------------------------------------------------------------
VT = unreal.InputActionValueType
ACTIONS = {
    "IA_Move": VT.AXIS2D,
    "IA_Look": VT.AXIS2D,
    "IA_Sprint": VT.BOOLEAN,
    "IA_Walk": VT.BOOLEAN,
    "IA_Crouch": VT.BOOLEAN,
    "IA_Interact": VT.BOOLEAN,
    "IA_Vault": VT.BOOLEAN,
    "IA_Notebook": VT.BOOLEAN,
    "IA_ShoulderSwap": VT.BOOLEAN,
    "IA_CameraToggle": VT.BOOLEAN,
    "IA_Pause": VT.BOOLEAN,
}


def make_actions():
    out = {}
    for name, vtype in ACTIONS.items():
        a = get_or_create(name, unreal.InputAction, ("InputAction_Factory", "InputActionFactory"))
        a.set_editor_property("value_type", vtype)
        # Pause must also be consumed when other contexts are active; no accumulation for booleans.
        try:
            a.set_editor_property("consume_input", True)
        except Exception:
            pass
        EAL.save_loaded_asset(a)
        out[name] = a
    return out


# Modifiers -------------------------------------------------------------------------------------------
def mod_negate(x=False, y=False, z=False):
    m = unreal.InputModifierNegate()
    m.set_editor_property("x", x)
    m.set_editor_property("y", y)
    m.set_editor_property("z", z)
    return m


def mod_swizzle_yxz():
    m = unreal.InputModifierSwizzleAxis()
    m.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
    return m


def mod_deadzone(lower, upper):
    m = unreal.InputModifierDeadZone()
    m.set_editor_property("lower_threshold", lower)
    m.set_editor_property("upper_threshold", upper)
    m.set_editor_property("type", unreal.DeadZoneType.RADIAL)
    return m


def mod_scalar(sx, sy=None, sz=1.0):
    m = unreal.InputModifierScalar()
    m.set_editor_property("scalar", unreal.Vector(sx, sx if sy is None else sy, sz))
    return m


def mod_dt():
    return unreal.InputModifierScaleByDeltaTime()


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


# Mapping table: (action, key name, [modifiers]) ---------------------------------------------------------
def build_table():
    t = []
    # Move: keyboard
    t.append(("IA_Move", "D", []))
    t.append(("IA_Move", "A", [mod_negate(x=True)]))
    t.append(("IA_Move", "W", [mod_swizzle_yxz()]))
    t.append(("IA_Move", "S", [mod_swizzle_yxz(), mod_negate(x=True)]))
    # Move: gamepad (left stick, radial dead zone)
    t.append(("IA_Move", "Gamepad_Left2D", [mod_deadzone(*MOVE_DEADZONE)]))

    # Look: mouse (counts -> degrees); gamepad (stick -> deg/s * dt)
    t.append(("IA_Look", "Mouse2D", [mod_scalar(MOUSE_DEG_PER_COUNT)]))
    t.append(("IA_Look", "Gamepad_Right2D", [mod_deadzone(*LOOK_DEADZONE), mod_scalar(STICK_LOOK_DEG_PER_SEC), mod_dt()]))

    # Sprint: hold Shift / click L3 (C++ turns the L3 click into a toggle)
    t.append(("IA_Sprint", "LeftShift", []))
    t.append(("IA_Sprint", "Gamepad_LeftThumbstick", []))
    # Hold-to-walk: Alt / LB
    t.append(("IA_Walk", "LeftAlt", []))
    t.append(("IA_Walk", "Gamepad_LeftShoulder", []))
    # Crouch (toggle): C, Ctrl / B (Circle)
    t.append(("IA_Crouch", "C", []))
    t.append(("IA_Crouch", "LeftControl", []))
    t.append(("IA_Crouch", "Gamepad_FaceButton_Right", []))
    # Interact: E / A (Cross)
    t.append(("IA_Interact", "E", []))
    t.append(("IA_Interact", "Gamepad_FaceButton_Bottom", []))
    # Vault / mantle: Space / X (Square)
    t.append(("IA_Vault", "SpaceBar", []))
    t.append(("IA_Vault", "Gamepad_FaceButton_Left", []))
    # Notebook: Tab, N / View (Share)
    t.append(("IA_Notebook", "Tab", []))
    t.append(("IA_Notebook", "N", []))
    t.append(("IA_Notebook", "Gamepad_Special_Left", []))
    # Shoulder swap: Q / R3
    t.append(("IA_ShoulderSwap", "Q", []))
    t.append(("IA_ShoulderSwap", "Gamepad_RightThumbstick", []))
    # First / third person: V / D-pad up
    t.append(("IA_CameraToggle", "V", []))
    t.append(("IA_CameraToggle", "Gamepad_DPad_Up", []))
    # Pause: Esc / Menu (Options)
    t.append(("IA_Pause", "Escape", []))
    t.append(("IA_Pause", "Gamepad_Special_Right", []))
    return t


def fill_context(imc, actions):
    table = build_table()

    # Strategy 1: assemble full mapping structs and assign them (keeps modifiers reliably).
    try:
        try:
            imc.unmap_all()
        except Exception:
            pass
        structs = []
        for aname, kname, mods in table:
            m = unreal.EnhancedActionKeyMapping()
            m.set_editor_property("action", actions[aname])
            m.set_editor_property("key", key(kname))
            m.set_editor_property("modifiers", mods)
            structs.append(m)
        assigned = False
        for prop in ("default_key_mappings", "mappings"):
            try:
                if prop == "default_key_mappings":
                    holder = imc.get_editor_property(prop)
                    holder.set_editor_property("mappings", structs)
                    imc.set_editor_property(prop, holder)
                else:
                    imc.set_editor_property(prop, structs)
                assigned = True
                break
            except Exception as e:
                warn("assign via %s failed: %s" % (prop, e))
        if assigned and verify(imc, len(table)):
            return "struct-assign"
    except Exception as e:
        warn("strategy 1 failed: %s" % e)

    # Strategy 2: map_key then patch modifiers on the returned mapping.
    try:
        imc.unmap_all()
    except Exception:
        pass
    for aname, kname, mods in table:
        m = imc.map_key(actions[aname], key(kname))
        try:
            m.set_editor_property("modifiers", mods)
        except Exception as e:
            warn("could not set modifiers for %s/%s: %s" % (aname, kname, e))
    return "map_key"


def verify(imc, expected):
    try:
        holder = imc.get_editor_property("default_key_mappings")
        got = holder.get_editor_property("mappings")
        mods = sum(1 for m in got if len(m.get_editor_property("modifiers")) > 0)
        log("IMC_Default has %d mappings, %d with modifiers" % (len(got), mods))
        return len(got) == expected
    except Exception as e:
        warn("verify failed: %s" % e)
        return False


def main():
    if not EAL.does_directory_exist(DIR):
        EAL.make_directory(DIR)
    actions = make_actions()
    imc = get_or_create("IMC_Default", unreal.InputMappingContext,
                        ("InputMappingContext_Factory", "InputMappingContextFactory"))
    how = fill_context(imc, actions)
    EAL.save_loaded_asset(imc)
    log("IMC_Default filled via %s. Done: %d actions + IMC_Default in %s" % (how, len(actions), DIR))
    if how != "struct-assign":
        warn("Open IMC_Default once and check that dead zones / swizzles are present on Move and Look; "
             "if not, re-run after updating this script for the 5.8 API.")


main()
