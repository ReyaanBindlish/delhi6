"""Set up the sky, weather and world boundary in the currently open level of Unreal Engine 5.8.

  UnrealEditor-Cmd.exe OldDelhi.uproject <map> -run=pythonscript -script="<this file>" -unattended -nop4
  (or run it from the editor's Python console / Tools > Execute Python Script with the level open)

What it does (see OldDelhi/docs/ATMOSPHERE.md):
  1. MPC_Weather (adds the new parameters to an existing one), MPC_Boundary, and M_Boundary
     (translucent unlit hex-lattice reveal built node by node with MaterialEditingLibrary).
  2. Removes loose sun / sky / fog / cloud / post-process actors (unless DELHI_KEEP_EXISTING=1).
  3. Spawns ADelhiSkyRig (sun + moon from a real NOAA sun position, SkyAtmosphere, real-time SkyLight,
     ExponentialHeightFog + volumetric fog, VolumetricCloud, physical-exposure PostProcess) for the chosen
     city / date / time / weather, and ADelhiWeatherController.
     If the OldDelhi C++ module is not compiled yet it falls back to plain engine actors with the same numbers.
  4. Optionally spawns AWorldBoundary (construct wall) as a rectangle around the origin.

Environment variables (all optional):
  DELHI_CITY            delhi | karachi                       (default delhi)
  DELHI_DATETIME        "YYYY-MM-DD HH:MM" local time         (default 2019-10-14 16:00)
  DELHI_WEATHER         Clear Haze Smog Fog Rain Monsoon DustStorm Overcast Thunderstorm SeaHaze
                        (default: let the weather system decide, previewing Haze)
  DELHI_GROUND_Z        ground height in cm for the fog and rig (default 0)
  DELHI_BOUNDARY        1 to spawn the world boundary (default 1)
  DELHI_BOUNDARY_HALF_M half size of the default boundary box in metres (default 1200)
  DELHI_KEEP_EXISTING   1 to leave existing lighting actors alone
  DELHI_FORCE_MATERIALS 1 to rebuild M_Boundary even if it exists
"""
import math
import os
import re
from datetime import datetime

import unreal

G = "/Game/Delhi"
MAT = f"{G}/Materials"
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ACT = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LVL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(*a):
    unreal.log("[lighting] " + " ".join(str(x) for x in a))


def setp(obj, names, value):
    """Set the first property name that exists (python names drop the leading b of bools; drift between versions)."""
    for n in names if isinstance(names, (list, tuple)) else [names]:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception:
            continue
    log("WARN could not set", names, "on", obj.get_class().get_name())
    return False


def env(name, default):
    v = os.environ.get(name)
    return v if v not in (None, "") else default


# ------------------------------------------------------------------------------------------------ configuration
CITY = env("DELHI_CITY", "delhi").lower()
WHEN = datetime.strptime(env("DELHI_DATETIME", "2019-10-14 16:00"), "%Y-%m-%d %H:%M")
WEATHER = env("DELHI_WEATHER", "")
GROUND_Z = float(env("DELHI_GROUND_Z", "0"))
WANT_BOUNDARY = env("DELHI_BOUNDARY", "1") == "1"
BOUNDARY_HALF_M = float(env("DELHI_BOUNDARY_HALF_M", "1200"))
KEEP = env("DELHI_KEEP_EXISTING", "0") == "1"
FORCE_MAT = env("DELHI_FORCE_MATERIALS", "0") == "1"

GEO = {  # lat, lon, UTC offset hours (explicit; research/02 A4.2)
    "delhi": (28.656, 77.232, 5.5),
    "karachi": (24.87, 66.99, 5.0),
}
if CITY not in GEO:
    CITY = "delhi"

