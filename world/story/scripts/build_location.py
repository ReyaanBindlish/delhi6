"""Adapted from world/karachi/scripts/lyari_build.py (original untouched).

blender --background --python build_location.py -- <location> [--lod-radii R0 R1] [--limit N] [--no-export]
Inputs : export/<loc>/_work/slice.json (prep_location.py), terrain/<loc>/ (make_terrain_story.py)
Outputs: export/<loc>/buildings/*.fbx (+UCX), station/*.fbx (platforms, canopies, tracks, FOBs, stairs, signals,
         viaducts), Roads.fbx, Overhead_Wires.fbx, manifest.json; export/<loc>/_work/<loc>.blend
Extras vs the original: style from the slice, LOD tiers by distance (grills/slats/frames thinned outside the core),
used-materials-only slots, platform / canopy / track / footbridge / stair / viaduct builders.

Original description follows.

Per building, real geometry (no painted-on detail):
  - outer walls built bay-by-bay with true openings (reveals 0.2 m deep, frames, recessed glass)
  - welded iron window grills (individual bars), rolling shutters + shutter boxes on street-level shops
  - cantilevered balcony slabs with railings, projecting floor-slab bands, roof slab
  - parapet walls, black PVC water tanks, stair-head rooms, split-AC outdoor units, shop signboards
  - party walls (touching a neighbour) stay blank, like real row buildings
Physics: every building gets UCX_ convex collision pieces (Unreal auto-imports them as simple
collision); ground and roads get their own collision. Units: metres, Z up.
"""
import math
import random
import sys
import json
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

import argparse
sys.path.insert(0, str(Path(__file__).resolve().parent))
import loc_config as LC

argv = sys.argv[sys.argv.index("--") + 1:]
_ap = argparse.ArgumentParser()
_ap.add_argument("location", choices=sorted(LC.LOCATIONS))
_ap.add_argument("--lod-radii", type=float, nargs=2, default=None,
                 help="fractions of the half-size: LOD0 inside r0, LOD1 inside r1, LOD2 outside")
_ap.add_argument("--limit", type=int, default=0, help="debug: build only N buildings")
_ap.add_argument("--no-export", action="store_true")
_a = _ap.parse_args(argv)
PP = LC.paths(_a.location)
SRC, TEX = str(PP["slice"]), LC.TEX_DIR
OUT = str(PP["work"] / _a.location)
TERRAIN = PP["terrain"]
EXPORT_DIR = None if _a.no_export else PP["out"]
data = json.load(open(SRC))
STYLE = data["style"]
CUR_LOD = 0
LOD_R = _a.lod_radii or ([0.22, 0.5] if _a.location == "gulbahar_lines" else [0.55, 0.85])

# ---- world frame: with a terrain, everything is placed relative to the terrain centre, on real ground
OX = OY = 0.0


def ground(x, y):
    return 0.0


if TERRAIN:
    import numpy as np
    TMETA = json.load(open(TERRAIN / "terrain.json"))
    DTM = np.load(TERRAIN / TMETA["dtm_file"])
    tlat, tlon = TMETA["centre_latlon"]
    _R = 6378137.0
    _mlat = math.radians(1) * _R
    _mlon = math.radians(1) * _R * math.cos(math.radians(tlat))
    slat, slon = data["center"]
    OX, OY = (slon - tlon) * _mlon, (slat - tlat) * _mlat
    _half = TMETA["extent_m"] / 2; _px = TMETA["pixel_m"]; _base = TMETA["base_elev_m"]; _n = DTM.shape[0]

    def ground(x, y):
        """Terrain height (m above base) at world x (east), y (north)."""
        c = (x + _half) / _px; r = (_half - y) / _px
        c = min(max(c, 0), _n - 1.001); r = min(max(r, 0), _n - 1.001)
        c0, r0 = int(c), int(r); fc, fr = c - c0, r - r0
        h = (DTM[r0, c0] * (1 - fc) * (1 - fr) + DTM[r0, c0 + 1] * fc * (1 - fr)
             + DTM[r0 + 1, c0] * (1 - fc) * fr + DTM[r0 + 1, c0 + 1] * fc * fr)
        return float(h) - _base

    print(f"terrain frame: slice offset ({OX:.1f}, {OY:.1f}) m", flush=True)
MANIFEST = []

FH = 3.05          # floor-to-floor height
WALL_D = 0.22      # wall depth used for reveals
PALETTE = [  # faded Karachi paint colours (linear-ish RGB)
    (0.62, 0.80, 0.70), (0.55, 0.70, 0.85), (0.90, 0.75, 0.70), (0.92, 0.86, 0.62),
    (0.85, 0.85, 0.80), (0.70, 0.72, 0.75), (0.80, 0.62, 0.50), (0.95, 0.92, 0.85),
    (0.60, 0.75, 0.55), (0.88, 0.70, 0.78), (0.75, 0.75, 0.70), (0.98, 0.95, 0.90),
]
if STYLE == "delhi":  # Old Delhi: ochre, dusty pink, whitewash, faded yellow, sandstone
    PALETTE = [(0.93, 0.80, 0.55), (0.90, 0.68, 0.62), (0.96, 0.94, 0.88), (0.95, 0.87, 0.58),
               (0.82, 0.62, 0.45), (0.86, 0.86, 0.80), (0.72, 0.80, 0.78), (0.98, 0.90, 0.80)]
SIGN_COLS =[(0.8, 0.1, 0.1), (0.1, 0.3, 0.8), (0.1, 0.55, 0.2), (0.9, 0.7, 0.1), (0.95, 0.95, 0.95)]

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


# ------------------------------------------------------------------ materials
def img(name, cs="sRGB"):
    p = TEX / name
    im = bpy.data.images.load(str(p), check_existing=True)
    im.colorspace_settings.name = cs
    return im


def pbr(name, tex_id, tint_attr=False, base=None, rough=None, metal=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; Lk = nt.links
    bsdf = N["Principled BSDF"]
    bsdf.inputs["Metallic"].default_value = metal
    if tex_id:
        d = N.new("ShaderNodeTexImage"); d.image = img(f"{tex_id}_diff.jpg")
        r = N.new("ShaderNodeTexImage"); r.image = img(f"{tex_id}_rough.jpg", "Non-Color")
        n = N.new("ShaderNodeTexImage"); n.image = img(f"{tex_id}_nor.jpg", "Non-Color")
        nm = N.new("ShaderNodeNormalMap")
        Lk.new(n.outputs["Color"], nm.inputs["Color"]); Lk.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
        Lk.new(r.outputs["Color"], bsdf.inputs["Roughness"])
        col = d.outputs["Color"]
        if tint_attr:
            ca = N.new("ShaderNodeVertexColor"); ca.layer_name = "paint"
            mix = N.new("ShaderNodeMix"); mix.data_type = "RGBA"; mix.blend_type = "MULTIPLY"
            mix.inputs["Factor"].default_value = 1.0
            Lk.new(col, mix.inputs[6]); Lk.new(ca.outputs["Color"], mix.inputs[7])
            col = mix.outputs[2]
        Lk.new(col, bsdf.inputs["Base Color"])
    if base:
        bsdf.inputs["Base Color"].default_value = (*base, 1)
    if rough is not None:
        bsdf.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*(base or (0.6, 0.6, 0.6)), 1)
    return m


