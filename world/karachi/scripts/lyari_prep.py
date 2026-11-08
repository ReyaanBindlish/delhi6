"""Prepare a Lyari vertical slice for procedural building generation.

Inputs : source/lyari_overture_buildings.geojson (Overture: Google Open Buildings + OSM)
         source/karachi_lyari.osm (roads)
Output : lyari_slice.json - per building: footprint (local metres, CCW), floors, per-edge
         flags (exposed / street-facing), convex collision pieces, seed; plus road polylines.

Usage: python lyari_prep.py [--size 300] [--center LAT LON]
"""
import argparse
import json
import math
import random
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np
import shapely
from shapely.geometry import LineString, Point, Polygon, shape
from shapely.strtree import STRtree

ROOT = Path(__file__).resolve().parent.parent
ap = argparse.ArgumentParser()
ap.add_argument("--size", type=float, default=300.0)
ap.add_argument("--center", type=float, nargs=2)
ap.add_argument("--geojson", default=str(ROOT / "source" / "lyari_overture_buildings.geojson"))
ap.add_argument("--osm", default=str(ROOT / "source" / "karachi_lyari.osm"))
ap.add_argument("--out", default=str(ROOT / "lyari_slice.json"))
args = ap.parse_args()

feats = json.load(open(args.geojson, encoding="utf-8"))["features"]
polys_ll = []
for f in feats:
    g = shape(f["geometry"])
    if g.geom_type == "MultiPolygon":
        g = max(g.geoms, key=lambda p: p.area)
    if g.geom_type == "Polygon":
        polys_ll.append(g)

# choose slice centre: densest cell unless given
if args.center:
    lat0, lon0 = args.center
else:
    c = np.array([[p.centroid.y, p.centroid.x] for p in polys_ll])
    cell = args.size / 111320.0
    keys = np.floor(c / cell).astype(int)
    uniq, counts = np.unique(keys, axis=0, return_counts=True)
    k = uniq[np.argmax(counts)]
    lat0, lon0 = (k + 0.5) * cell
R = 6378137.0
kx = math.radians(1) * R * math.cos(math.radians(lat0)); ky = math.radians(1) * R


def to_local(coords):
    return [((lo - lon0) * kx, (la - lat0) * ky) for lo, la in coords]


half = args.size / 2
margin = 40.0
polys = []
for p in polys_ll:
    q = Polygon(to_local(p.exterior.coords))
    if not q.is_valid:
        q = q.buffer(0)
        if q.geom_type != "Polygon":
            continue
    cx, cy = q.centroid.x, q.centroid.y
    if abs(cx) < half + margin and abs(cy) < half + margin:
        polys.append(q)

# roads from OSM
root = ET.parse(args.osm).getroot()
nodes = {n.get("id"): (float(n.get("lon")), float(n.get("lat"))) for n in root.iter("node")}
roads = []
for w in root.iter("way"):
    tags = {t.get("k"): t.get("v") for t in w.iter("tag")}
    if "highway" not in tags:
        continue
    pts = to_local([nodes[nd.get("ref")] for nd in w.iter("nd") if nd.get("ref") in nodes])
    if len(pts) >= 2:
        ls = LineString(pts)
        if ls.intersects(shapely.box(-half - margin, -half - margin, half + margin, half + margin)):
            roads.append((tags["highway"], pts))
road_lines = [LineString(p) for _, p in roads]
road_tree = STRtree(road_lines) if road_lines else None

tree = STRtree(polys)
FLOORS = [2, 3, 4, 5, 6]
WEIGHTS = [0.12, 0.28, 0.32, 0.18, 0.10]
out = []
for i, q in enumerate(polys):
    cx, cy = q.centroid.x, q.centroid.y
    if abs(cx) > half or abs(cy) > half:
        continue  # margin buildings only used for adjacency tests
    q = q.simplify(0.25, preserve_topology=True)
    if q.area < 10 or q.geom_type != "Polygon":
        continue
    q = shapely.geometry.polygon.orient(q, 1.0)  # CCW
    pts = list(q.exterior.coords)[:-1]
    rnd = random.Random(i * 7919 + 13)
    floors = rnd.choices(FLOORS, WEIGHTS)[0]
    if q.area < 30:
        floors = min(floors, 3)
    edges = []
    n = len(pts)
    for j in range(n):
        a, b = np.array(pts[j]), np.array(pts[(j + 1) % n])
        d = b - a
        L = float(np.linalg.norm(d))
        if L < 1e-6:
            edges.append({"exposed": False, "street": False}); continue
        nrm = np.array([d[1], -d[0]]) / L  # outward for CCW
        mid = (a + b) / 2
        probe = Point(*(mid + nrm * 0.8))
        hits = [k for k in tree.query(probe) if polys[k] is not polys[i] and polys[k].contains(probe)]
        exposed = not hits
        street = False
        if exposed and road_tree is not None:
            near = road_tree.query(Point(*(mid + nrm * 2)).buffer(5), predicate="intersects")
            street = len(near) > 0
        edges.append({"exposed": exposed, "street": street})
    # convex collision pieces
    hull = q.convex_hull
    if hull.area / q.area < 1.08:
        pieces = [list(hull.exterior.coords)[:-1]]
    else:
        tris = shapely.constrained_delaunay_triangles(q)
        pieces = [list(t.exterior.coords)[:-1] for t in tris.geoms if t.area > 0.05]
    out.append({
        "id": f"Bldg_{len(out):04d}", "footprint": [list(map(float, p)) for p in pts],
        "floors": floors, "edges": edges, "collision": [[list(map(float, p)) for p in pc] for pc in pieces],
        "seed": rnd.randrange(1 << 30), "area": q.area,
    })

data = {"center": [lat0, lon0], "size": args.size, "buildings": out,
        "roads": [{"type": t, "pts": p} for t, p in roads]}
json.dump(data, open(args.out, "w"))
ex = sum(e["exposed"] for b in out for e in b["edges"]); st = sum(e["street"] for b in out for e in b["edges"])
print(f"centre {lat0:.5f},{lon0:.5f}  buildings {len(out)}  edges exposed {ex} street {st}  roads {len(roads)}")