# research/02 B2.6 / B2.7 numbers, used only by the no-C++ fallback and for the log line.
# name: (AOD, cloud, sun_temp_K, diffuse_lux, EV100, mie_scat, mie_abs, mie_g, mie_h_km, fog_density, fog_falloff, vol_ext)
PRESETS = {
    "Clear": (0.7, 0.05, 5900, 30000, 14.5, 1.0, 1.0, 0.8, 1.2, 0.010, 0.20, 1.0),
    "Haze": (1.2, 0.15, 5600, 35000, 14.0, 5.0, 3.0, 0.7, 2.0, 0.020, 0.10, 1.5),
    "Smog": (2.5, 0.20, 4200, 9000, 11.75, 10.0, 6.5, 0.65, 1.2, 0.120, 0.07, 2.5),
    "Fog": (1.0, 0.95, 6500, 7000, 10.75, 7.0, 0.0, 0.4, 0.4, 0.350, 0.30, 3.0),
    "Rain": (0.35, 0.85, 6500, 20000, 12.8, 2.0, 0.5, 0.8, 1.5, 0.030, 0.10, 1.5),
    "Monsoon": (0.35, 0.95, 6500, 18000, 12.5, 2.0, 0.5, 0.8, 1.5, 0.035, 0.10, 1.8),
    "DustStorm": (2.5, 0.30, 4500, 12000, 11.5, 15.0, 10.0, 0.6, 3.5, 0.060, 0.05, 3.0),
    "Overcast": (0.6, 0.75, 6300, 22000, 13.2, 2.0, 0.5, 0.8, 1.5, 0.025, 0.10, 1.2),
    "Thunderstorm": (0.5, 1.0, 6500, 9000, 11.5, 3.0, 1.0, 0.75, 1.5, 0.045, 0.10, 2.0),
    "SeaHaze": (0.9, 0.20, 5800, 32000, 14.2, 3.5, 1.2, 0.75, 1.6, 0.025, 0.12, 1.5),
}


# ------------------------------------------------------------------------------------------------ NOAA sun (python port of UDelhiAtmosphereLibrary)
def sun_position(local, lat, lon, tz):
    """Azimuth (deg from north, clockwise) and refracted elevation for a local wall-clock datetime."""
    utc_minutes = local.hour * 60 + local.minute + local.second / 60.0 - tz * 60.0
    y, m, d = local.year, local.month, local.day
    day_frac = 0.0
    while utc_minutes < 0:
        utc_minutes += 1440.0
        day_frac -= 1
    while utc_minutes >= 1440:
        utc_minutes -= 1440.0
        day_frac += 1
    if m <= 2:
        y -= 1
        m += 12
    a = y // 100
    b = 2 - a + a // 4
    jd = math.floor(365.25 * (y + 4716)) + math.floor(30.6001 * (m + 1)) + d + b - 1524.5 + day_frac + utc_minutes / 1440.0
    jc = (jd - 2451545.0) / 36525.0
    rad, deg = math.radians, math.degrees
    l0 = (280.46646 + jc * (36000.76983 + jc * 0.0003032)) % 360
    mm = 357.52911 + jc * (35999.05029 - 0.0001537 * jc)
    e = 0.016708634 - jc * (0.000042037 + 0.0000001267 * jc)
    c = (math.sin(rad(mm)) * (1.914602 - jc * (0.004817 + 0.000014 * jc)) + math.sin(rad(2 * mm)) * (0.019993 - 0.000101 * jc)
         + math.sin(rad(3 * mm)) * 0.000289)
    app = l0 + c - 0.00569 - 0.00478 * math.sin(rad(125.04 - 1934.136 * jc))
    obl = 23 + (26 + (21.448 - jc * (46.815 + jc * (0.00059 - jc * 0.001813))) / 60) / 60 + 0.00256 * math.cos(rad(125.04 - 1934.136 * jc))
    decl = math.asin(math.sin(rad(obl)) * math.sin(rad(app)))
    yy = math.tan(rad(obl) / 2) ** 2
    eq = 4 * deg(yy * math.sin(2 * rad(l0)) - 2 * e * math.sin(rad(mm)) + 4 * e * yy * math.sin(rad(mm)) * math.cos(2 * rad(l0))
                 - 0.5 * yy * yy * math.sin(4 * rad(l0)) - 1.25 * e * e * math.sin(2 * rad(mm)))
    tst = (utc_minutes + eq + 4 * lon) % 1440
    ha = tst / 4 - 180
    la = rad(lat)
    cz = max(-1.0, min(1.0, math.sin(la) * math.sin(decl) + math.cos(la) * math.cos(decl) * math.cos(rad(ha))))
    zen = math.acos(cz)
    elev = 90 - deg(zen)
    az = 0.0
    if abs(math.sin(zen)) > 1e-6:
        ca = max(-1.0, min(1.0, (math.sin(la) * cz - math.sin(decl)) / (math.cos(la) * math.sin(zen))))
        ar = deg(math.acos(ca))
        az = (ar + 180) % 360 if ha > 0 else (540 - ar) % 360
    if -0.575 < elev < 85:
        t = math.tan(rad(elev))
        arc = 58.1 / t - 0.07 / t ** 3 + 0.000086 / t ** 5 if elev > 5 else 1735 + elev * (-518.2 + elev * (103.4 + elev * (-12.79 + elev * 0.711)))
        elev += arc / 3600.0
    return az, elev