MAT = [
    pbr("M_PaintedPlaster", "painted_plaster_wall", tint_attr=True, base=(0.8, 0.78, 0.72)),  # 0 walls
    pbr("M_Concrete", "concrete_wall_008", base=(0.5, 0.5, 0.48)),                             # 1 slabs
    pbr("M_Glass", None, base=(0.05, 0.07, 0.08), rough=0.05, metal=0.0),                     # 2 glass
    pbr("M_RustyMetal", "rusty_metal", base=(0.25, 0.2, 0.18), metal=0.6),                    # 3 grills/rails
    pbr("M_RollingShutter", "rusted_shutter", base=(0.45, 0.42, 0.4), metal=0.5),             # 4 shutters
    pbr("M_PVCTank", None, base=(0.02, 0.02, 0.02), rough=0.45),                               # 5 tanks
    pbr("M_RoofConcrete", "concrete_floor_worn_001", base=(0.45, 0.44, 0.42)),                # 6 roof
    pbr("M_Signboard", None, base=(0.8, 0.2, 0.1), rough=0.6),                                 # 7 signs
    pbr("M_ACUnit", None, base=(0.85, 0.85, 0.82), rough=0.5),                                 # 8 AC
    pbr("M_DamagedPlaster", "damaged_plaster", tint_attr=True, base=(0.75, 0.72, 0.68)),      # 9 old walls
    pbr("M_Brick", "brick_wall_001", base=(0.55, 0.35, 0.25)),                                 # 10 bare brick
    pbr("M_Cable", None, base=(0.01, 0.01, 0.01), rough=0.4),                                  # 11 wires
    pbr("M_PlatformSlab", "concrete_floor_worn_001", base=(0.55, 0.53, 0.5)),                       # 12 platform / bridge deck
    pbr("M_Ballast", "concrete_floor_worn_001", base=(0.16, 0.15, 0.14), rough=0.95),               # 13 track ballast
    pbr("M_Rail", "rusty_metal", base=(0.12, 0.11, 0.1), metal=0.8),                            # 14 rails / steel
    pbr("M_CanopyMetal", "rusted_shutter", base=(0.5, 0.52, 0.55), metal=0.5),                  # 15 canopy sheet
    pbr("M_SafetyLine", None, base=(0.9, 0.72, 0.05), rough=0.7),                               # 16 yellow line
    pbr("M_Sleeper", "concrete_wall_008", base=(0.35, 0.34, 0.33)),                             # 17 sleepers
]
MI_PLATFORM, MI_BALLAST, MI_RAIL, MI_CANOPY, MI_YELLOW, MI_SLEEPER = 12, 13, 14, 15, 16, 17
# grills/railings: dark painted steel with rust showing through, not bare orange rust
_rm = MAT[3].node_tree
_mix = _rm.nodes.new("ShaderNodeMix"); _mix.data_type = "RGBA"; _mix.blend_type = "MULTIPLY"
_mix.inputs["Factor"].default_value = 1.0; _mix.inputs[7].default_value = (0.25, 0.25, 0.27, 1)
_tex = [n for n in _rm.nodes if n.type == "TEX_IMAGE" and "diff" in n.image.name][0]
_rm.links.new(_tex.outputs["Color"], _mix.inputs[6])
_rm.links.new(_mix.outputs[2], _rm.nodes["Principled BSDF"].inputs["Base Color"])
# sign colour from vertex colour too
sn = MAT[7].node_tree
ca = sn.nodes.new("ShaderNodeVertexColor"); ca.layer_name = "paint"
sn.links.new(ca.outputs["Color"], sn.nodes["Principled BSDF"].inputs["Base Color"])
M_ASPHALT = pbr("M_Asphalt", "asphalt_02", base=(0.1, 0.1, 0.1))
M_GROUND = pbr("M_GroundConcrete", "concrete_floor_worn_001", base=(0.4, 0.38, 0.35))


# ------------------------------------------------------------------ geometry helpers
class G:
    """Accumulates faces with material index into one bmesh."""

    def __init__(self):
        self.bm = bmesh.new()

    def quad(self, pts, mat):
        vs = [self.bm.verts.new(p) for p in pts]
        try:
            f = self.bm.faces.new(vs); f.material_index = mat
        except ValueError:
            pass

    def box(self, c, x, y, z, sx, sy, sz, mat, caps=True):
        """Oriented box: centre c, unit axes x,y,z (Vectors), half-sizes."""
        p = [c + x * a * sx + y * b * sy + z * d * sz for d in (-1, 1) for b in (-1, 1) for a in (-1, 1)]
        F = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
        if not caps:
            F = F[2:]
        for f in F:
            self.quad([p[i] for i in f], mat)

    def cylinder(self, c, r, h, mat, seg=14):
        top, bot = [], []
        for k in range(seg):
            a = 2 * math.pi * k / seg
            off = Vector((math.cos(a) * r, math.sin(a) * r, 0))
            bot.append(self.bm.verts.new(c + off)); top.append(self.bm.verts.new(c + off + Vector((0, 0, h))))
        for k in range(seg):
            j = (k + 1) % seg
            f = self.bm.faces.new((bot[k], bot[j], top[j], top[k])); f.material_index = mat
        f = self.bm.faces.new(top); f.material_index = mat
        # domed lid ring
        lid = self.bm.verts.new(c + Vector((0, 0, h + r * 0.25)))
        for k in range(seg):
            f = self.bm.faces.new((top[k], top[(k + 1) % seg], lid)); f.material_index = mat


def wall_with_opening(g, a, t, n, u0, u1, z0, z1, op, wall_mat):
    """Wall panel from u0..u1 along tangent t, z0..z1; op=(ou0,ou1,oz0,oz1) opening or None.
    Builds outer face around the hole + 4 reveal faces going WALL_D inward."""
    P = lambda u, z, d=0.0: Vector((a.x + t.x * u - n.x * d, a.y + t.y * u - n.y * d, z))
    if op is None:
        g.quad([P(u0, z0), P(u1, z0), P(u1, z1), P(u0, z1)], wall_mat); return
    ou0, ou1, oz0, oz1 = op
    g.quad([P(u0, z0), P(ou0, z0), P(ou0, z1), P(u0, z1)], wall_mat)          # left
    g.quad([P(ou1, z0), P(u1, z0), P(u1, z1), P(ou1, z1)], wall_mat)          # right
    if oz0 > z0 + 1e-3:
        g.quad([P(ou0, z0), P(ou1, z0), P(ou1, oz0), P(ou0, oz0)], wall_mat)  # below
    if oz1 < z1 - 1e-3:
        g.quad([P(ou0, oz1), P(ou1, oz1), P(ou1, z1), P(ou0, z1)], wall_mat)  # above
    D = WALL_D
    g.quad([P(ou0, oz0), P(ou0, oz0, D), P(ou0, oz1, D), P(ou0, oz1)], 1)     # reveals (concrete)
    g.quad([P(ou1, oz0), P(ou1, oz1), P(ou1, oz1, D), P(ou1, oz0, D)], 1)
    g.quad([P(ou0, oz1), P(ou0, oz1, D), P(ou1, oz1, D), P(ou1, oz1)], 1)
    if oz0 > z0 + 1e-3:
        g.quad([P(ou0, oz0), P(ou1, oz0), P(ou1, oz0, D), P(ou0, oz0, D)], 1)
    else:  # threshold
        g.quad([P(ou0, oz0 + 0.02), P(ou1, oz0 + 0.02), P(ou1, oz0 + 0.02, D), P(ou0, oz0 + 0.02, D)], 1)


def window(g, a, t, n, ou0, ou1, oz0, oz1, rnd, grill=True):
    P = lambda u, z, d=0.0: Vector((a.x + t.x * u - n.x * d, a.y + t.y * u - n.y * d, z))
    up = Vector((0, 0, 1)); tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
    gd = 0.13
    g.quad([P(ou0, oz0, gd), P(ou1, oz0, gd), P(ou1, oz1, gd), P(ou0, oz1, gd)], 2)  # glass
    # frame (aluminium/steel) + mullion
    fw = 0.045
    um, zm = (ou0 + ou1) / 2, (oz0 + oz1) / 2
    if CUR_LOD < 2:
        for (cu, cz, su, sz) in ((ou0 + fw, zm, fw, (oz1 - oz0) / 2), (ou1 - fw, zm, fw, (oz1 - oz0) / 2),
                                 (um, oz0 + fw, (ou1 - ou0) / 2, fw), (um, oz1 - fw, (ou1 - ou0) / 2, fw),
                                 (um, zm, fw * 0.7, (oz1 - oz0) / 2)):
            g.box(P(cu, cz, gd - 0.03), tv, nv, up, su, 0.03, sz, 3, caps=False)
    # concrete sunshade (chajja) over the window - very common in Karachi
    if rnd.random() < 0.7:
        g.box(P(um, oz1 + 0.06, -0.22), tv, nv, up, (ou1 - ou0) / 2 + 0.15, 0.25, 0.04, 1)
    if grill and CUR_LOD < 2:
        # welded iron grill proud of the wall: verticals + 2 horizontals (sparser verticals at LOD1)
        sp = 0.13 if CUR_LOD == 0 else 0.32
        k = int((ou1 - ou0 - 0.1) / sp)
        for i in range(1, k + 1):
            u = ou0 + 0.05 + i * (ou1 - ou0 - 0.1) / (k + 1)
            g.box(P(u, zm, -0.04), tv, nv, up, 0.008, 0.008, (oz1 - oz0) / 2, 3, caps=False)
        for z in (oz0 + 0.25, oz1 - 0.25):
            g.box(P(um, z, -0.04), tv, nv, up, (ou1 - ou0) / 2, 0.012, 0.012, 3, caps=False)
        for u in (ou0, ou1):
            g.box(P(u, zm, -0.02), tv, nv, up, 0.015, 0.03, (oz1 - oz0) / 2, 3)


