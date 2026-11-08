"""Build a real-scale blockout of a Mumbai district from an OpenStreetMap .osm (XML) file.

Run with Blender (no add-ons needed):
  blender --background --python osm_to_blender.py -- <input.osm> <output_basename>

Creates <output>.blend and <output>.fbx (1 unit = 1 m, Z up, origin at the bbox centre):
  - Buildings: real footprints extruded to OSM `height`, else `building:levels` x 3.2 m,
    else a deterministic 3-7 storey guess (typical South Mumbai chawls / Art Deco blocks).
  - Roads: flat strips sized by highway class. Rail lines. Parks/grass. Water/sea.
Data (c) OpenStreetMap contributors, ODbL 1.0 - credit "© OpenStreetMap contributors" in-game.
"""
import math
import random
import sys
import xml.etree.ElementTree as ET

import bmesh
import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
# optional: minlat minlon maxlat maxlon (clip; Overpass recursion pulls far-away relation members)
BBOX = tuple(map(float, argv[2:6])) if len(argv) >= 6 else None

ROAD_W = {"motorway": 16, "trunk": 14, "primary": 12, "secondary": 10, "tertiary": 8,
          "residential": 6, "unclassified": 6, "service": 4, "living_street": 5,
          "pedestrian": 6, "footway": 2, "path": 1.5, "cycleway": 2, "steps": 2}
GREEN = {"park", "garden", "pitch", "playground", "recreation_ground"}
GREEN_LU = {"grass", "recreation_ground", "forest", "meadow", "village_green", "cemetery"}

# ------------------------------------------------------------------ parse
root = ET.parse(SRC).getroot()
b = root.find("bounds")
nodes = {n.get("id"): (float(n.get("lat")), float(n.get("lon"))) for n in root.iter("node")}
if BBOX:
    lat0, lon0 = (BBOX[0] + BBOX[2]) / 2, (BBOX[1] + BBOX[3]) / 2
elif b is not None:
    lat0 = (float(b.get("minlat")) + float(b.get("maxlat"))) / 2
    lon0 = (float(b.get("minlon")) + float(b.get("maxlon"))) / 2
else:
    lats = [p[0] for p in nodes.values()]; lons = [p[1] for p in nodes.values()]
    lat0, lon0 = (min(lats) + max(lats)) / 2, (min(lons) + max(lons)) / 2
R = 6378137.0
kx = math.radians(1) * R * math.cos(math.radians(lat0))
ky = math.radians(1) * R


def xy(nid):
    la, lo = nodes[nid]
    return ((lo - lon0) * kx, (la - lat0) * ky)


def inside(nid):
    if not BBOX:
        return True
    la, lo = nodes[nid]
    return BBOX[0] <= la <= BBOX[2] and BBOX[1] <= lo <= BBOX[3]


ways = {}
for w in root.iter("way"):
    refs = [nd.get("ref") for nd in w.iter("nd") if nd.get("ref") in nodes]
    if not any(inside(r) for r in refs):
        continue
    tags = {t.get("k"): t.get("v") for t in w.iter("tag")}
    ways[w.get("id")] = (refs, tags)

# multipolygon relations (outer rings only - enough for big buildings & parks)
rel_polys = []
for r in root.iter("relation"):
    tags = {t.get("k"): t.get("v") for t in r.iter("tag")}
    if tags.get("type") != "multipolygon":
        continue
    for m in r.iter("member"):
        if m.get("type") == "way" and m.get("role") == "outer" and m.get("ref") in ways:
            refs, wt = ways[m.get("ref")]
            if len(refs) > 3 and refs[0] == refs[-1]:
                rel_polys.append((refs, {**wt, **tags}, r.get("id")))

# ------------------------------------------------------------------ helpers
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"


def material(name, rgb, rough=0.8):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)
    bsdf.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)  # Solid-view / Workbench colour
    return m