def sun_lux(elev, aod, cloud):
    if elev <= 0:
        return 0.0
    air = 1.0 / (math.sin(math.radians(elev)) + 0.50572 * (elev + 6.07995) ** -1.6364)
    return 127500.0 * math.exp(-(0.2 + aod) * air) * (1 - 0.95 * cloud ** 1.3)


# ------------------------------------------------------------------------------------------------ material parameter collections
def ensure_mpc(name, scalars, vectors=()):
    path = f"{MAT}/{name}"
    if EAL.does_asset_exist(path):
        mpc = unreal.load_asset(path)
    else:
        mpc = AT.create_asset(name, MAT, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    have_s = {str(p.get_editor_property("parameter_name")) for p in mpc.get_editor_property("scalar_parameters")}
    have_v = {str(p.get_editor_property("parameter_name")) for p in mpc.get_editor_property("vector_parameters")}
    s_list = list(mpc.get_editor_property("scalar_parameters"))
    v_list = list(mpc.get_editor_property("vector_parameters"))
    for n, v in scalars:
        if n not in have_s:
            p = unreal.CollectionScalarParameter()
            setp(p, "parameter_name", n)
            setp(p, "default_value", v)
            s_list.append(p)
    for n, v in vectors:
        if n not in have_v:
            p = unreal.CollectionVectorParameter()
            setp(p, "parameter_name", n)
            setp(p, "default_value", unreal.LinearColor(*v))
            v_list.append(p)
    setp(mpc, "scalar_parameters", s_list)
    setp(mpc, "vector_parameters", v_list)
    EAL.save_loaded_asset(mpc)
    return mpc


def make_mpcs():
    weather = ensure_mpc("MPC_Weather", [
        ("Wetness", 0.0), ("Dust", 0.25), ("Smog", 0.45), ("WindX", 1.4), ("WindY", 1.4), ("WindStrength", 2.0),
        ("RainIntensity", 0.0), ("Visibility", 6000.0), ("SunElevation", 30.0), ("NightFactor", 0.0), ("TimeOfDay", 16.0)])
    boundary = ensure_mpc("MPC_Boundary", [("Proximity", 0.0), ("RevealRadius", 1200.0)],
                          [("PlayerPos", (0.0, 0.0, -1.0e7, 1.0))])
    return weather, boundary


# ------------------------------------------------------------------------------------------------ M_Boundary
class Graph:
    """Tiny helper so the lattice graph below reads like the spec in implementation.md 4.2."""

    def __init__(self, mat):
        self.m = mat
        self.n = 0

    def node(self, cls, **props):
        x = -2400 + (self.n // 9) * 260
        y = (self.n % 9) * 170
        self.n += 1
        e = MEL.create_material_expression(self.m, cls, x, y)
        for k, v in props.items():
            setp(e, k, v)
        return e

    def c(self, v):
        return self.node(unreal.MaterialExpressionConstant, r=float(v))

    def op(self, cls, inputs, **props):
        e = self.node(cls, **props)
        for name, src in inputs.items():
            out = ""
            if isinstance(src, tuple):
                src, out = src
            if isinstance(src, (int, float)):
                src = self.c(src)
            MEL.connect_material_expressions(src, out, e, name)
        return e

    def mask(self, src, ch):
        return self.node_in(unreal.MaterialExpressionComponentMask, src, r=ch == "r", g=ch == "g", b=ch == "b", a=ch == "a")

    def node_in(self, cls, src, **props):
        e = self.node(cls, **props)
        MEL.connect_material_expressions(src, "", e, "")
        return e

    def mul(self, a, b): return self.op(unreal.MaterialExpressionMultiply, {"A": a, "B": b})
    def add(self, a, b): return self.op(unreal.MaterialExpressionAdd, {"A": a, "B": b})
    def sub(self, a, b): return self.op(unreal.MaterialExpressionSubtract, {"A": a, "B": b})
    def div(self, a, b): return self.op(unreal.MaterialExpressionDivide, {"A": a, "B": b})
    def mx(self, a, b): return self.op(unreal.MaterialExpressionMax, {"A": a, "B": b})
    def frac(self, a): return self.node_in(unreal.MaterialExpressionFrac, self._e(a))
    def floor(self, a): return self.node_in(unreal.MaterialExpressionFloor, self._e(a))
    def absv(self, a): return self.node_in(unreal.MaterialExpressionAbs, self._e(a))
    def sat(self, a): return self.node_in(unreal.MaterialExpressionSaturate, self._e(a))
    def sine(self, a): return self.node_in(unreal.MaterialExpressionSine, self._e(a), period=1.0)
    def smooth(self, lo, hi, v): return self.op(unreal.MaterialExpressionSmoothStep, {"Min": lo, "Max": hi, "Value": v})
    def lerp(self, a, b, t): return self.op(unreal.MaterialExpressionLinearInterpolate, {"A": a, "B": b, "Alpha": t})

    def _e(self, a):
        return self.c(a) if isinstance(a, (int, float)) else a


def make_boundary_material(mpc_boundary):
    path = f"{MAT}/M_Boundary"
    if EAL.does_asset_exist(path):
        if not FORCE_MAT:
            log("M_Boundary exists (DELHI_FORCE_MATERIALS=1 to rebuild)")
            return unreal.load_asset(path)
        EAL.delete_asset(path)
    m = AT.create_asset("M_Boundary", MAT, unreal.Material, unreal.MaterialFactoryNew())
    setp(m, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "two_sided", True)
    try:
        setp(m, "refraction_method", unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET)
    except Exception:
        log("WARN refraction method enum not found; leaving default")
    g = Graph(m)

    # --- per-segment inputs from AWorldBoundary's dynamic material instances
    wp = g.node(unreal.MaterialExpressionWorldPosition)
    seg = g.node(unreal.MaterialExpressionVectorParameter, parameter_name="Seg", default_value=unreal.LinearColor(0, 0, 0, 0))
    seg_dir = g.node(unreal.MaterialExpressionVectorParameter, parameter_name="SegDir", default_value=unreal.LinearColor(1, 0, 0, 0))
    cell = g.node(unreal.MaterialExpressionScalarParameter, parameter_name="Cell", default_value=60.0)
    intensity = g.node(unreal.MaterialExpressionScalarParameter, parameter_name="Intensity", default_value=8.0)
    line_w = g.node(unreal.MaterialExpressionScalarParameter, parameter_name="LineWidth", default_value=0.035)

    # --- (Lattice) along-wall coordinate h rebuilt from the segment anchor, so joints do not tear
    rel_x = g.sub(g.mask(wp, "r"), g.mask(seg, "r"))
    rel_y = g.sub(g.mask(wp, "g"), g.mask(seg, "g"))
    h = g.add(g.add(g.mul(rel_x, g.mask(seg_dir, "r")), g.mul(rel_y, g.mask(seg_dir, "g"))), g.mask(seg, "b"))
    v = g.mask(wp, "b")

    # --- (Glitch) time-stepped row bands that jump by a fraction of a cell
    row = g.floor(g.div(v, g.mul(cell, 0.5)))
    tstep = g.floor(g.mul(g.node(unreal.MaterialExpressionTime), 6.0))
    rnd = g.frac(g.mul(g.sine(g.add(g.mul(row, 12.9898), g.mul(tstep, 78.233))), 43758.5453))
    glitch = g.smooth(0.94, 0.9401, rnd)
    hs = g.add(h, g.mul(glitch, g.mul(cell, 0.37)))

    # --- triangular lattice from three line families at 0, 60, 120 degrees (the dual of the hex pattern)
    u1 = g.div(hs, cell)
    u2 = g.div(g.add(g.mul(hs, 0.5), g.mul(v, 0.866025)), cell)
    u3 = g.div(g.sub(g.mul(hs, 0.5), g.mul(v, 0.866025)), cell)
    lo = g.sub(0.5, line_w)

    def line(u):
        return g.smooth(lo, 0.5, g.absv(g.sub(g.frac(u), 0.5)))

    lattice = g.mx(g.mx(line(u1), line(u2)), line(u3))

    # --- (Reveal) only the patch near the player
    player = g.node(unreal.MaterialExpressionCollectionParameter, collection=mpc_boundary, parameter_name="PlayerPos")
    radius = g.node(unreal.MaterialExpressionCollectionParameter, collection=mpc_boundary, parameter_name="RevealRadius")
    reveal = g.op(unreal.MaterialExpressionSphereMask, {"A": wp, "B": player, "Radius": radius, "Hardness": 0.4})

    # --- rim, ground fade (80 cm)
    fres = g.op(unreal.MaterialExpressionFresnel, {"ExponentIn": 2.0, "BaseReflectFractionIn": 0.1})
    rim = g.lerp(0.4, 1.0, fres)
    ground = g.sat(g.div(g.sub(v, g.mask(seg, "a")), 80.0))

    mask = g.mul(g.mul(lattice, reveal), ground)
    colour = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0.55, 0.95, 1.0, 1.0))
    flash = g.add(1.0, g.mul(glitch, 2.0))
    emissive = g.mul(g.mul(colour, g.mul(g.mul(mask, rim), flash)), intensity)
    opacity = g.mul(mask, 0.7)
    refraction = g.lerp(1.0, 1.08, g.sat(g.mul(reveal, lattice)))

    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(refraction, "", unreal.MaterialProperty.MP_REFRACTION)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    log("M_Boundary built")
    return m