def shutter(g, a, t, n, ou0, ou1, oz0, oz1, rnd, sign_col):
    P = lambda u, z, d=0.0: Vector((a.x + t.x * u - n.x * d, a.y + t.y * u - n.y * d, z))
    up = Vector((0, 0, 1)); tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
    um = (ou0 + ou1) / 2
    openness = rnd.choice([0.0, 0.0, 0.3, 1.0])  # some shops half/fully open
    zc = oz1 - (oz1 - oz0) * (1 - openness)
    # corrugated shutter: real ribs as geometry (slats) on a backing plane
    if zc < oz1 - 0.05:
        zb = zc
        g.quad([P(ou0, zb, 0.05), P(ou1, zb, 0.05), P(ou1, oz1, 0.05), P(ou0, oz1, 0.05)], 4)
        k = int((oz1 - zb) / 0.09) if CUR_LOD == 0 else 0
        for i in range(k):
            z = zb + (i + 0.5) * (oz1 - zb) / k
            g.box(P(um, z, 0.035), tv, nv, up, (ou1 - ou0) / 2, 0.015, 0.012, 4, caps=False)
        g.box(P(um, zb + 0.03, 0.03), tv, nv, up, (ou1 - ou0) / 2, 0.03, 0.03, 3)  # bottom bar
    if zc > oz0 + 0.05:  # open part shows dark shop interior
        g.quad([P(ou0, oz0, 2.0), P(ou1, oz0, 2.0), P(ou1, zc, 2.0), P(ou0, zc, 2.0)], 5)
        g.quad([P(ou0, oz0 + 0.01, 0), P(ou1, oz0 + 0.01, 0), P(ou1, oz0 + 0.01, 2.0), P(ou0, oz0 + 0.01, 2.0)], 6)
        for u in (ou0, ou1):
            g.quad([P(u, oz0, 0), P(u, oz0, 2.0), P(u, zc, 2.0), P(u, zc, 0)], 1)
    # shutter roll box + guide rails
    g.box(P(um, oz1 + 0.15, -0.12), tv, nv, up, (ou1 - ou0) / 2 + 0.05, 0.14, 0.15, 3)
    for u in (ou0 + 0.02, ou1 - 0.02):
        g.box(P(u, (oz0 + oz1) / 2, -0.02), tv, nv, up, 0.02, 0.04, (oz1 - oz0) / 2, 3)
    # signboard
    if rnd.random() < 0.75:
        sid = len(sign_col)
        sign_col.append(rnd.choice(SIGN_COLS))
        return (P(um, oz1 + 0.6, -0.1), tv, nv, up, (ou1 - ou0) / 2 + 0.2, 0.05, 0.28)
    return None


def balcony(g, a, t, n, u0, u1, z, rnd):
    P = lambda u, zz, d=0.0: Vector((a.x + t.x * u - n.x * d, a.y + t.y * u - n.y * d, zz))
    up = Vector((0, 0, 1)); tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
    depth = rnd.uniform(0.8, 1.2)
    um = (u0 + u1) / 2; hw = (u1 - u0) / 2
    g.box(P(um, z - 0.07, -depth / 2), tv, nv, up, hw, depth / 2, 0.07, 1)  # cantilever slab
    solid = rnd.random() < 0.4
    if solid:  # low parapet wall balcony
        g.box(P(um, z + 0.5, -depth + 0.06), tv, nv, up, hw, 0.06, 0.5, 0)
        for s in (-1, 1):
            g.box(P(um + s * (hw - 0.06), z + 0.5, -depth / 2), tv, nv, up, 0.06, depth / 2, 0.5, 0)
    else:  # iron railing
        g.box(P(um, z + 1.0, -depth + 0.02), tv, nv, up, hw, 0.02, 0.02, 3)
        k = int(2 * hw / (0.12 if CUR_LOD == 0 else 0.4)) if CUR_LOD < 2 else 0
        for i in range(k + 1):
            u = u0 + i * (u1 - u0) / max(1, k)
            g.box(P(u, z + 0.5, -depth + 0.02), tv, nv, up, 0.008, 0.008, 0.5, 3, caps=False)
        for s in (-1, 1):
            g.box(P(um + s * hw, z + 1.0, -depth / 2), tv, nv, up, 0.02, depth / 2, 0.02, 3)
    # laundry line / clutter skipped; maybe a chair later


def ac_unit(g, a, t, n, u, z):
    if CUR_LOD >= 2:
        return
    P = lambda uu, zz, d=0.0: Vector((a.x + t.x * uu - n.x * d, a.y + t.y * uu - n.y * d, zz))
    up = Vector((0, 0, 1)); tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
    g.box(P(u, z, -0.2), tv, nv, up, 0.4, 0.15, 0.28, 8)
    for s in (-1, 1):  # brackets
        g.box(P(u + s * 0.3, z - 0.3, -0.15), tv, nv, up, 0.015, 0.17, 0.015, 3)


def point_in_poly(x, y, poly):
    ins = False
    for i in range(len(poly)):
        x1, y1 = poly[i]; x2, y2 = poly[(i + 1) % len(poly)]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1 + 1e-12) + x1:
            ins = not ins
    return ins


def box_uv(bm, scale=2.0):
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        nx, ny, nz = (abs(c) for c in f.normal)
        for l in f.loops:
            co = l.vert.co
            if nz >= nx and nz >= ny:
                l[uv].uv = (co.x / scale, co.y / scale)
            elif nx >= ny:
                l[uv].uv = (co.y / scale, co.z / scale)
            else:
                l[uv].uv = (co.x / scale, co.z / scale)


# ------------------------------------------------------------------ build
coll_b = bpy.data.collections.new("Buildings"); scene.collection.children.link(coll_b)
coll_c = bpy.data.collections.new("Collision_UCX"); scene.collection.children.link(coll_c)
coll_g = bpy.data.collections.new("Ground_Roads"); scene.collection.children.link(coll_g)

