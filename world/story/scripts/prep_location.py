"""Prepare a playable location slice (adapted from world/karachi/scripts/lyari_prep.py, which is unchanged).

python prep_location.py <location> [--size M] [--center LAT LON] [--limit N]

Reads Overture footprints + OSM, writes <export>/<loc>/_work/slice.json and pois.json:
  buildings : footprint (local metres, CCW), floors, per-edge exposed/street flags, convex collision pieces,
              kind (house|station|religious|shed), name
  roads     : highway polylines (+ bridge/layer/tunnel flags)
  rail      : platforms (+ track-side edge flags, canopy plan, collision pieces), tracks, footbridges
              (+ column plan clear of the tracks), stair flights, signals          [only when preset rail=True]
Heights: Overture height / num_floors, then OSM building:levels (already merged into Overture), then a
style-specific fallback from footprint area (research 05/08: Lyari 2-4 storey, mode 3, party walls;
Nizamuddin basti 2-4, colonies 3-5, institutional/railway 1-3).
"""
import argparse
import json
import math
import random
import sys
import xml.etree.ElementTree as ET

import numpy as np
import shapely
from shapely.geometry import LineString, MultiLineString, Point, Polygon, shape
from shapely.ops import unary_union
from shapely.strtree import STRtree

sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parent))
import loc_config as C

ap = argparse.ArgumentParser()
ap.add_argument("location", choices=sorted(C.LOCATIONS))
ap.add_argument("--size", type=float)
ap.add_argument("--center", type=float, nargs=2)
ap.add_argument("--limit", type=int, default=0, help="debug: keep only N buildings nearest the centre")
args = ap.parse_args()
cfg = dict(C.LOCATIONS[args.location])
P = C.paths(args.location)
P["work"].mkdir(parents=True, exist_ok=True)
lat0, lon0 = args.center or cfg["center"]
SIZE = args.size or cfg["size"]
STYLE = cfg["style"]
R = 6378137.0
kx = math.radians(1) * R * math.cos(math.radians(lat0))
ky = math.radians(1) * R
half = SIZE / 2
margin = 40.0


def to_local(coords):
    return [((lo - lon0) * kx, (la - lat0) * ky) for lo, la in coords]


def box(m):
    return shapely.box(-half - m, -half - m, half + m, half + m)


# ---------------------------------------------------------------- Overture footprints
feats = json.load(open(cfg["geojson"], encoding="utf-8"))["features"]
polys, props = [], []
for f in feats:
    g = shape(f["geometry"])
    if g.geom_type == "MultiPolygon":
        g = max(g.geoms, key=lambda p: p.area)
    if g.geom_type != "Polygon":
        continue
    cx, cy = (g.centroid.x - lon0) * kx, (g.centroid.y - lat0) * ky
    if abs(cx) > half + margin or abs(cy) > half + margin:
        continue
    q = Polygon(to_local(g.exterior.coords))
    if not q.is_valid:
        q = q.buffer(0)
        if q.geom_type != "Polygon":
            continue
    polys.append(q)
    props.append(f.get("properties") or {})
print(f"overture footprints in slice+margin: {len(polys)}")

# ---------------------------------------------------------------- OSM
root = ET.parse(str(cfg["osm"])).getroot()
nodes = {}
node_tags = []
for n in root.iter("node"):
    nid = n.get("id")
    nodes[nid] = (float(n.get("lon")), float(n.get("lat")))
    tg = n.findall("tag")
    if tg:
        node_tags.append((nid, {t.get("k"): t.get("v") for t in tg}))

roads, tracks, platforms_raw, fob_raw, steps_raw, osm_buildings = [], [], [], [], [], []
bbox_m = box(margin)
for w in root.iter("way"):
    tags = {t.get("k"): t.get("v") for t in w.iter("tag")}
    ids = [nd.get("ref") for nd in w.iter("nd") if nd.get("ref") in nodes]
    if len(ids) < 2:
        continue
    pts = to_local([nodes[i] for i in ids])
    closed = ids[0] == ids[-1]
    ls = LineString(pts)
    if not ls.intersects(bbox_m):
        continue
    hw = tags.get("highway")
    rw = tags.get("railway")
    if rw == "platform" or tags.get("public_transport") == "platform":
        if closed and len(pts) >= 4:
            platforms_raw.append((tags, pts))
        continue
    if rw == "rail":
        tracks.append((tags, pts))
        continue
    if hw:
        if tags.get("bridge") in ("yes", "viaduct") and hw in ("footway", "pedestrian", "path", "cycleway"):
            fob_raw.append((tags, pts))
        elif hw == "steps":
            steps_raw.append((tags, pts))
        else:
            roads.append((tags, pts))
        continue
    if "building" in tags and closed and len(pts) >= 4 and tags.get("name"):
        osm_buildings.append((tags, pts))