# ------------------------------------------------------------------------------------------------ level actors
def spawn(cls, loc=(0, 0, 0), rot=(0, 0, 0), label=None):
    a = ACT.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if a and label:
        a.set_actor_label(label)
    return a


def clear_existing():
    if KEEP:
        return
    kill = []
    for name in ("DirectionalLight", "SkyAtmosphere", "SkyLight", "ExponentialHeightFog", "VolumetricCloud",
                 "PostProcessVolume", "DelhiSkyRig", "DelhiWeatherController", "WorldBoundary"):
        cls = getattr(unreal, name, None)
        if cls is None:
            continue
        for a in ACT.get_all_level_actors():
            if not isinstance(a, cls):
                continue
            if name == "PostProcessVolume":  # keep bounded volumes (interiors, cameras); replace only global ones
                try:
                    if not a.get_editor_property("unbound"):
                        continue
                except Exception:
                    pass
            kill.append(a)
    for a in kill:
        ACT.destroy_actor(a)
    log("removed", len(kill), "existing sky/weather/boundary actors")


def comp(actor, *names):
    for n in names:
        try:
            c = actor.get_editor_property(n)
            if c:
                return c
        except Exception:
            pass
    return None


def fallback_sky(az, el, preset):
    """No OldDelhi module yet: plain engine actors with the same numbers (static for the chosen moment)."""
    aod, cloud, temp, diffuse, ev, ms, ma, g_, h_, fd, ff, vext = PRESETS[preset]
    north_yaw = -90.0  # project frame: X east, Y south
    sun = spawn(unreal.DirectionalLight, (0, 0, GROUND_Z + 50000), (-el, north_yaw + az + 180.0, 0), "Sun")
    sc = comp(sun, "directional_light_component", "light_component")
    setp(sc, "intensity", 127500.0 * (1 - 0.95 * cloud ** 1.3))  # top-of-atmosphere; the atmosphere attenuates
    setp(sc, ["atmosphere_sun_light", "b_atmosphere_sun_light"], True)
    setp(sc, ["use_temperature", "b_use_temperature"], True)
    setp(sc, "temperature", float(temp))
    setp(sc, "light_source_angle", 0.53)
    sky = spawn(unreal.SkyAtmosphere, (0, 0, GROUND_Z), label="SkyAtmosphere")
    skc = comp(sky, "sky_atmosphere_component", "component")
    # Engine defaults: Rayleigh 0.0331, Mie scattering 0.003996, Mie absorption 0.00444 (per km).
    setp(skc, "rayleigh_scattering_scale", 0.0331)
    setp(skc, "mie_scattering_scale", 0.003996 * ms)
    setp(skc, "mie_absorption_scale", 0.00444 * ma)
    setp(skc, "mie_anisotropy", g_)
    setp(skc, "mie_exponential_distribution", h_)
    sl = spawn(unreal.SkyLight, (0, 0, GROUND_Z + 2000), label="SkyLight")
    slc = comp(sl, "light_component", "sky_light_component")
    setp(slc, ["real_time_capture", "b_real_time_capture"], True)
    setp(slc, "mobility", unreal.ComponentMobility.MOVABLE)
    fog = spawn(unreal.ExponentialHeightFog, (0, 0, GROUND_Z), label="HeightFog")
    fc = comp(fog, "component", "exponential_height_fog_component")
    setp(fc, "fog_density", fd)
    setp(fc, "fog_height_falloff", ff)
    setp(fc, ["volumetric_fog", "b_volumetric_fog", "enable_volumetric_fog"], True)
    setp(fc, "volumetric_fog_extinction_scale", vext)
    spawn(unreal.VolumetricCloud, (0, 0, GROUND_Z), label="VolumetricCloud")
    ppv = spawn(unreal.PostProcessVolume, (0, 0, GROUND_Z), label="PhysicalExposure")
    setp(ppv, ["unbound", "b_unbound"], True)
    s = ppv.get_editor_property("settings")
    for k, v in (("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM),
                 ("auto_exposure_min_brightness", max(1.0, ev - 3.0)), ("auto_exposure_max_brightness", min(15.0, ev + 1.5)),
                 ("auto_exposure_speed_up", 3.0), ("auto_exposure_speed_down", 1.0),
                 ("bloom_intensity", 0.4), ("vignette_intensity", 0.25), ("film_grain_intensity", 0.06)):
        setp(s, "override_" + k, True)
        setp(s, k, v)
    ppv.set_editor_property("settings", s)


def weather_enum(name):
    """'DustStorm' -> unreal.DelhiWeather.DUST_STORM (python upper-snake-cases UENUM entries)."""
    snake = re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()
    for cand in (snake, name.upper()):
        if hasattr(unreal.DelhiWeather, cand):
            return getattr(unreal.DelhiWeather, cand)
    return unreal.DelhiWeather.HAZE


def spawn_rig(preset, weather_mpc):
    lat, lon, tz = GEO[CITY]
    az, el = sun_position(WHEN, lat, lon, tz)
    aod, cloud = PRESETS[preset][0], PRESETS[preset][1]
    log(f"{CITY} {WHEN:%Y-%m-%d %H:%M} -> sun az {az:.1f} el {el:.1f} deg, direct about {sun_lux(el, aod, cloud):.0f} lux "
        f"({preset}, AOD {aod}), target EV100 about {PRESETS[preset][4]} at noon")

    rig_cls = getattr(unreal, "DelhiSkyRig", None)
    if rig_cls is None:
        log("WARN DelhiSkyRig class not found (compile the OldDelhi module first): using the plain-actor fallback")
        fallback_sky(az, el, preset)
        return None
    rig = spawn(rig_cls, (0, 0, GROUND_Z), label="DelhiSkyRig")
    setp(rig, "city", unreal.AtmosphereCity.KARACHI if CITY == "karachi" else unreal.AtmosphereCity.DELHI)
    setp(rig, "preview_year", WHEN.year)
    setp(rig, "preview_month", WHEN.month)
    setp(rig, "preview_day", WHEN.day)
    setp(rig, "preview_hour", WHEN.hour + WHEN.minute / 60.0)
    setp(rig, "preview_weather", weather_enum(preset))
    setp(rig, "north_yaw_degrees", -90.0)
    try:
        rig.refresh_sky()
    except Exception:
        pass
    return rig


def spawn_controller(weather_mpc, preset_forced):
    cls = getattr(unreal, "DelhiWeatherController", None)
    if cls is None:
        return None
    wc = spawn(cls, (0, 0, GROUND_Z), label="DelhiWeatherController")
    setp(wc, "city", unreal.AtmosphereCity.KARACHI if CITY == "karachi" else unreal.AtmosphereCity.DELHI)
    setp(wc, "start_year", WHEN.year)
    setp(wc, "start_month", WHEN.month)
    setp(wc, "start_day", WHEN.day)
    setp(wc, "start_hour", WHEN.hour + WHEN.minute / 60.0)
    setp(wc, "weather_collection", weather_mpc)
    if preset_forced:
        setp(wc, ["force_initial_weather", "b_force_initial_weather"], True)
        setp(wc, "initial_weather", weather_enum(preset_forced))
    return wc


def spawn_boundary():
    cls = getattr(unreal, "WorldBoundary", None)
    if cls is None:
        log("WARN WorldBoundary class not found; skipping boundary")
        return None
    b = spawn(cls, (0, 0, GROUND_Z), label="WorldBoundary")
    try:
        half = BOUNDARY_HALF_M * 100.0
        b.set_rectangle(unreal.Vector(0, 0, GROUND_Z), half, half)
    except Exception as ex:
        log("WARN set_rectangle failed:", ex)
    log("boundary spawned: edit its spline (or call SetPolygonFromLatLon with the Netaji Subhash Marg / railway / "
        "S P Mukherjee Marg / Asaf Ali Road outline), then 'Snap Spline To Ground'")
    return b


def main():
    preset = WEATHER if WEATHER in PRESETS else "Haze"
    if WEATHER and WEATHER not in PRESETS:
        log("WARN unknown DELHI_WEATHER", WEATHER, "-> Haze. Valid:", ", ".join(PRESETS))
    weather_mpc, boundary_mpc = make_mpcs()
    make_boundary_material(boundary_mpc)
    clear_existing()
    spawn_rig(preset, weather_mpc)
    spawn_controller(weather_mpc, WEATHER if WEATHER in PRESETS else "")
    if WANT_BOUNDARY:
        spawn_boundary()
    LVL.save_current_level()
    log("done. Remember: Project Settings > Rendering: Extend Default Luminance Range in Auto Exposure ON, "
        "Lumen GI + reflections, Virtual Shadow Maps, volumetric fog (see docs/ATMOSPHERE.md)")


main()