MATS = {
    "building": material("M_Building_Plaster", (0.62, 0.58, 0.52)),
    "heritage": material("M_Building_Heritage_Stone", (0.70, 0.62, 0.48)),
    "road": material("M_Road_Asphalt", (0.08, 0.08, 0.085), 0.9),
    "foot": material("M_Pavement", (0.45, 0.44, 0.42)),
    "rail": material("M_Rail_Ballast", (0.25, 0.22, 0.2)),
    "green": material("M_Grass", (0.12, 0.25, 0.08)),
    "water": material("M_Sea", (0.05, 0.12, 0.15), 0.1),
    "ground": material("M_Ground", (0.30, 0.28, 0.25)),
}


class Builder:
    def __init__(self, name, mat):
        self.bm = bmesh.new(); self.name = name; self.mat = mat

    def finish(self, coll):
        if not self.bm.verts:
            return None
        me = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(me); self.bm.free()
        me.materials.append(self.mat)
        ob = bpy.data.objects.new(self.name, me)
        coll.objects.link(ob)
        return ob


def ring(refs):
    pts = [xy(r) for r in refs]
    if pts[0] == pts[-1]:
        pts = pts[:-1]
    # drop consecutive duplicates
    out = [p for i, p in enumerate(pts) if i == 0 or (abs(p[0] - pts[i - 1][0]) + abs(p[1] - pts[i - 1][1])) > 0.01]
    area = sum(out[i][0] * out[(i + 1) % len(out)][1] - out[(i + 1) % len(out)][0] * out[i][1]
               for i in range(len(out))) / 2
    if area < 0:
        out.reverse()
    return out, abs(area)


def extrude(bm, pts, z0, z1):
    bot = [bm.verts.new((x, y, z0)) for x, y in pts]
    top = [bm.verts.new((x, y, z1)) for x, y in pts]
    try:
        bm.faces.new(top)
        bm.faces.new(list(reversed(bot)))
    except ValueError:
        pass
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        try:
            bm.faces.new((bot[i], bot[j], top[j], top[i]))
        except ValueError:
            pass


def flat(bm, pts, z):
    vs = [bm.verts.new((x, y, z)) for x, y in pts]
    try:
        bm.faces.new(vs)
    except ValueError:
        pass


def strip(bm, pts, width, z):
    h = width / 2
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        dx, dy = x1 - x0, y1 - y0
        L = math.hypot(dx, dy)
        if L < 0.05:
            continue
        nx, ny = -dy / L * h, dx / L * h
        vs = [bm.verts.new(v) for v in ((x0 + nx, y0 + ny, z), (x0 - nx, y0 - ny, z),
                                         (x1 - nx, y1 - ny, z), (x1 + nx, y1 + ny, z))]
        bm.faces.new((vs[1], vs[2], vs[3], vs[0]))
        # round joint cap so segments overlap at bends
        c = [bm.verts.new((x1 + h * math.cos(a), y1 + h * math.sin(a), z))
             for a in [k * math.pi / 4 for k in range(8)]]
        bm.faces.new(c)


def height_of(tags, wid):
    for k in ("height", "building:height"):
        if k in tags:
            try:
                return float(tags[k].replace("m", "").strip())
            except ValueError:
                pass
    if "building:levels" in tags:
        try:
            return float(tags["building:levels"]) * 3.2 + 1.0
        except ValueError:
            pass
    rnd = random.Random(int(wid) if str(wid).isdigit() else hash(wid))
    return rnd.randint(3, 7) * 3.2 + 1.0


# ------------------------------------------------------------------ build
coll = {n: bpy.data.collections.new(n) for n in ("Buildings", "Roads", "Landuse", "Water")}
for c in coll.values():
    scene.collection.children.link(c)

bld = Builder("Buildings", MATS["building"])
heritage = Builder("Buildings_Heritage", MATS["heritage"])
roads = Builder("Roads", MATS["road"]); foot = Builder("Footways", MATS["foot"])
rail = Builder("Rail", MATS["rail"]); green = Builder("Parks", MATS["green"])
water = Builder("Water", MATS["water"])