total_faces = 0
ANCHORS = []  # (point, outward normal) on street-facing walls, for overhead cables
_blds = data["buildings"][:_a.limit] if _a.limit else data["buildings"]
_half = data["size"] / 2
for bi, B in enumerate(_blds):
    rnd = random.Random(B["seed"])
    _r = max(abs(sum(p[0] for p in B["footprint"]) / len(B["footprint"])),
             abs(sum(p[1] for p in B["footprint"]) / len(B["footprint"]))) / _half
    CUR_LOD = 0 if _r < LOD_R[0] else 1 if _r < LOD_R[1] else 2
    if B.get("kind") == "religious":
        CUR_LOD = 0
        B["edges"] = [dict(e, street=False) for e in B["edges"]]   # no shop shutters on a shrine / mosque / temple
    # pivot at footprint centroid; geometry built relative to it
    CX = sum(p[0] for p in B["footprint"]) / len(B["footprint"])
    CY = sum(p[1] for p in B["footprint"]) / len(B["footprint"])
    fp = [Vector((x - CX, y - CY)) for x, y in B["footprint"]]
    ZB = min(ground(x + OX, y + OY) for x, y in B["footprint"])  # sit on lowest ground point
    WORLD = Vector((CX + OX, CY + OY, ZB))
    nfl = B["floors"]
    H = nfl * FH
    g = G()
    paint = rnd.choice(PALETTE)
    if rnd.random() < 0.18:
        paint = (0.62, 0.61, 0.58)  # bare grey cement render
    wall_mat = 9 if rnd.random() < 0.35 else 0
    if STYLE == "delhi" and rnd.random() < 0.3:
        wall_mat = 10  # exposed brick (unplastered / Lakhori)
    signs = []
    sign_boxes = []
    for ei, E in enumerate(B["edges"]):
        a = fp[ei]; b = fp[(ei + 1) % len(fp)]
        d = b - a; L = d.length
        if L < 1e-3:
            continue
        t = d / L; n = Vector((t.y, -t.x))
        if E["street"]:
            for k in range(int(L / 3) + 1):
                if rnd.random() < 0.7:
                    u = rnd.uniform(0.2, L - 0.2)
                    zz = rnd.uniform(3.4, min(H - 0.3, 7.5)) if H > 3.8 else H - 0.3
                    ANCHORS.append((Vector((a.x + t.x * u + n.x * 0.12, a.y + t.y * u + n.y * 0.12, zz)) + WORLD,
                                    Vector((n.x, n.y, 0))))
        nb = max(1, round(L / 2.7))
        bw = L / nb
        for fl in range(nfl):
            z0, z1 = fl * FH, (fl + 1) * FH
            for k in range(nb):
                u0, u1 = k * bw, (k + 1) * bw
                op = None
                if E["exposed"] and bw > 1.5:
                    if fl == 0 and E["street"]:
                        op = (u0 + 0.25, u1 - 0.25, z0, z0 + 2.6)
                        sb = shutter(g, a, t, n, *op, rnd, signs)
                        if sb:
                            sign_boxes.append(sb)
                    elif fl == 0:
                        if rnd.random() < 0.5:
                            w = min(1.0, bw - 0.6)
                            um = (u0 + u1) / 2
                            op = (um - w / 2, um + w / 2, z0, z0 + 2.2)  # door
                            g.quad([Vector((a.x + t.x * op[0] - n.x * 0.15, a.y + t.y * op[0] - n.y * 0.15, z0)),
                                    Vector((a.x + t.x * op[1] - n.x * 0.15, a.y + t.y * op[1] - n.y * 0.15, z0)),
                                    Vector((a.x + t.x * op[1] - n.x * 0.15, a.y + t.y * op[1] - n.y * 0.15, z0 + 2.2)),
                                    Vector((a.x + t.x * op[0] - n.x * 0.15, a.y + t.y * op[0] - n.y * 0.15, z0 + 2.2))], 3)
                        else:
                            w = min(1.2, bw - 0.6); um = (u0 + u1) / 2
                            op = (um - w / 2, um + w / 2, z0 + 1.0, z0 + 2.2)
                            window(g, a, t, n, *op, rnd, grill=True)
                    else:
                        um = (u0 + u1) / 2
                        if E["street"] and rnd.random() < 0.4:
                            w = min(1.0, bw - 0.7)
                            op = (um - w / 2, um + w / 2, z0, z0 + 2.3)  # balcony door
                            window(g, a, t, n, *op, rnd, grill=False)
                            balcony(g, a, t, n, u0 + 0.1, u1 - 0.1, z0, rnd)
                        elif rnd.random() < 0.85:
                            w = min(rnd.choice([0.9, 1.2, 1.5]), bw - 0.6)
                            op = (um - w / 2, um + w / 2, z0 + 0.9, z0 + 2.2)
                            window(g, a, t, n, *op, rnd, grill=rnd.random() < 0.75)
                            if rnd.random() < 0.3:
                                ac_unit(g, a, t, n, um + (w / 2 + 0.55) * rnd.choice([-1, 1]), z0 + 1.2)
                wall_with_opening(g, a, t, n, u0, u1, z0, z1, op, wall_mat)
            # projecting floor band
            if fl > 0 and E["exposed"]:
                tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
                c = Vector((a.x + d.x / 2 + n.x * 0.05, a.y + d.y / 2 + n.y * 0.05, z0))
                g.box(c, tv, nv, Vector((0, 0, 1)), L / 2, 0.05, 0.09, 1, caps=False)
        # parapet (both faces + cap)
        tv = Vector((t.x, t.y, 0)); nv = Vector((n.x, n.y, 0))
        ph = rnd.choice([0.9, 1.0, 1.1])
        c = Vector((a.x + d.x / 2 - n.x * 0.075, a.y + d.y / 2 - n.y * 0.075, H + ph / 2))
        g.box(c, tv, nv, Vector((0, 0, 1)), L / 2, 0.075, ph / 2, wall_mat)
    # roof slab
    g.quad([Vector((p.x, p.y, H)) for p in fp], 6)
    # rooftop: water tanks + stair room
    xs = [p.x for p in fp]; ys = [p.y for p in fp]
    poly2 = [(p.x, p.y) for p in fp]
    placed = 0
    for _ in range(25):
        if placed >= rnd.choice([1, 1, 2, 3]):
            break
        x = rnd.uniform(min(xs) + 0.8, max(xs) - 0.8); y = rnd.uniform(min(ys) + 0.8, max(ys) - 0.8)
        if point_in_poly(x, y, poly2):
            r = rnd.choice([0.5, 0.6, 0.7])
            # steel stand
            g.box(Vector((x, y, H + 0.25)), Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)), r, r, 0.25, 3)
            g.cylinder(Vector((x, y, H + 0.5)), r, r * 1.8, 5)
            placed += 1
    if B["area"] > 25 and rnd.random() < 0.55:
        for _ in range(20):
            x = rnd.uniform(min(xs) + 1.4, max(xs) - 1.4); y = rnd.uniform(min(ys) + 1.4, max(ys) - 1.4)
            if all(point_in_poly(x + dx, y + dy, poly2) for dx in (-1.3, 1.3) for dy in (-1.3, 1.3)):
                g.box(Vector((x, y, H + 1.3)), Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)),
                      1.25, 1.25, 1.3, wall_mat)
                g.box(Vector((x, y, H + 2.65)), Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)),
                      1.4, 1.4, 0.06, 1)
                break
    for sb in sign_boxes:
        g.box(*sb, 7)

    bm = g.bm
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.002)
    box_uv(bm)
    me = bpy.data.meshes.new(B["id"])
    bm.to_mesh(me); bm.free()
    attr = me.color_attributes.new("paint", "FLOAT_COLOR", "POINT")
    sign_c = signs[0] if signs else paint
    for v in attr.data:
        v.color = (*paint, 1)
    # signboards get their own colour
    if signs:
        for poly in me.polygons:
            if poly.material_index == 7:
                for vi in poly.vertices:
                    attr.data[vi].color = (*sign_c, 1)
    # only the materials this building really uses (keeps Unreal material slots / draw calls low)
    used = sorted({p.material_index for p in me.polygons})
    remap = {o: i for i, o in enumerate(used)}
    for poly in me.polygons:
        poly.material_index = remap[poly.material_index]
    for o in used:
        me.materials.append(MAT[o])
    ob = bpy.data.objects.new(B["id"], me)
    ob.location = WORLD
    coll_b.objects.link(ob)
    total_faces += len(me.polygons)
    MANIFEST.append({"id": B["id"], "location_m": [round(v, 3) for v in WORLD], "floors": nfl,
                     "height_m": round(H, 2), "faces": len(me.polygons), "ucx": len(B["collision"]),
                     "kind": B.get("kind", "house"), "name": B.get("name", ""), "lod": CUR_LOD,
                     "area_m2": round(B["area"], 1), "religious": B.get("kind") == "religious"})

    # UCX collision: one convex prism per piece (walls + roof; parapet excluded), reaching 1 m below grade
    for ci, piece in enumerate(B["collision"]):
        piece = [(x - CX, y - CY) for x, y in piece]
        cb = bmesh.new()
        bot = [cb.verts.new((x, y, -1.0)) for x, y in piece]
        top = [cb.verts.new((x, y, H)) for x, y in piece]
        cb.faces.new(top); cb.faces.new(list(reversed(bot)))
        for i in range(len(piece)):
            j = (i + 1) % len(piece)
            cb.faces.new((bot[i], bot[j], top[j], top[i]))
        bmesh.ops.recalc_face_normals(cb, faces=cb.faces)
        cm = bpy.data.meshes.new(f"UCX_{B['id']}_{ci:02d}")
        cb.to_mesh(cm); cb.free()
        co = bpy.data.objects.new(f"UCX_{B['id']}_{ci:02d}", cm)
        co.display_type = "WIRE"; co.hide_render = True
        co.location = WORLD
        coll_c.objects.link(co)
    if bi % 100 == 0:
        print(f"  {bi}/{len(_blds)} buildings, {total_faces} faces", flush=True)

