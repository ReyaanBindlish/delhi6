"""Crowd lanes from OSM street centrelines -> <location>_lanes.json (+ ZoneShape actors in the open level).

Two modes in one file (it picks the mode by whether the `unreal` module exists):

1. LANES (plain Python, e.g. the characters venv; no Unreal needed)
     C:\\Users\\Rayaan\\Desktop\\game\\characters\\.venv\\Scripts\\python.exe build_zonegraph.py lanes --location nizamuddin
     ... lanes --all                      # every story location that has an OSM file
     ... lanes --location karachi_lyari --origin 24.8752 66.9944
   Reads world/story/source/<location>.osm (or the Lyari / Old Delhi sources), splits walkable ways at
   intersections, classifies each segment into a street type (rows of Data/DT_StreetDensity_Story.csv),
   and writes OldDelhi/Content/Crowd/Lanes/<location>_lanes.json in Unreal world cm
   (X east, Y south, Z up, origin = bbox centre, same frame as osm_to_blender.py / build_vertical_slice.py).
   UDhCrowdSubsystem loads this file at runtime: the crowd works without any ZoneGraph data.

2. UE (inside the editor, optional; for the Mass / MassCrowd ZoneGraph path)
     set DH_LOCATION=nizamuddin
     set DH_WORLD_OFFSET=0,0,0            (cm; shifts lanes into the level's frame)
     UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="<this file>" -unattended -nop4
   Opens the current level, spawns one ZoneShape per OSM way (spline, pedestrian lane profile) plus a
   ZoneGraphData actor, and logs how to trigger the ZoneGraph build. Everything is best effort: the
   exposed Python surface of the ZoneGraph plugin differs between engine versions, so every step is
   wrapped and reports instead of failing.
"""
import argparse
import csv
import json
import math
import os
import sys
from pathlib import Path

try:
    import unreal  # type: ignore
    IN_UE = True
except ImportError:
    unreal = None
    IN_UE = False

REPO = Path(__file__).resolve().parents[3]
PROJECT = REPO / "OldDelhi"
SRC = REPO / "world" / "story" / "source"
LANE_DIR = PROJECT / "Content" / "Crowd" / "Lanes"

# Extra sources not listed in _tiles_bbox.csv. bbox = S, W, N, E ; origin = (lat, lon) of the UE world origin.
EXTRA = {
    "karachi_lyari": dict(osm=REPO / "world" / "karachi" / "source" / "karachi_lyari.osm",
                          bbox=(24.8640, 66.9850, 24.8800, 67.0010), origin=(24.8752, 66.9944)),
    "old_delhi": dict(osm=REPO / "world" / "delhi" / "source" / "old_delhi_chandni_chowk.osm",
                      bbox=None, origin=None),
}

R_EARTH = 6378137.0

WALK_WIDTH_M = {  # carriageway width incl. footpaths when OSM has no width tag
    "trunk": 18.0, "primary": 14.0, "primary_link": 8.0, "secondary": 12.0, "secondary_link": 7.0,
    "tertiary": 9.0, "tertiary_link": 6.0, "unclassified": 7.0, "residential": 6.0, "living_street": 5.0,
    "service": 4.0, "pedestrian": 5.0, "footway": 2.5, "path": 2.0, "track": 3.0, "cycleway": 2.5,
}
SKIP_FOOTWAY = {"sidewalk", "crossing", "traffic_island"}
SKIP_SERVICE = {"driveway", "parking_aisle"}
MAIN = {"trunk", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "tertiary_link"}
GALI = {"residential", "living_street", "service", "unclassified", "track"}


# ------------------------------------------------------------------------------------------ OSM parsing
def read_bboxes():
    out = {}
    f = SRC / "_tiles_bbox.csv"
    if f.exists():
        for row in csv.reader(open(f, encoding="utf-8")):
            if len(row) >= 5:
                out[row[0]] = tuple(float(v) for v in row[1:5])
    return out