count = 0
polys = [(refs, tags, wid) for wid, (refs, tags) in ways.items()] + rel_polys
for refs, tags, wid in polys:
    closed = len(refs) > 3 and refs[0] == refs[-1]
    if "building" in tags and closed:
        pts, area = ring(refs)
        if len(pts) < 3 or area < 4:
            continue
        hgt = height_of(tags, wid)
        is_her = any(k in tags for k in ("heritage", "historic")) or tags.get("building") in (
            "train_station", "cathedral", "church", "temple", "mosque", "government", "museum")
        extrude((heritage if is_her else bld).bm, pts, 0.0, hgt)
        count += 1
    elif closed and (tags.get("leisure") in GREEN or tags.get("landuse") in GREEN_LU):
        pts, _ = ring(refs)
        if len(pts) >= 3:
            flat(green.bm, pts, 0.05)
    elif closed and (tags.get("natural") == "water" or tags.get("landuse") in ("basin", "reservoir")):
        pts, _ = ring(refs)
        if len(pts) >= 3:
            flat(water.bm, pts, -0.3)
    elif "highway" in tags and not closed:
        hw = tags["highway"]
        w = ROAD_W.get(hw.replace("_link", ""), 0)
        if w:
            pts = [xy(r) for r in refs]
            strip((foot if w <= 2 else roads).bm, pts, w, 0.08 if w > 2 else 0.12)
    elif tags.get("railway") in ("rail", "light_rail", "subway") and not closed:
        strip(rail.bm, [xy(r) for r in refs], 4.0, 0.06)

# ground + sea plane covering the bbox (coastline polygons are rarely closed in a crop)
if BBOX:
    gx, gy = (BBOX[3] - BBOX[1]) / 2 * kx, (BBOX[2] - BBOX[0]) / 2 * ky
else:
    xs = [xy(n)[0] for n in nodes]; ys = [xy(n)[1] for n in nodes]
    gx, gy = max(abs(min(xs)), abs(max(xs))), max(abs(min(ys)), abs(max(ys)))
gb = Builder("Ground", MATS["ground"])
flat(gb.bm, [(-gx, -gy), (gx, -gy), (gx, gy), (-gx, gy)], 0.0)
sea = Builder("Sea", MATS["water"])
flat(sea.bm, [(-gx * 3, -gy * 3), (gx * 3, -gy * 3), (gx * 3, gy * 3), (-gx * 3, gy * 3)], -0.5)

for b_, c in ((bld, "Buildings"), (heritage, "Buildings"), (roads, "Roads"), (foot, "Roads"),
              (rail, "Roads"), (green, "Landuse"), (gb, "Landuse"), (water, "Water"), (sea, "Water")):
    ob = b_.finish(coll[c])
    if ob is not None:
        me = ob.data
        bm = bmesh.new(); bm.from_mesh(me)
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.001)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(me); bm.free()

# sun + sky so the .blend looks right when opened
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 4; sun.rotation_euler = (math.radians(50), 0, math.radians(30))
scene.collection.objects.link(sun)
world = bpy.data.worlds.new("Sky"); scene.world = world; world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.55, 0.65, 0.8, 1)
cam = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
cam.location = (-gx * 0.9, -gy * 1.3, 900); cam.rotation_euler = (math.radians(55), 0, math.radians(-30))
cam.data.clip_start = 2; cam.data.clip_end = 20000  # avoid depth banding
scene.collection.objects.link(cam); scene.camera = cam

bpy.ops.wm.save_as_mainfile(filepath=OUT + ".blend")
bpy.ops.export_scene.fbx(filepath=OUT + ".fbx", apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                         object_types={"MESH"}, mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z")
print(f"BUILT {count} buildings; area {2 * gx:.0f} x {2 * gy:.0f} m -> {OUT}.blend / .fbx")