# ------------------------------------------------------------------ overhead cable tangle
from mathutils.kdtree import KDTree
wr = random.Random(99)
kd = KDTree(len(ANCHORS))
for i, (p, _) in enumerate(ANCHORS):
    kd.insert(p, i)
kd.balance()
wg = G()
up = Vector((0, 0, 1))
nw = 0
for i, (p, n) in enumerate(ANCHORS):
    cands = [(q, j, dist) for q, j, dist in kd.find_range(p, 22.0) if 4.0 < dist and j > i]
    wr.shuffle(cands)
    links = 0
    for q, j, dist in cands:
        if links >= wr.choice([1, 2, 2, 3]):
            break
        dvec = (q - p); dvec.z = 0
        if dvec.length < 1e-3:
            continue
        dn = dvec.normalized()
        if n.dot(dn) < 0.4 or ANCHORS[j][1].dot(-dn) < 0.4:  # walls must face each other
            continue
        sag = 0.05 * dist + wr.uniform(0.1, 0.5)
        pts = [p.lerp(q, s) - up * sag * 4 * s * (1 - s) for s in [k / 10 for k in range(11)]]
        r = wr.choice([0.008, 0.012, 0.012, 0.02])
        for a_, b_ in zip(pts, pts[1:]):
            ax = (b_ - a_); Ls = ax.length; ax.normalize()
            side = ax.cross(up).normalized(); vert = side.cross(ax).normalized()
            wg.box((a_ + b_) / 2, ax, side, vert, Ls / 2 + r, r, r, 0, caps=False)
        links += 1; nw += 1
if nw:
    wm = bpy.data.meshes.new("Overhead_Wires"); wg.bm.to_mesh(wm); wg.bm.free()
    wm.materials.append(MAT[11])
    wob = bpy.data.objects.new("Overhead_Wires", wm); coll_g.objects.link(wob)
print(f"  {nw} overhead cables from {len(ANCHORS)} anchors")

# ------------------------------------------------------------------ ground + roads
half = data["size"] / 2 + 40
if TERRAIN:
    # draped terrain preview patch for Blender only (in Unreal the Landscape is the ground)
    gbm = bmesh.new()
    steps = int(2 * half / 4)
    grid = [[gbm.verts.new((OX - half + i * 4, OY - half + j * 4, ground(OX - half + i * 4, OY - half + j * 4)))
             for i in range(steps + 1)] for j in range(steps + 1)]
    for j in range(steps):
        for i in range(steps):
            gbm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    box_uv(gbm, 3.0)
    gm = bpy.data.meshes.new("TerrainPreview"); gbm.to_mesh(gm); gbm.free(); gm.materials.append(M_GROUND)
    gob = bpy.data.objects.new("TerrainPreview", gm); coll_g.objects.link(gob)
else:
    gbm = bmesh.new()
    gv = [gbm.verts.new(p) for p in ((-half, -half, 0), (half, -half, 0), (half, half, 0), (-half, half, 0))]
    gbm.faces.new(gv); box_uv(gbm, 3.0)
    gm = bpy.data.meshes.new("Ground"); gbm.to_mesh(gm); gbm.free(); gm.materials.append(M_GROUND)
    gob = bpy.data.objects.new("Ground", gm); coll_g.objects.link(gob)
    # ground collision box
    cb = bmesh.new(); bmesh.ops.create_cube(cb, size=1.0)
    for v in cb.verts:
        v.co.x *= 2 * half; v.co.y *= 2 * half; v.co.z = v.co.z * 1.0 - 0.5
    cm = bpy.data.meshes.new("UCX_Ground_00"); cb.to_mesh(cm); cb.free()
    co = bpy.data.objects.new("UCX_Ground_00", cm); co.display_type = "WIRE"; co.hide_render = True
    coll_c.objects.link(co)

ROAD_EXTRA = []   # (name, kind, location_m, ucx_count) for station pieces / viaducts exported later
RAIL = data.get("rail")
PLATFORM_POLYS = [[tuple(p) for p in pl["poly"]] for pl in RAIL["platforms"]] if RAIL else []


def _in_platform(x, y):
    return any(point_in_poly(x, y, pp) for pp in PLATFORM_POLYS)


def new_obj(name, g, mats, loc, coll=None):
    """bmesh G -> object with only the used materials; pivot at loc."""
    bm = g.bm
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.002)
    box_uv(bm)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    used = sorted({p.material_index for p in me.polygons})
    remap = {o: i for i, o in enumerate(used)}
    for poly in me.polygons:
        poly.material_index = remap[poly.material_index]
    for o in used:
        me.materials.append(mats[o])
    ob = bpy.data.objects.new(name, me)
    ob.location = loc
    (coll or coll_g).objects.link(ob)
    return ob


def ucx_hull(name, pts3, loc):
    """Convex collision piece (UCX_<mesh>_NN) from points relative to the pivot."""
    cb = bmesh.new()
    vs = [cb.verts.new(p) for p in pts3]
    res = bmesh.ops.convex_hull(cb, input=vs)
    interior = [e for e in res.get("geom_interior", []) if isinstance(e, bmesh.types.BMVert)]
    if interior:
        bmesh.ops.delete(cb, geom=interior, context="VERTS")
    bmesh.ops.recalc_face_normals(cb, faces=cb.faces)
    cm = bpy.data.meshes.new(name); cb.to_mesh(cm); cb.free()
    co = bpy.data.objects.new(name, cm)
    co.display_type = "WIRE"; co.hide_render = True
    co.location = loc
    coll_c.objects.link(co)


def obox_pts(c, x, y, z, hx, hy, hz):
    return [c + x * a * hx + y * b * hy + z * d * hz for a in (-1, 1) for b in (-1, 1) for d in (-1, 1)]


def obox(g, c, x, y, z, hx, hy, hz, mat, caps=True):
    """G.box wants a left-handed (x, y, z) for outward faces; fix the handedness here."""
    if x.cross(y).dot(z) > 0:
        y = -y
    g.box(c, x, y, z, hx, hy, hz, mat, caps)


def seg_frame(p, q):
    ax = (q - p)
    L = ax.length
    ax = ax / L
    side = Vector((-ax.y, ax.x, 0))
    if side.length < 1e-6:
        side = Vector((1, 0, 0))
    side.normalize()
    up = ax.cross(side)
    if up.z < 0:
        up = -up
    return ax, side, up.normalized(), L


def prism(g, ring, z0, z1, mat_top, mat_side):
    """Extruded CCW polygon (relative coords)."""
    bot = [g.bm.verts.new((x, y, z0)) for x, y in ring]
    top = [g.bm.verts.new((x, y, z1)) for x, y in ring]
    try:
        f = g.bm.faces.new(top); f.material_index = mat_top
    except ValueError:
        pass
    n = len(ring)
    for i in range(n):
        j = (i + 1) % n
        try:
            f = g.bm.faces.new((bot[i], bot[j], top[j], top[i])); f.material_index = mat_side
        except ValueError:
            pass


def mean_ground(pts):
    return sum(ground(x + OX, y + OY) for x, y in pts) / len(pts)