road_lines = [LineString(p) for _, p in roads]
road_tree = STRtree(road_lines) if road_lines else None

# ---------------------------------------------------------------- rail model (platforms, canopies, FOBs)
rail = None
if cfg.get("rail"):
    track_ls = [LineString(p) for _, p in tracks]
    track_union = unary_union(track_ls) if track_ls else None
    plat = []
    for i, (tags, pts) in enumerate(platforms_raw):
        q = Polygon(pts)
        if not q.is_valid:
            q = q.buffer(0)
        if q.geom_type != "Polygon" or q.area < 30:
            continue
        q = shapely.geometry.polygon.orient(q.simplify(0.2, preserve_topology=True), 1.0)
        ring = list(q.exterior.coords)[:-1]
        edges = []
        for j in range(len(ring)):
            a, b = ring[j], ring[(j + 1) % len(ring)]
            mid = Point((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
            edges.append(bool(track_union is not None and mid.distance(track_union) < 5.0))
        hull = q.convex_hull
        if hull.area / q.area < 1.08:
            pieces = [list(hull.exterior.coords)[:-1]]
        else:
            tris = shapely.constrained_delaunay_triangles(q)
            pieces = [list(t.exterior.coords)[:-1] for t in tris.geoms if t.area > 0.2]
        # canopy plan from the minimum rotated rectangle
        canopy = None
        mrr = q.minimum_rotated_rectangle
        c4 = np.array(list(mrr.exterior.coords)[:4])
        e0, e1 = c4[1] - c4[0], c4[2] - c4[1]
        lg, sh = (e0, e1) if np.linalg.norm(e0) >= np.linalg.norm(e1) else (e1, e0)
        Lp, Wp = float(np.linalg.norm(lg)), float(np.linalg.norm(sh))
        if Lp >= 25 and Wp >= 4.0:
            u = lg / Lp
            v = np.array([-u[1], u[0]])
            cen = np.array([mrr.centroid.x, mrr.centroid.y])
            span = Lp * 0.84
            cw = min(Wp - 1.2, 7.0)
            rows = [0.0] if cw < 5.5 else [-cw * 0.32, cw * 0.32]
            ncol = max(2, int(round(span / 8.0)) + 1)
            cols = [list(map(float, cen + u * (-span / 2 + k * span / (ncol - 1)) + v * r))
                    for k in range(ncol) for r in rows]
            corners = [cen + u * s1 * span / 2 + v * s2 * cw / 2 for s1, s2 in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            canopy = {"corners": [list(map(float, c)) for c in corners], "ridge_dir": list(map(float, u)),
                      "columns": cols, "height_m": 4.6}
        plat.append({"id": f"Platform_{len(plat):02d}", "name": tags.get("name", ""), "ref": tags.get("ref", ""),
                     "poly": [list(map(float, p)) for p in ring], "track_edges": edges,
                     "collision": [[list(map(float, p)) for p in pc] for pc in pieces], "canopy": canopy,
                     "area": q.area})
    plat_union = unary_union([Polygon(p["poly"]) for p in plat]) if plat else None
    rail_area = unary_union([g for g in (track_union, plat_union) if g is not None]) if (track_union or plat_union) else None

    def near_rail(ls, d):
        return rail_area is not None and ls.distance(rail_area) <= d

    fobs = []
    for tags, pts in fob_raw:
        ls = LineString(pts)
        if not near_rail(ls, 30.0):
            continue
        covered = tags.get("covered") == "yes"
        width = 4.0 if covered else 3.0
        cols = []
        L = ls.length
        k = max(1, int(L // 16))
        for s in range(1, k + 1):
            pt = ls.interpolate(L * s / (k + 1))
            if track_union is not None and pt.distance(track_union) < 2.6:
                continue
            cols.append([pt.x, pt.y])
        fobs.append({"id": f"FOB_{len(fobs):02d}", "name": tags.get("name", ""), "pts": [list(map(float, p)) for p in pts],
                     "covered": covered, "width": width, "columns": cols})
    fob_lines = [LineString(f["pts"]) for f in fobs]
    steps = []
    for tags, pts in steps_raw:
        if tags.get("bridge") not in ("yes", "viaduct") or len(pts) < 2:
            continue
        ls = LineString(pts)
        if not fob_lines:
            continue
        d = [fl.distance(Point(pts[0])) for fl in fob_lines], [fl.distance(Point(pts[-1])) for fl in fob_lines]
        d0, d1 = min(d[0]), min(d[1])
        if min(d0, d1) > 6.0:
            continue
        top_first = d0 <= d1
        steps.append({"pts": [list(map(float, p)) for p in pts], "top_first": top_first,
                      "covered": tags.get("covered") == "yes", "width": 2.2})
    signals = []
    for nid, t in node_tags:
        if t.get("railway") == "signal":
            x, y = to_local([nodes[nid]])[0]
            if abs(x) < half and abs(y) < half:
                signals.append([x, y])
    rail = {"platforms": plat, "tracks": [{"id": f"Track_{i:03d}", "pts": [list(map(float, p)) for p in pts], "gauge_m": 1.676}
                                          for i, (_, pts) in enumerate(tracks)],
            "footbridges": fobs, "steps": steps, "signals": signals}
    print(f"rail: platforms {len(plat)} (canopies {sum(1 for p in plat if p['canopy'])}), tracks {len(tracks)}, "
          f"footbridges {len(fobs)}, stair flights {len(steps)}, signals {len(signals)}")
    exclude = unary_union([g.buffer(2.0) for g in ([plat_union] if plat_union is not None else [])] +
                          ([track_union.buffer(4.0)] if track_union is not None else []))
else:
    exclude = None

# ---------------------------------------------------------------- floors
RELIGIOUS = {"religious", "mosque", "temple", "church", "place_of_worship", "synagogue", "shrine", "cathedral", "chapel"}
LOWRISE = {"garage", "garages", "shed", "roof", "hut", "cowshed", "carport", "kiosk", "toilets", "service", "barn", "stable"}


def pick_floors(pr, area, rnd):
    nf = pr.get("num_floors")
    if nf:
        return max(1, min(int(nf), 8)), "tagged"
    h = pr.get("height")
    if h:
        return max(1, min(int(round(float(h) / 3.05)), 8)), "tagged"
    cls = (pr.get("class") or "") + " " + (pr.get("subtype") or "")
    cl = set(cls.replace(",", " ").split())
    if cl & LOWRISE:
        return 1, "class"
    if cl & RELIGIOUS:
        return 1, "class"
    if STYLE == "karachi":
        # Lyari: 2-4 storeys, mode 3 (research 08); small plots stay low
        if area < 25:
            return rnd.choices([1, 2, 3], [0.35, 0.5, 0.15])[0], "est"
        return rnd.choices([2, 3, 4, 5, 6], [0.18, 0.34, 0.30, 0.14, 0.04])[0], "est"
    # Delhi: basti 2-4, colonies 3-5, large institutional footprints low
    if area < 50:
        return rnd.choices([1, 2, 3, 4], [0.10, 0.40, 0.40, 0.10])[0], "est"
    if area < 160:
        return rnd.choices([2, 3, 4, 5], [0.20, 0.38, 0.30, 0.12])[0], "est"
    if area > 700:
        return rnd.choices([1, 2, 3], [0.3, 0.5, 0.2])[0], "est"
    return rnd.choices([2, 3, 4, 5], [0.22, 0.30, 0.30, 0.18])[0], "est"


tree = STRtree(polys)
out = []
tagged = 0
order = sorted(range(len(polys)), key=lambda i: polys[i].centroid.x ** 2 + polys[i].centroid.y ** 2)
if args.limit:
    order = order[:args.limit]
for i in order:
    q = polys[i]
    cx, cy = q.centroid.x, q.centroid.y
    if abs(cx) > half or abs(cy) > half:
        continue
    q = q.simplify(0.25, preserve_topology=True)
    if q.area < 10 or q.geom_type != "Polygon":
        continue
    if exclude is not None and q.intersection(exclude).area > 0.25 * q.area:
        continue  # sits on a platform / track bed: the station geometry replaces it
    q = shapely.geometry.polygon.orient(q, 1.0)
    pts = list(q.exterior.coords)[:-1]
    pr = props[i]
    rnd = random.Random(i * 7919 + 13)
    floors, src = pick_floors(pr, q.area, rnd)
    tagged += src == "tagged"
    if q.area < 30:
        floors = min(floors, 3)
    cls = set(((pr.get("class") or "") + " " + (pr.get("subtype") or "")).replace(",", " ").split())
    kind = "religious" if cls & RELIGIOUS else "station" if ("train_station" in cls or "transportation" in cls) else \
        "shed" if cls & LOWRISE else "house"
    nm = (pr.get("names") or {}).get("primary", "") if isinstance(pr.get("names"), dict) else ""
    edges = []
    n = len(pts)
    for j in range(n):
        a, b = np.array(pts[j]), np.array(pts[(j + 1) % n])
        d = b - a
        L = float(np.linalg.norm(d))
        if L < 1e-6:
            edges.append({"exposed": False, "street": False})
            continue
        nrm = np.array([d[1], -d[0]]) / L
        mid = (a + b) / 2
        probe = Point(*(mid + nrm * 0.8))
        hits = [k for k in tree.query(probe) if polys[k] is not polys[i] and polys[k].contains(probe)]
        exposed = not hits
        street = False
        if exposed and road_tree is not None:
            near = road_tree.query(Point(*(mid + nrm * 2)).buffer(5), predicate="intersects")
            street = len(near) > 0
        edges.append({"exposed": exposed, "street": street})
    hull = q.convex_hull
    if hull.area / q.area < 1.08:
        pieces = [list(hull.exterior.coords)[:-1]]
    else:
        tris = shapely.constrained_delaunay_triangles(q)
        pieces = [list(t.exterior.coords)[:-1] for t in tris.geoms if t.area > 0.05]
    out.append({"id": f"Bldg_{len(out):04d}", "footprint": [list(map(float, p)) for p in pts], "floors": floors,
                "edges": edges, "collision": [[list(map(float, p)) for p in pc] for pc in pieces],
                "seed": rnd.randrange(1 << 30), "area": q.area, "kind": kind, "name": nm})

# ---------------------------------------------------------------- road records
road_recs = []
for tags, pts in roads:
    layer = 0
    try:
        layer = int(tags.get("layer", "0"))
    except ValueError:
        pass
    road_recs.append({"type": tags["highway"], "pts": [list(map(float, p)) for p in pts],
                      "bridge": tags.get("bridge") in ("yes", "viaduct"), "tunnel": tags.get("tunnel") in ("yes", "building_passage", "culvert"),
                      "layer": layer, "name": tags.get("name", "")})

# ---------------------------------------------------------------- POIs (for missions / NPC spawn / navigation)
pois = []
KEYS = ("amenity", "shop", "tourism", "leisure", "public_transport", "railway", "historic", "man_made", "place", "building")
for nid, t in node_tags:
    if not (t.get("name") or t.get("amenity") or t.get("shop") or t.get("railway") in ("signal", "station", "halt")):
        continue
    x, y = to_local([nodes[nid]])[0]
    if abs(x) > half or abs(y) > half:
        continue
    kind = next((f"{k}={t[k]}" for k in KEYS if k in t), "named")
    pois.append({"name": t.get("name", ""), "kind": kind, "x": round(x, 2), "y": round(y, 2),
                 "lat": round(nodes[nid][1], 6), "lon": round(nodes[nid][0], 6),
                 "religious": t.get("amenity") == "place_of_worship"})
for tags, pts in osm_buildings:
    c = Polygon(pts).centroid
    if abs(c.x) <= half and abs(c.y) <= half:
        pois.append({"name": tags.get("name", ""), "kind": f"building={tags.get('building')}", "x": round(c.x, 2),
                     "y": round(c.y, 2), "lat": round(lat0 + c.y / ky, 6), "lon": round(lon0 + c.x / kx, 6),
                     "religious": tags.get("amenity") == "place_of_worship" or tags.get("building") in ("mosque", "temple")})

data = {"location": args.location, "title": cfg["title"], "style": STYLE, "center": [lat0, lon0], "size": SIZE,
        "buildings": out, "roads": road_recs, "rail": rail}
json.dump(data, open(P["slice"], "w"))
json.dump({"location": args.location, "center": [lat0, lon0], "frame": "local metres, X east, Y north, origin = slice centre",
           "pois": pois}, open(P["work"] / "pois.json", "w"), indent=1)
ex = sum(e["exposed"] for b in out for e in b["edges"])
st = sum(e["street"] for b in out for e in b["edges"])
fl = np.array([b["floors"] for b in out]) if out else np.array([0])
print(f"centre {lat0:.5f},{lon0:.5f} size {SIZE:.0f} m  buildings {len(out)} (height tagged {tagged}, mean floors "
      f"{fl.mean():.2f})  edges exposed {ex} street {st}  roads {len(road_recs)}  pois {len(pois)}")
