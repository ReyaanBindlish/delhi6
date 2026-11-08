"""Procedurally MODEL Lyari-style buildings from real footprints (lyari_slice.json).

blender --background --python lyari_build.py -- <slice.json> <out_basename> <textures_dir>

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

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT, TEX = argv[0], argv[1], Path(argv[2])
STYLE = argv[3] if len(argv) > 3 else "karachi"
data = json.load(open(SRC))

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
]
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
    for (cu, cz, su, sz) in ((ou0 + fw, zm, fw, (oz1 - oz0) / 2), (ou1 - fw, zm, fw, (oz1 - oz0) / 2),
                             (um, oz0 + fw, (ou1 - ou0) / 2, fw), (um, oz1 - fw, (ou1 - ou0) / 2, fw),
                             (um, zm, fw * 0.7, (oz1 - oz0) / 2)):
        g.box(P(cu, cz, gd - 0.03), tv, nv, up, su, 0.03, sz, 3, caps=False)
    # concrete sunshade (chajja) over the window - very common in Karachi
    if rnd.random() < 0.7:
        g.box(P(um, oz1 + 0.06, -0.22), tv, nv, up, (ou1 - ou0) / 2 + 0.15, 0.25, 0.04, 1)
    if grill:
        # welded iron grill proud of the wall: verticals + 2 horizontals
        sp = 0.13
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
        k = int((oz1 - zb) / 0.09)
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
        k = int(2 * hw / 0.12)
        for i in range(k + 1):
            u = u0 + i * (u1 - u0) / max(1, k)
            g.box(P(u, z + 0.5, -depth + 0.02), tv, nv, up, 0.008, 0.008, 0.5, 3, caps=False)
        for s in (-1, 1):
            g.box(P(um + s * hw, z + 1.0, -depth / 2), tv, nv, up, 0.02, depth / 2, 0.02, 3)
    # laundry line / clutter skipped; maybe a chair later


def ac_unit(g, a, t, n, u, z):
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
for bi, B in enumerate(data["buildings"]):
    rnd = random.Random(B["seed"])
    fp = [Vector((x, y)) for x, y in B["footprint"]]
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
                    ANCHORS.append((Vector((a.x + t.x * u + n.x * 0.12, a.y + t.y * u + n.y * 0.12, zz)),
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
    for m in MAT:
        me.materials.append(m)
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
    ob = bpy.data.objects.new(B["id"], me)
    coll_b.objects.link(ob)
    total_faces += len(me.polygons)

    # UCX collision: one convex prism per piece (walls + roof; parapet excluded)
    for ci, piece in enumerate(B["collision"]):
        cb = bmesh.new()
        bot = [cb.verts.new((x, y, 0)) for x, y in piece]
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
        coll_c.objects.link(co)
    if bi % 100 == 0:
        print(f"  {bi}/{len(data['buildings'])} buildings, {total_faces} faces", flush=True)

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

RW = {"primary": 10, "secondary": 8, "tertiary": 7, "residential": 5, "unclassified": 5, "service": 3.5,
      "living_street": 4, "footway": 1.8, "path": 1.5, "pedestrian": 4}
rbm = bmesh.new()
for r in data["roads"]:
    w = RW.get(r["type"].replace("_link", ""), 0)
    if not w:
        continue
    pts = [Vector((x, y, 0.03)) for x, y in r["pts"]]
    for p, q in zip(pts, pts[1:]):
        d = q - p
        if d.length < 0.05:
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

bpy.ops.wm.save_as_mainfile(filepath=OUT + ".blend")
# FBX: buildings + UCX collision + ground/roads (Unreal: import with "Combine Meshes" off)
bpy.ops.export_scene.fbx(filepath=OUT + ".fbx", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                         object_types={"MESH"}, mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z",
                         path_mode="COPY", embed_textures=False, colors_type="LINEAR")
print(f"DONE {len(data['buildings'])} buildings, {total_faces} faces -> {OUT}.blend/.fbx")