# ------------------------------------------------------------------ road network (+ elevated viaducts)
RW = {"trunk": 14, "motorway": 14, "primary": 10, "secondary": 8, "tertiary": 7, "residential": 5,
      "unclassified": 5, "service": 3.5, "living_street": 4, "footway": 1.8, "path": 1.5, "pedestrian": 4,
      "cycleway": 2.5, "steps": 0}
rbm = bmesh.new()
vg = G()
via_n = 0
via_ucx = []
for r in data["roads"]:
    w = RW.get(r["type"].replace("_link", ""), 0)
    if not w or r.get("tunnel"):
        continue
    if r.get("bridge") and r["type"].replace("_link", "") in ("trunk", "motorway", "primary", "secondary", "tertiary"):
        wp = [Vector((x + OX, y + OY, 0)) for x, y in r["pts"]]
        cum = [0.0]
        for a_, b_ in zip(wp, wp[1:]):
            cum.append(cum[-1] + (b_ - a_).length)
        T = cum[-1]
        if T >= 45:
            Hd = 5.5 if r.get("layer", 1) <= 1 else 7.5
            ramp = max(10.0, min(60.0, T / 2))
            zs = [ground(p.x, p.y) + Hd * (lambda t: t * t * (3 - 2 * t))(min(1.0, min(s, T - s) / ramp))
                  for p, s in zip(wp, cum)]
            pts3 = [Vector((p.x, p.y, z)) for p, z in zip(wp, zs)]
            for a_, b_ in zip(pts3, pts3[1:]):
                if (b_ - a_).length < 0.5:
                    continue
                ax, sd, upv, L = seg_frame(a_, b_)
                c = (a_ + b_) / 2 - upv * 0.45
                obox(vg, c, ax, sd, upv, L / 2 + 0.1, w / 2, 0.45, 12)
                ucx = obox_pts(c, ax, sd, upv, L / 2 + 0.1, w / 2, 0.45)
                via_ucx.append(ucx)
                for sgn in (-1, 1):   # jersey barriers
                    obox(vg, (a_ + b_) / 2 + sd * sgn * (w / 2 - 0.2) + upv * 0.4, ax, sd, upv, L / 2, 0.2, 0.4, 1)
                    via_ucx.append(obox_pts((a_ + b_) / 2 + sd * sgn * (w / 2 - 0.2) + upv * 0.4, ax, sd, upv, L / 2, 0.2, 0.4))
            nxt = 0.0
            for p, s, z in zip(pts3, cum, zs):
                if s >= nxt and z - ground(p.x, p.y) > 2.5:
                    gz = ground(p.x, p.y)
                    c = Vector((p.x, p.y, (gz + z - 0.9) / 2))
                    obox(vg, c, Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)), 0.8, 0.8, (z - 0.9 - gz) / 2, 1)
                    via_ucx.append(obox_pts(c, Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)), 0.8, 0.8, (z - 0.9 - gz) / 2))
                    nxt = s + 25.0
            via_n += 1
            continue
    pts = [Vector((x + OX, y + OY, ground(x + OX, y + OY) + 0.08)) for x, y in r["pts"]]
    for p, q in zip(pts, pts[1:]):
        d = q - p
        if d.length < 0.05:
            continue
        if PLATFORM_POLYS and _in_platform((p.x + q.x) / 2, (p.y + q.y) / 2):
            continue
        nn = Vector((-d.y, d.x, 0)).normalized() * (w / 2)
        vs = [rbm.verts.new(v) for v in (p - nn, q - nn, q + nn, p + nn)]
        rbm.faces.new(vs)
        cap = [rbm.verts.new(q + Vector((math.cos(a) * w / 2, math.sin(a) * w / 2, 0)))
               for a in [k * math.pi / 4 for k in range(8)]]
        rbm.faces.new(cap)
box_uv(rbm, 4.0)
rm = bpy.data.meshes.new("Roads"); rbm.to_mesh(rm); rbm.free(); rm.materials.append(M_ASPHALT)
rob = bpy.data.objects.new("Roads", rm); coll_g.objects.link(rob)

STATION = []   # exported station / infrastructure pieces
if via_n:
    vob = new_obj("Viaducts", vg, MAT, (0, 0, 0))
    for i, pts in enumerate(via_ucx):
        ucx_hull(f"UCX_Viaducts_{i:03d}", pts, (0, 0, 0))
    STATION.append({"id": "Viaducts", "kind": "viaduct", "location_m": [0, 0, 0], "ucx": len(via_ucx)})
    print(f"  {via_n} elevated road decks, {len(via_ucx)} collision pieces")
else:
    vg.bm.free()