def parse_osm(path):
    """Returns nodes {id: (lat, lon)}, ways [(id, refs, tags)], pois [(lat, lon, tags)], bounds."""
    import xml.etree.ElementTree as ET

    nodes, ways, pois = {}, [], []
    bounds = None
    for ev, el in ET.iterparse(str(path), events=("end",)):
        tag = el.tag
        if tag == "node":
            lat, lon = float(el.get("lat")), float(el.get("lon"))
            nodes[el.get("id")] = (lat, lon)
            tags = {t.get("k"): t.get("v") for t in el.findall("tag")}
            if tags:
                pois.append((lat, lon, tags))
            el.clear()
        elif tag == "way":
            refs = [nd.get("ref") for nd in el.findall("nd")]
            tags = {t.get("k"): t.get("v") for t in el.findall("tag")}
            ways.append((el.get("id"), refs, tags))
            el.clear()
        elif tag == "bounds":
            bounds = (float(el.get("minlat")), float(el.get("minlon")), float(el.get("maxlat")), float(el.get("maxlon")))
        elif tag == "relation":
            el.clear()
    return nodes, ways, pois, bounds


# ------------------------------------------------------------------------------------------ POI index
class PoiIndex:
    """Grid hash of typed points of interest in local metres."""

    CELL = 100.0

    def __init__(self):
        self.cells = {}

    def add(self, kind, x, y):
        self.cells.setdefault((int(x // self.CELL), int(y // self.CELL)), []).append((kind, x, y))

    def near(self, x, y, radius):
        out = {}
        r = int(radius // self.CELL) + 1
        cx, cy = int(x // self.CELL), int(y // self.CELL)
        for ix in range(cx - r, cx + r + 1):
            for iy in range(cy - r, cy + r + 1):
                for kind, px, py in self.cells.get((ix, iy), ()):
                    d = math.hypot(px - x, py - y)
                    if d <= radius and d < out.get(kind, 1e9):
                        out[kind] = d
        return out


def poi_kind(t):
    """Maps OSM tags to a crowd POI kind or None."""
    name = (t.get("name") or "").lower()
    if t.get("amenity") == "prison" or t.get("building") == "prison":
        return "jail"
    if t.get("barrier") == "checkpoint" or t.get("military") in ("checkpoint", "bunker") or \
            any(k in name for k in ("ranger", "checkpost", "check post", "naka")):
        return "check"
    if t.get("amenity") == "police":
        return "police"
    if t.get("harbour") == "yes" or t.get("man_made") in ("pier", "quay") or t.get("landuse") in ("harbour", "port") or \
            t.get("waterway") in ("dock", "boatyard") or t.get("amenity") == "boat_rental" or \
            any(k in name for k in ("jetty", "harbour", "harbor", "fish")):
        return "harbour"
    if t.get("railway") in ("station", "halt") or t.get("public_transport") == "station" or t.get("amenity") == "bus_station":
        return "station"
    if t.get("amenity") == "place_of_worship" or t.get("building") in ("mosque", "temple", "shrine", "church", "gurdwara"):
        shrine = any(k in name for k in ("dargah", "shrine", "mazar", "ghazi", "darbar"))
        return "shrine" if shrine or t.get("historic") == "shrine" else \
            ("mosque" if t.get("religion") == "muslim" or t.get("building") == "mosque" else "shrine")
    if t.get("shop") or t.get("amenity") in ("marketplace", "restaurant", "cafe", "fast_food", "bank", "pharmacy"):
        return "shop"
    return None


# ------------------------------------------------------------------------------------------ lanes
def build_lanes(location, origin=None, bbox=None, osm_path=None, verbose=True):
    bbox_table = read_bboxes()
    extra = EXTRA.get(location, {})
    osm_path = Path(osm_path) if osm_path else (extra.get("osm") or SRC / f"{location}.osm")
    if not Path(osm_path).exists():
        raise FileNotFoundError(f"OSM source not found: {osm_path}")
    bbox = bbox or extra.get("bbox") or bbox_table.get(location)

    nodes, ways, pois, bounds = parse_osm(osm_path)
    if bbox is None:
        bbox = bounds
    if bbox is None:
        lats = [p[0] for p in nodes.values()]; lons = [p[1] for p in nodes.values()]
        bbox = (min(lats), min(lons), max(lats), max(lons))
    lat0, lon0 = origin or extra.get("origin") or ((bbox[0] + bbox[2]) / 2, (bbox[1] + bbox[3]) / 2)
    kx = math.radians(1) * R_EARTH * math.cos(math.radians(lat0))
    ky = math.radians(1) * R_EARTH

    def inside(nid):
        p = nodes.get(nid)
        return p is not None and bbox[0] <= p[0] <= bbox[2] and bbox[1] <= p[1] <= bbox[3]

    def xy(nid):
        la, lo = nodes[nid]
        return ((lo - lon0) * kx, (la - lat0) * ky)  # metres: east, north

    # points of interest (nodes with tags, ways via the centroid of their nodes)
    index = PoiIndex()
    for la, lo, t in pois:
        k = poi_kind(t)
        if k:
            index.add(k, (lo - lon0) * kx, (la - lat0) * ky)
    for _, refs, t in ways:
        k = poi_kind(t)
        if k and refs:
            pts = [xy(r) for r in refs if r in nodes]
            if pts:
                index.add(k, sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts))

    # hand-placed POIs for things OSM does not tag (jetties, rangers posts, dargah forecourts):
    #   Tools/ue/crowd_poi_overrides.json = {"karachi_ibrahim_hyderi": [{"kind": "harbour", "lat": 24.79, "lon": 67.14}]}
    override_file = Path(__file__).with_name("crowd_poi_overrides.json")
    if override_file.exists():
        for p in json.load(open(override_file, encoding="utf-8")).get(location, []):
            index.add(p["kind"], (p["lon"] - lon0) * kx, (p["lat"] - lat0) * ky)

    # walkable ways
    walk = []
    for wid, refs, t in ways:
        hw = t.get("highway")
        if hw not in WALK_WIDTH_M:
            continue
        if t.get("access") in ("private", "no") or t.get("area") == "yes":
            continue
        if hw == "footway" and t.get("footway") in SKIP_FOOTWAY:
            continue
        if hw == "service" and t.get("service") in SKIP_SERVICE:
            continue
        # clip to bbox: keep runs of consecutive inside nodes
        run = []
        for r in refs:
            if inside(r):
                run.append(r)
            else:
                if len(run) >= 2:
                    walk.append((wid, run, t))
                run = []
        if len(run) >= 2:
            walk.append((wid, run, t))

    # intersections: nodes used by 2+ walkable ways (or interior nodes of 2 ways), plus way ends
    use = {}
    for _, refs, _ in walk:
        for r in set(refs):
            use[r] = use.get(r, 0) + 1

    karachi_gali = location.startswith("karachi_lyari") or location.startswith("lyari")
    sector = location.startswith("vasundhara")

    # shop density needs counts, not nearest distance: separate cheap counter
    shop_pts = [(x, y) for cell in index.cells.values() for k, x, y in cell if k == "shop"]
    shop_grid = {}
    for x, y in shop_pts:
        shop_grid.setdefault((int(x // 50), int(y // 50)), []).append((x, y))

    def shop_count(mid, radius=40.0):
        n = 0
        cx, cy = int(mid[0] // 50), int(mid[1] // 50)
        for ix in range(cx - 1, cx + 2):
            for iy in range(cy - 1, cy + 2):
                for x, y in shop_grid.get((ix, iy), ()):
                    if math.hypot(x - mid[0], y - mid[1]) <= radius:
                        n += 1
        return n

    def classify(hw, mid):
        near = index.near(mid[0], mid[1], 120.0)
        if "jail" in near:
            return "JailGate"
        if "check" in near and near["check"] < 70:
            return "CheckpointApproach"
        if "police" in near and near["police"] < 40 and location.startswith("karachi"):
            return "CheckpointApproach"
        if "harbour" in near and near["harbour"] < 90:
            return "HarbourFront"
        if "station" in near and near["station"] < 90:
            return "StationForecourt"
        if "shrine" in near and near["shrine"] < 55:
            return "ShrineForecourt"
        if "mosque" in near and near["mosque"] < 35:
            return "MosqueGate"
        if hw == "trunk":
            return "HighwayVerge"
        shops = shop_count(mid)
        if hw in MAIN:
            return "BazaarGalli" if (hw.startswith("tertiary") and shops >= 4) else "MainRoad"
        if shops >= 3:
            return "BazaarGalli"
        if hw in ("pedestrian", "footway", "path", "cycleway"):
            return "ResidentialGalli"
        if karachi_gali:
            return "LyariGali"
        if sector:
            return "SectorStreet"
        return "ResidentialGalli"

    def to_cm(pt, off):
        return [round((pt[0] * 100.0) + off[0], 1), round((-pt[1] * 100.0) + off[1], 1), off[2]]

    lanes, shapes = [], []
    for wid, refs, t in walk:
        hw = t["highway"]
        try:
            width = float(str(t.get("width", "")).split()[0])
        except (ValueError, IndexError):
            width = WALK_WIDTH_M[hw]
        if karachi_gali and hw in GALI:
            width = min(width, 5.0)
        width = max(2.0, min(width, 25.0))
        pts_m = [xy(r) for r in refs]
        if sum(math.dist(pts_m[i], pts_m[i + 1]) for i in range(len(pts_m) - 1)) < 3.0:
            continue

        # whole-way shape for ZoneGraph
        mid_way = pts_m[len(pts_m) // 2]
        shapes.append(dict(way=wid, street=classify(hw, mid_way), width_cm=round(width * 100), points_m=[[round(p[0], 2), round(p[1], 2)] for p in pts_m]))

        # lane segments, split at intersections
        seg = [0]
        for i in range(1, len(refs)):
            if use.get(refs[i], 0) >= 2 or i == len(refs) - 1:
                seg.append(i)
        for a, b in zip(seg[:-1], seg[1:]):
            sub = pts_m[a:b + 1]
            length = sum(math.dist(sub[i], sub[i + 1]) for i in range(len(sub) - 1))
            if length < 3.0:
                continue
            mid = sub[len(sub) // 2]
            side = (width * 100.0 * 0.5 - 100.0) if width >= 8.0 else 0.0  # footpath 1 m inside the kerb
            lanes.append(dict(street=classify(hw, mid), width_cm=round(width * 100), side_cm=round(max(side, 0.0)),
                              hw=hw, points=[to_cm(p, (0, 0, 0)) for p in sub]))
    result = dict(location=location, units="cm", frame="UE: X east, Y south, Z up; origin = lat/lon below",
                  origin=[lat0, lon0], bbox=list(bbox), source=str(osm_path.name), lanes=lanes, shapes=shapes)
    if verbose:
        by = {}
        for l in lanes:
            by[l["street"]] = by.get(l["street"], 0) + 1
        total_km = sum(sum(math.dist(l["points"][i][:2], l["points"][i + 1][:2]) for i in range(len(l["points"]) - 1)) for l in lanes) / 100000.0
        print(f"[crowd] {location}: {len(lanes)} lane segments, {len(shapes)} ways, {total_km:.1f} km  {by}")
    return result


def write_lanes(location, **kw):
    data = build_lanes(location, **kw)
    LANE_DIR.mkdir(parents=True, exist_ok=True)
    out = LANE_DIR / f"{location}_lanes.json"
    with open(out, "w", encoding="utf-8") as f:
        json.dump(data, f, separators=(",", ":"))
    print(f"[crowd] wrote {out} ({out.stat().st_size / 1024:.0f} KB)")
    return out


# ------------------------------------------------------------------------------------------ Unreal side
def ue_log(*a):
    unreal.log("[crowd-zg] " + " ".join(str(x) for x in a))


def ue_spawn_zone_graph(location, offset_cm):
    """Best effort: ZoneShape per way + ZoneGraphData actor in the loaded level."""
    path = LANE_DIR / f"{location}_lanes.json"
    if not path.exists():
        ue_log("lane file missing, writing it first")
        write_lanes(location)
    data = json.load(open(path, encoding="utf-8"))
    shapes = data.get("shapes", [])
    limit = int(os.environ.get("DH_ZG_MAX", "4000"))
    ue_log(f"{location}: {len(shapes)} ways, spawning up to {limit}")

    shape_cls = getattr(unreal, "ZoneShape", None)
    if shape_cls is None:
        ue_log("unreal.ZoneShape is not exposed. Enable the ZoneGraph plugin, or place shapes by hand; "
               "the runtime crowd still works from the lane JSON.")
        return
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # a data actor that owns the baked graph (bounds are recomputed on build)
    data_cls = getattr(unreal, "ZoneGraphData", None)
    if data_cls is not None and not [a for a in actors.get_all_level_actors() if isinstance(a, data_cls)]:
        try:
            actors.spawn_actor_from_class(data_cls, unreal.Vector(0, 0, 0))
            ue_log("spawned ZoneGraphData")
        except Exception as e:  # noqa: BLE001
            ue_log("could not spawn ZoneGraphData:", e)

    spawned = 0
    for s in shapes[:limit]:
        pts = s["points_m"]
        if len(pts) < 2:
            continue
        try:
            loc0 = unreal.Vector(pts[0][0] * 100 + offset_cm[0], -pts[0][1] * 100 + offset_cm[1], offset_cm[2])
            actor = actors.spawn_actor_from_class(shape_cls, loc0)
            actor.set_actor_label(f"ZG_{location}_{s['way']}")
            comp = actor.get_component_by_class(unreal.ZoneShapeComponent)
            comp_points = []
            for p in pts:
                zp = unreal.ZoneShapePoint()
                zp.set_editor_property("position", unreal.Vector(p[0] * 100 + offset_cm[0] - loc0.x,
                                                                 -p[1] * 100 + offset_cm[1] - loc0.y, 0))
                comp_points.append(zp)
            comp.set_editor_property("points", comp_points)
            actor.set_folder_path(f"Crowd/ZoneGraph/{location}")
            spawned += 1
        except Exception as e:  # noqa: BLE001
            ue_log("shape failed:", s["way"], e)
            break
    ue_log(f"spawned {spawned} ZoneShape actors. Next: assign the 'DhPedestrian' lane profile (see docs/CROWD.md) "
           "and build the graph from the ZoneGraph toolbar / 'ZoneGraph.Build'.")
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    except Exception as e:  # noqa: BLE001
        ue_log("save level failed:", e)


# ------------------------------------------------------------------------------------------ entry
def main_cli(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["lanes"])
    ap.add_argument("--location")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--origin", type=float, nargs=2)
    ap.add_argument("--osm")
    ap.add_argument("--bbox", type=float, nargs=4, metavar=("S", "W", "N", "E"))
    a = ap.parse_args(argv)
    if a.all:
        names = [n for n in read_bboxes() if (SRC / f"{n}.osm").exists()] + ["karachi_lyari"]
    elif a.location:
        names = [a.location]
    else:
        ap.error("--location or --all required")
    for n in names:
        try:
            write_lanes(n, origin=tuple(a.origin) if a.origin else None, bbox=tuple(a.bbox) if a.bbox else None, osm_path=a.osm)
        except FileNotFoundError as e:
            print(f"[crowd] skip {n}: {e}")


if IN_UE:
    loc = os.environ.get("DH_LOCATION", "karachi_lyari")
    off = [float(v) for v in os.environ.get("DH_WORLD_OFFSET", "0,0,0").split(",")]
    ue_spawn_zone_graph(loc, off)
elif __name__ == "__main__":
    main_cli(sys.argv[1:])
