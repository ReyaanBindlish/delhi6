"""Bare-earth terrain for a story location (method of world/delhi/scripts/make_terrain_v2.py, unchanged original).

python make_terrain_story.py <location>

Copernicus GLO-30 DSM -> mask Overture footprints (+1 cell) and anything >1.5 m above a morphological opening ->
Laplace fill -> gaussian -> DTM <= DSM -> bicubic resample to a 1009 x 1009 grid at 1 m (Landscape-ready, 1008 m)
centred on the location centre. Writes terrain/<loc>/:
  dtm_1009.npy (m ASL, row 0 = north) | LS_<loc>_1009.png (16-bit Landscape heightmap) | terrain.json |
  landscape.json (import settings) | preview_hillshade.png
Level frame: Z = metres above base_elev_m (minimum DTM), X east, Y north.
"""
import json
import math
import sys
from pathlib import Path

import numpy as np
import tifffile
from PIL import Image, ImageDraw
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent))
import loc_config as C

name = sys.argv[1]
cfg = C.LOCATIONS[name]
OUT = C.TERRAIN_ROOT / name
OUT.mkdir(parents=True, exist_ok=True)
LAT0, LON0 = cfg["center"]
N, EXT = C.TERRAIN_N, C.TERRAIN_EXTENT_M
HALF = EXT / 2
R = 6378137.0
mlat = math.radians(1) * R
mlon = math.radians(1) * R * math.cos(math.radians(LAT0))

dsm_full = tifffile.imread(cfg["dem"]).astype(np.float32)
top, left = cfg["dem_tile"]
S = dsm_full.shape[0]                      # pixels per degree
pad = 0.01
r0 = max(0, int((top - (LAT0 + HALF / mlat + pad)) * S)); r1 = min(S, int((top - (LAT0 - HALF / mlat - pad)) * S) + 1)
c0 = max(0, int((LON0 - HALF / mlon - pad - left) * S)); c1 = min(S, int((LON0 + HALF / mlon + pad - left) * S) + 1)
dsm = dsm_full[r0:r1, c0:c1]
H, W = dsm.shape


def to_px(lon, lat):
    return (lon - left) * S - c0, (top - lat) * S - r0


SS = 4
mimg = Image.new("L", (W * SS, H * SS), 0)
draw = ImageDraw.Draw(mimg)
geo = json.load(open(cfg["geojson"], encoding="utf-8"))
nb = 0
for f in geo["features"]:
    g = f["geometry"]
    polys = [g["coordinates"]] if g["type"] == "Polygon" else g["coordinates"] if g["type"] == "MultiPolygon" else []
    for p in polys:
        ring = [(to_px(x, y)[0] * SS, to_px(x, y)[1] * SS) for x, y in p[0]]
        if len(ring) >= 3 and all(-50 < a < W * SS + 50 and -50 < b < H * SS + 50 for a, b in ring[:1]):
            draw.polygon(ring, fill=255); nb += 1
cover = np.asarray(mimg.resize((W, H), Image.BOX), dtype=np.float32) / 255.0
built = ndimage.binary_dilation(cover > 0.25, iterations=1)
print(f"footprints {nb}, masked {built.mean() * 100:.1f}% of crop ({W}x{H} px)")

opened = ndimage.grey_opening(dsm, size=(7, 7))
nonground = built | ((dsm - opened) > 1.5)
ground = np.where(nonground, np.nan, dsm)
idx = ndimage.distance_transform_edt(np.isnan(ground), return_distances=False, return_indices=True)
filled = ground[tuple(idx)]
for _ in range(200):
    sm = ndimage.uniform_filter(filled, size=3)
    filled = np.where(nonground, sm, dsm)
dtm = np.minimum(filled, opened + 0.5)
dtm = ndimage.gaussian_filter(dtm, sigma=2.5)
dtm = np.minimum(dtm, dsm)

ys = np.linspace(HALF, -HALF, N); xs = np.linspace(-HALF, HALF, N)
lat = LAT0 + ys / mlat; lon = LON0 + xs / mlon
RR, CC = np.meshgrid((top - lat) * S - 0.5 - r0, (lon - left) * S - 0.5 - c0, indexing="ij")
grid = ndimage.map_coordinates(dtm, [RR, CC], order=3, mode="nearest").astype(np.float32)
grid = ndimage.gaussian_filter(grid, sigma=25).astype(np.float32)

base = float(grid.min()); mid = float((grid.max() + grid.min()) / 2)
gy, gx = np.gradient(grid, 1.0)
slope = np.hypot(gx, gy)
print(f"{name}: DTM {grid.min():.1f}-{grid.max():.1f} m ASL, slope p95 = 1:{1 / max(np.percentile(slope, 95), 1e-6):.0f}")

# Landscape encoding: world Z(cm) = actor_z + (v - 32768) * zscale / 128 ; actor_z at mid height (level frame, above base)
half_range_cm = max(grid.max() - mid, mid - grid.min()) * 100.0
zscale = max(5.0, math.ceil(half_range_cm / 256.0 * 1.15 * 10) / 10)
actor_z_cm = (mid - base) * 100.0
val = np.clip(np.round(32768 + ((grid - mid) * 100.0) * 128.0 / zscale), 0, 65535).astype(np.uint16)
Image.fromarray(val).save(OUT / f"LS_{name}_{N}.png")
np.save(OUT / f"dtm_{N}.npy", grid)
small = grid[::2, ::2]
dy, dx = np.gradient(small, 2.0)
Image.fromarray((np.clip(0.5 + (-dx * 0.7 + dy * 0.7) * 40, 0, 1) * 255).astype(np.uint8)).save(OUT / "preview_hillshade.png")

meta = {"location": name, "centre_latlon": [LAT0, LON0], "extent_m": EXT, "size_px": N, "pixel_m": 1.0,
        "n": N, "dtm_file": f"dtm_{N}.npy", "base_elev_m": base, "elev_min_m": float(grid.min()),
        "elev_max_m": float(grid.max()), "frame": "Z = metres above base_elev_m; X east, Y north; origin = centre",
        "method": "Copernicus GLO-30 DSM; Overture footprints + >1.5 m residual masked; Laplace fill; gaussian; DTM<=DSM",
        "source": cfg["dem"].name}
json.dump(meta, open(OUT / "terrain.json", "w"), indent=1)
json.dump({"heightmap": f"LS_{name}_{N}.png", "verts": N, "quad_m": 1.0,
           "ue_landscape": {"location_cm_unreal": [-HALF * 100, -HALF * 100, actor_z_cm],
                            "scale": [100.0, 100.0, zscale],
                            "section_size_quads": 63, "sections_per_component": "2x2", "components": "8x8 (1008 m)",
                            "note": "Unreal X=east, Y=south, so image row 0 (north) = most negative Y; actor location is the "
                                    "NW corner. Z is relative to the level origin where Z=0 is the DTM minimum "
                                    "(base_elev_m) - same frame as buildings and terrain tiles."}},
          open(OUT / "landscape.json", "w"), indent=1)
print(f"wrote {OUT}  base {base:.2f} m ASL, zscale {zscale}")