# ------------------------------------------------------------------ railway station set (platforms, canopies, tracks, FOBs)
if RAIL:
    UPX, UPY, UPZ = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
    PLAT_H = 0.84            # Indian Railways high-level platform: 840 mm above rail level
    plat_top_z = {}
    for pl in RAIL["platforms"]:
        ring = [tuple(p) for p in pl["poly"]]
        cx = sum(p[0] for p in ring) / len(ring); cy = sum(p[1] for p in ring) / len(ring)
        zg = mean_ground(ring)
        zmin = min(ground(x + OX, y + OY) for x, y in ring)
        top = zg + PLAT_H - zmin            # relative to the pivot (pivot z = zmin)
        loc = Vector((cx + OX, cy + OY, zmin))
        rel = [(x - cx, y - cy) for x, y in ring]
        g = G()
        prism(g, rel, -0.6, top, MI_PLATFORM, 1)
        # yellow safety strip 0.6 m in from every track-side edge
        for j, ts in enumerate(pl["track_edges"]):
            if not ts:
                continue
            a = Vector(rel[j]); b = Vector(rel[(j + 1) % len(rel)])
            d = b - a
            if d.length < 0.5:
                continue
            t_ = d.normalized(); inn = Vector((-t_.y, t_.x))      # inward for a CCW ring
            q0, q1 = a + inn * 0.55, a + inn * 0.65
            q2, q3 = b + inn * 0.65, b + inn * 0.55
            g.quad([Vector((q0.x, q0.y, top + 0.004)), Vector((q3.x, q3.y, top + 0.004)),
                    Vector((q2.x, q2.y, top + 0.004)), Vector((q1.x, q1.y, top + 0.004))], MI_YELLOW)
        pid = pl["id"]
        new_obj(pid, g, MAT, loc)
        for k, pc in enumerate(pl["collision"]):
            pts3 = [(x - cx, y - cy, z) for x, y in pc for z in (-0.6, top)]
            ucx_hull(f"UCX_{pid}_{k:02d}", pts3, loc)
        STATION.append({"id": pid, "kind": "platform", "name": pl["name"], "ref": pl["ref"],
                        "location_m": [round(v, 3) for v in loc], "top_above_pivot_m": round(top, 3),
                        "ucx": len(pl["collision"]), "area_m2": round(pl["area"], 1)})
        plat_top_z[pid] = zmin + top
        # ---- canopy
        cn = pl.get("canopy")
        if cn:
            g = G()
            H0 = top + cn["height_m"]
            cs = [Vector((x - cx, y - cy)) for x, y in cn["corners"]]
            r0_ = (cs[0] + cs[3]) / 2; r1_ = (cs[1] + cs[2]) / 2
            rise = 0.55
            P3 = lambda v, z: Vector((v.x, v.y, z))
            # corrugated sheet roof, gabled along the platform: top skin + underside + fascia
            for (a_, b_, c_, d_) in ((cs[0], cs[1], r1_, r0_), (r0_, r1_, cs[2], cs[3])):
                za = lambda v: H0 + (rise if (v is r0_ or v is r1_) else 0.0)
                g.quad([P3(a_, za(a_)), P3(b_, za(b_)), P3(c_, za(c_)), P3(d_, za(d_))], MI_CANOPY)
                g.quad([P3(d_, za(d_) - 0.14), P3(c_, za(c_) - 0.14), P3(b_, za(b_) - 0.14), P3(a_, za(a_) - 0.14)], MI_CANOPY)
            for a_, b_ in ((cs[0], cs[1]), (cs[2], cs[3])):   # long edge fascia
                dv = (b_ - a_); L_ = dv.length
                obox(g, P3((a_ + b_) / 2, H0 - 0.07), Vector((dv.x / L_, dv.y / L_, 0)), Vector((-dv.y / L_, dv.x / L_, 0)),
                     UPZ, L_ / 2, 0.05, 0.1, MI_RAIL)
            # steel columns + two longitudinal girders
            for (px_, py_) in cn["columns"]:
                p = Vector((px_ - cx, py_ - cy, 0))
                obox(g, Vector((p.x, p.y, (top + H0 - 0.14) / 2 + 0.0)), UPX, UPY, UPZ, 0.16, 0.16, (H0 - 0.14 - top) / 2, MI_RAIL)
                ucx_hull(f"UCX_{pid}_Canopy_{len(STATION):03d}_{int(px_ * 7) % 997:03d}",
                         [(p.x + dx, p.y + dy, z) for dx in (-0.16, 0.16) for dy in (-0.16, 0.16) for z in (-0.6, H0 - 0.14)], loc)
            cname = "Canopy_" + pid.split("_")[-1]
            cob = new_obj(cname, g, MAT, loc)
            # rename the collision pieces created above so they belong to this mesh: UCX_<mesh>_NN
            cols = [o for o in coll_c.objects if o.name.startswith(f"UCX_{pid}_Canopy_")]
            for k, o in enumerate(cols):
                o.name = f"UCX_{cname}_{k:02d}"; o.data.name = o.name
            STATION.append({"id": cname, "kind": "canopy", "location_m": [round(v, 3) for v in loc],
                            "ucx": len(cols), "height_above_platform_m": cn["height_m"]})

    # ---- tracks: ballast bed, two rails (1676 mm broad gauge), concrete sleepers every 0.65 m
    lim = data["size"] / 2 + 60
    ntrk = 0
    for tr in RAIL["tracks"]:
        pts = [Vector((x + OX, y + OY, 0)) for x, y in tr["pts"]]
        keep = [(a_, b_) for a_, b_ in zip(pts, pts[1:])
                if (abs(a_.x) < lim and abs(a_.y) < lim) or (abs(b_.x) < lim and abs(b_.y) < lim)]
        if not keep:
            continue
        p0 = keep[0][0]
        loc = Vector((p0.x, p0.y, ground(p0.x, p0.y)))
        g = G()
        ucx = []
        gauge = tr["gauge_m"]
        for a_, b_ in keep:
            za, zb = ground(a_.x, a_.y) - loc.z, ground(b_.x, b_.y) - loc.z
            A = Vector((a_.x - loc.x, a_.y - loc.y, za)); B = Vector((b_.x - loc.x, b_.y - loc.y, zb))
            if (B - A).length < 0.3:
                continue
            ax, sd, upv, L = seg_frame(A, B)
            mid = (A + B) / 2
            obox(g, mid + upv * 0.02, ax, sd, upv, L / 2 + 0.15, 1.9, 0.18, MI_BALLAST)
            ucx.append(obox_pts(mid + upv * 0.02, ax, sd, upv, L / 2, 1.9, 0.18))
            for s in (-1, 1):
                obox(g, mid + sd * s * gauge / 2 + upv * 0.31, ax, sd, upv, L / 2 + 0.1, 0.035, 0.085, MI_RAIL)
            ns = int(L / 0.65)
            for k in range(ns):
                c = A + ax * ((k + 0.5) * L / ns) + upv * 0.27
                obox(g, c, ax, sd, upv, 0.13, 1.3, 0.07, MI_SLEEPER, caps=False)
        if not ucx:
            g.bm.free(); continue
        tid = tr["id"]
        new_obj(tid, g, MAT, loc)
        for k, pts3 in enumerate(ucx):
            ucx_hull(f"UCX_{tid}_{k:02d}", pts3, loc)
        STATION.append({"id": tid, "kind": "track", "location_m": [round(v, 3) for v in loc], "ucx": len(ucx)})
        ntrk += 1

    # ---- foot overbridges (FOB 1 uncovered, FOB 2 covered ...) with columns clear of the tracks, + stair flights
    FOB_H = 6.8              # deck above ground at the platforms
    fob_levels = []
    for fb in RAIL["footbridges"]:
        pts = [(x + OX, y + OY) for x, y in fb["pts"]]
        zg = mean_ground(fb["pts"])
        ztop = zg + FOB_H
        fob_levels.append(ztop)
        W_ = fb["width"]
        c0 = Vector((sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts), zg))
        g = G(); ucx = []
        P0 = [Vector((x - c0.x, y - c0.y, ztop - zg)) for x, y in pts]
        for a_, b_ in zip(P0, P0[1:]):
            if (b_ - a_).length < 0.3:
                continue
            ax, sd, upv, L = seg_frame(a_, b_)
            mid = (a_ + b_) / 2
            obox(g, mid - upv * 0.17, ax, sd, upv, L / 2 + W_ * 0.3, W_ / 2, 0.17, MI_PLATFORM)
            ucx.append(obox_pts(mid - upv * 0.17, ax, sd, upv, L / 2, W_ / 2, 0.17))
            for s in (-1, 1):   # railing: steel mesh panel + handrail
                cc = mid + sd * s * (W_ / 2 - 0.05) + upv * 0.55
                obox(g, cc, ax, sd, upv, L / 2, 0.025, 0.5, MI_RAIL)
                obox(g, mid + sd * s * (W_ / 2 - 0.05) + upv * 1.08, ax, sd, upv, L / 2, 0.05, 0.03, MI_RAIL)
                ucx.append(obox_pts(cc + upv * 0.1, ax, sd, upv, L / 2, 0.05, 0.6))
            if fb["covered"]:
                # roof sheet on posts every ~4 m
                k = max(1, int(L / 4))
                obox(g, mid + upv * 3.1, ax, sd, upv, L / 2 + 0.3, W_ / 2 + 0.4, 0.04, MI_CANOPY)
                for i in range(k + 1):
                    for s in (-1, 1):
                        cc = a_ + ax * (L * i / k) + sd * s * (W_ / 2 - 0.05) + upv * 1.55
                        obox(g, cc, UPX, UPY, UPZ, 0.05, 0.05, 1.55, MI_RAIL)
        for (x, y) in fb["columns"]:
            gz = ground(x + OX, y + OY)
            c = Vector((x + OX - c0.x, y + OY - c0.y, 0)); c.z = (gz - zg + (ztop - zg) - 0.34) / 2
            hh = ((ztop - zg) - 0.34 - (gz - zg)) / 2
            if hh > 0.5:
                obox(g, c, UPX, UPY, UPZ, 0.28, 0.28, hh, MI_PLATFORM)
                ucx.append(obox_pts(c, UPX, UPY, UPZ, 0.28, 0.28, hh))
        fid = fb["id"]
        new_obj(fid, g, MAT, c0)
        for k, pts3 in enumerate(ucx):
            ucx_hull(f"UCX_{fid}_{k:02d}", pts3, c0)
        STATION.append({"id": fid, "kind": "footbridge", "name": fb["name"], "covered": fb["covered"],
                        "location_m": [round(v, 3) for v in c0], "deck_z_m": round(ztop, 3), "ucx": len(ucx)})
    deck_z = (sum(fob_levels) / len(fob_levels)) if fob_levels else None
    for si, st in enumerate(RAIL["steps"]):
        pts = [(x + OX, y + OY) for x, y in st["pts"]]
        if st["top_first"]:
            top_p, bot_p = pts[0], pts[-1]
        else:
            top_p, bot_p = pts[-1], pts[0]
        # nearest footbridge deck level at the top end
        zt = deck_z if deck_z is not None else ground(*top_p) + FOB_H
        gb = ground(*bot_p)
        zb_ = gb + (PLAT_H if _in_platform(bot_p[0] - OX, bot_p[1] - OY) else 0.0)
        top_v = Vector((top_p[0], top_p[1], zt)); bot_v = Vector((bot_p[0], bot_p[1], zb_))
        run = Vector((bot_v.x - top_v.x, bot_v.y - top_v.y, 0)); Lh = run.length
        if Lh < 1.0:
            continue
        ns = max(4, int(round((zt - zb_) / 0.17)))
        rise = (zt - zb_) / ns; tread = Lh / ns
        dirv = run / Lh; sd = Vector((-dirv.y, dirv.x, 0)); Wd = st["width"]
        loc = Vector((top_v.x, top_v.y, zb_))
        g = G()
        for k in range(ns):
            zc_top = zt - (k + 1) * rise + rise          # top surface of step k (top end first)
            c = dirv * (tread * (k + 0.5)) + Vector((0, 0, (zc_top - zb_ - 0.0) / 2))
            # solid riser stack from the floor to the tread
            hz = max(0.02, (zc_top - zb_) / 2)
            c.z = zb_ - loc.z + hz
            obox(g, c, dirv, sd, UPZ, tread / 2, Wd / 2, hz, MI_PLATFORM, caps=True)
        for s in (-1, 1):   # handrails following the slope
            a_ = Vector((0, 0, zt - loc.z + 1.0)) + sd * s * (Wd / 2 - 0.04)
            b_ = dirv * Lh + Vector((0, 0, zb_ - loc.z + 1.0)) + sd * s * (Wd / 2 - 0.04)
            ax, sd2, upv, L = seg_frame(a_, b_)
            obox(g, (a_ + b_) / 2, ax, sd2, upv, L / 2, 0.03, 0.03, MI_RAIL)
        sid = f"Stairs_{si:02d}"
        new_obj(sid, g, MAT, loc)
        wedge = [(dirv.x * 0 + sd.x * s * Wd / 2, sd.y * s * Wd / 2, zt - loc.z) for s in (-1, 1)] + \
                [(dirv.x * Lh + sd.x * s * Wd / 2, dirv.y * Lh + sd.y * s * Wd / 2, 0.0) for s in (-1, 1)] + \
                [(sd.x * s * Wd / 2, sd.y * s * Wd / 2, 0.0) for s in (-1, 1)]
        ucx_hull(f"UCX_{sid}_00", wedge, loc)
        # side rails collision (keep walkers on the flight)
        STATION.append({"id": sid, "kind": "stairs", "location_m": [round(v, 3) for v in loc], "ucx": 1,
                        "rise_m": round(zt - zb_, 2), "steps": ns})
    # ---- signal posts
    if RAIL["signals"]:
        g = G(); ucx = []
        for (x, y) in RAIL["signals"]:
            gz = ground(x + OX, y + OY); c = Vector((x + OX, y + OY, gz))
            obox(g, c + Vector((0, 0, 2.6)), UPX, UPY, UPZ, 0.07, 0.07, 2.6, MI_RAIL)
            obox(g, c + Vector((0, 0, 5.0)), UPX, UPY, UPZ, 0.22, 0.14, 0.45, MI_RAIL)
            ucx.append(obox_pts(c + Vector((0, 0, 2.6)), UPX, UPY, UPZ, 0.15, 0.15, 2.6))
        new_obj("Station_Signals", g, MAT, (0, 0, 0))
        for k, pts3 in enumerate(ucx):
            ucx_hull(f"UCX_Station_Signals_{k:02d}", pts3, (0, 0, 0))
        STATION.append({"id": "Station_Signals", "kind": "signals", "location_m": [0, 0, 0], "ucx": len(ucx)})
    print(f"  station set: {len(RAIL['platforms'])} platforms, {ntrk} track pieces, {len(RAIL['footbridges'])} footbridges, "
          f"{len(RAIL['steps'])} stair flights")

# ------------------------------------------------------------------ lighting / camera
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 5.0; sun.data.angle = math.radians(1.5)
sun.rotation_euler = (math.radians(40), math.radians(10), math.radians(140))
scene.collection.objects.link(sun)
world = bpy.data.worlds.new("Sky"); scene.world = world; world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.6, 0.72, 0.9, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.8
cam = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
cam.data.clip_start = 0.1; cam.data.clip_end = 3000
scene.collection.objects.link(cam); scene.camera = cam

PP["work"].mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=OUT + ".blend")


# ------------------------------------------------------------------ per-object FBX + manifest (Unreal)
def _ucx_for(mesh_name):
    pre = f"UCX_{mesh_name}_"
    return [o for o in coll_c.objects if o.name.startswith(pre) and o.name[len(pre):].isdigit()]


def _export(ob, ucx, path, relative_pivot=True):
    bpy.ops.object.select_all(action="DESELECT")
    group = [ob] + ucx
    saved = [o.location.copy() for o in group]
    for o in group:
        if relative_pivot:
            o.location = (0, 0, 0)          # pivot = object origin (footprint centroid at grade)
        o.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                             mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z",
                             path_mode="RELATIVE", embed_textures=False, colors_type="LINEAR")
    for o, l in zip(group, saved):
        o.location = l


if EXPORT_DIR:
    (EXPORT_DIR / "buildings").mkdir(parents=True, exist_ok=True)
    (EXPORT_DIR / "station").mkdir(parents=True, exist_ok=True)
    ucx_by_name = {}
    for o in coll_c.objects:
        if o.name.startswith("UCX_"):
            base = o.name[4:].rsplit("_", 1)[0]
            ucx_by_name.setdefault(base, []).append(o)
    for entry in MANIFEST:
        ob = bpy.data.objects[entry["id"]]
        _export(ob, ucx_by_name.get(entry["id"], []), EXPORT_DIR / "buildings" / f"{entry['id']}.fbx")
        entry["fbx"] = f"buildings/{entry['id']}.fbx"
    for st in STATION:
        ob = bpy.data.objects[st["id"]]
        _export(ob, ucx_by_name.get(st["id"], []), EXPORT_DIR / "station" / f"{st['id']}.fbx")
        st["fbx"] = f"station/{st['id']}.fbx"
        st["surface_types"] = {}
    for name in ("Roads", "Overhead_Wires"):
        o = bpy.data.objects.get(name)
        if o is None:
            continue
        _export(o, [], EXPORT_DIR / f"{name}.fbx", relative_pivot=False)
    tman = json.load(open(PP["tiles"] / "terrain_manifest.json")) if (PP["tiles"] / "terrain_manifest.json").exists() else None
    pois = json.load(open(PP["work"] / "pois.json"))["pois"] if (PP["work"] / "pois.json").exists() else []
    man = {"location": _a.location, "title": data["title"], "map_name": LC.LOCATIONS[_a.location]["map_name"],
           "slice_center_latlon": data["center"], "size_m": data["size"], "style": STYLE,
           "world_origin": "slice centre = terrain centre", "terrain": TMETA if TERRAIN else None,
           "terrain_tiles": [dict(t, fbx=f"terrain/{t['fbx']}") for t in tman["tiles"]] if tman else [],
           "landscape_json": str(TERRAIN / "landscape.json"),
           "units": "metres, Z up, X east, Y north (Unreal: X east, Y = -north, cm)",
           "materials": [m.name for m in MAT] + ["M_Asphalt"],
           "surface_types": {"M_PaintedPlaster": "Plaster", "M_DamagedPlaster": "Plaster", "M_Brick": "Brick",
                             "M_Concrete": "Concrete", "M_RoofConcrete": "Concrete", "M_Glass": "Glass",
                             "M_RustyMetal": "Metal", "M_RollingShutter": "Metal", "M_ACUnit": "Metal",
                             "M_PVCTank": "Plastic", "M_Signboard": "Plastic", "M_Cable": "Metal",
                             "M_Asphalt": "Asphalt", "M_PlatformSlab": "Concrete", "M_Ballast": "Gravel",
                             "M_Rail": "Metal", "M_CanopyMetal": "Metal", "M_SafetyLine": "Paint",
                             "M_Sleeper": "Concrete"},
           "buildings": MANIFEST, "station": STATION, "pois": pois,
           "stats": {"building_faces": total_faces, "buildings": len(MANIFEST)}}
    json.dump(man, open(EXPORT_DIR / "manifest.json", "w"), indent=1)
    print(f"EXPORTED {len(MANIFEST)} building FBX + {len(STATION)} station pieces -> {EXPORT_DIR}", flush=True)
print(f"DONE {len(_blds)} buildings, {total_faces} building faces -> {OUT}.blend")
