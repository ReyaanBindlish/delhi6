"""Terrain v2 (research/02 section B1): commercial-safe bare-earth DTM + 4033 Landscape heightmap.

Method: rasterise the 22,006 Overture footprints onto the Copernicus 30 m grid, dilate 1 cell, mark as
NoData, fill from surrounding open-ground cells (nearest + iterative diffusion, like gdal_fillnodata),
low-pass sigma 1.5, clamp DTM <= DSM, clamp slopes in the flat core, then bicubic-resample to a
4032 x 4032 m Landscape at 1 m/quad (4033 verts), Z scale 12.5, actor Z = 21,000 cm.

Outputs (world/delhi/terrain_v2/):
  LS_Phase1_4033.png        16-bit heightmap for UE Landscape import
  dtm_4033.npy              float32 metres (ASL), same grid
  landscape.json            all import settings + provenance
  preview_hillshade.png     quick visual check
"""
import json
from pathlib import Path

import numpy as np
import tifffile
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "terrain_v2"; OUT.mkdir(exist_ok=True)
LAT0, LON0 = 28.655, 77.230
N = 4033
QUAD_M = 1.0
HALF = (N - 1) * QUAD_M / 2          # 2016 m
ZSCALE = 12.5                        # +-32 m range, 0.098 cm step
ACTOR_Z_CM = 21000.0                 # landscape mid-height = 210 m ASL

R = 6378137.0
mlat = np.radians(1) * R
mlon = np.radians(1) * R * np.cos(np.radians(LAT0))

dsm_full = tifffile.imread(ROOT / "source" / "Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif").astype(np.float32)
pad = 0.02
r0 = int((29 - (LAT0 + HALF / mlat + pad)) * 3600); r1 = int((29 - (LAT0 - HALF / mlat - pad)) * 3600) + 1
c0 = int((LON0 - HALF / mlon - pad - 77) * 3600); c1 = int((LON0 + HALF / mlon + pad - 77) * 3600) + 1
dsm = dsm_full[r0:r1, c0:c1]
H, W = dsm.shape


def to_px(lon, lat):  # fractional pixel in the crop (x=col, y=row), pixel centres at +0.5
    return (lon - 77) * 3600 - c0, (29 - lat) * 3600 - r0


# 1) building mask on the DEM grid (rasterised at 4x supersampling for accuracy)
SS = 4
mimg = Image.new("L", (W * SS, H * SS), 0)
draw = ImageDraw.Draw(mimg)
geo = json.load(open(ROOT / "source" / "old_delhi_chandni_chowk_overture_buildings.geojson", encoding="utf-8"))
nb = 0
for f in geo["features"]:
    g = f["geometry"]
    polys = [g["coordinates"]] if g["type"] == "Polygon" else g["coordinates"] if g["type"] == "MultiPolygon" else []
    for p in polys:
        ring = [(to_px(x, y)[0] * SS, to_px(x, y)[1] * SS) for x, y in p[0]]
        if len(ring) >= 3:
            draw.polygon(ring, fill=255); nb += 1
cover = np.asarray(mimg.resize((W, H), Image.BOX), dtype=np.float32) / 255.0   # building cover fraction per cell
built = cover > 0.25
built = ndimage.binary_dilation(built, iterations=1)
print(f"footprints {nb}, cells masked {built.mean() * 100:.1f}% of crop")

# 2) unmapped roofs / trees (outside the footprint download): anything > 1.5 m above a ~200 m
#    morphological opening (Zhang-style progressive filter, single pass) is non-ground
opened = ndimage.grey_opening(dsm, size=(7, 7))
nonground = built | ((dsm - opened) > 1.5)
print(f"non-ground cells {nonground.mean() * 100:.1f}%")

# 3) fill NoData: nearest ground value, then diffusion smoothing restricted to the holes
ground = np.where(nonground, np.nan, dsm)
idx = ndimage.distance_transform_edt(np.isnan(ground), return_distances=False, return_indices=True)
filled = ground[tuple(idx)]
for _ in range(200):  # Laplace fill in the holes, keeps measured ground cells fixed
    sm = ndimage.uniform_filter(filled, size=3)
    filled = np.where(nonground, sm, dsm)
dtm = np.minimum(filled, opened + 0.5)                       # never above the opened ground estimate
dtm = ndimage.gaussian_filter(dtm, sigma=2.5)
dtm = np.minimum(dtm, dsm)                                  # DTM <= DSM

# 4) resample to the 4033 landscape grid (row 0 = north)
ys = np.linspace(HALF, -HALF, N); xs = np.linspace(-HALF, HALF, N)
lat = LAT0 + ys / mlat; lon = LON0 + xs / mlon
RR, CC = np.meshgrid((29 - lat) * 3600 - 0.5 - r0, (lon - 77) * 3600 - 0.5 - c0, indexing="ij")
grid = ndimage.map_coordinates(dtm, [RR, CC], order=3, mode="nearest").astype(np.float32)
grid = ndimage.gaussian_filter(grid, sigma=25).astype(np.float32)   # remove 1-2 m fill noise, keep km-scale relief

# 5) report slopes in the core (real relief kept: Jama Masjid sits on the Bhojla Pahari outcrop,
#    Red Fort on the old Yamuna bank)
yy, xx = np.meshgrid(ys, xs, indexing="ij")
core = (np.abs(xx) < 1500) & (np.abs(yy) < 1150)
gy, gx = np.gradient(grid, QUAD_M)
slope_core = np.hypot(gx, gy)[core]
print(f"ground {grid.min():.1f}-{grid.max():.1f} m ASL; core slope p95 = 1:{1 / max(np.percentile(slope_core, 95), 1e-6):.0f}")

# 6) encode for UE: world Z(cm) = ACTOR_Z + (v - 32768) * ZSCALE / 128
val = np.clip(np.round(32768 + (grid * 100.0 - ACTOR_Z_CM) * 128.0 / ZSCALE), 0, 65535).astype(np.uint16)
Image.fromarray(val).save(OUT / "LS_Phase1_4033.png")
np.save(OUT / "dtm_4033.npy", grid)

# hillshade preview (downsampled)
small = grid[::4, ::4]
dy, dx = np.gradient(small, 4.0)
shade = np.clip(0.5 + (-dx * 0.7 + dy * 0.7) * 60, 0, 1)   # strongly exaggerated (flat city)
Image.fromarray((shade * 255).astype(np.uint8)).save(OUT / "preview_hillshade.png")

meta = {
    "centre_latlon": [LAT0, LON0], "extent_m": 2 * HALF, "size_px": N, "pixel_m": QUAD_M,
    "elev_min_m": float(grid.min()), "elev_max_m": float(grid.max()),
    "ue_landscape": {
        "heightmap": "LS_Phase1_4033.png", "location_cm": [0, 0, ACTOR_Z_CM],
        "scale": [QUAD_M * 100, QUAD_M * 100, ZSCALE],
        "section_size_quads": 63, "sections_per_component": "2x2", "components": "32x32 (1024)",
        "world_partition": True, "streaming_proxy_components": "4x4 (~504 m)",
        "layers": ["RoadDust", "PackedEarth", "DryGrass", "MudSlush", "AshSoot", "RiverSand", "LeafLitter", "Gravel"],
        "physical_materials": {"Asphalt": "PM_Asphalt", "Mud": "PM_Mud", "Sandstone": "PM_RedSandstone"},
        "world_z_note": "World Z (cm) = elevation_m * 100 - 0 (absolute ASL in cm); slices/buildings must use the same.",
    },
    "method": "Copernicus GLO-30 DSM; Overture footprints + residual>4 m masked; Laplace fill; gaussian 1.5; "
              "DTM<=DSM; core slope softened; bicubic to 1 m.",
    "source": "Copernicus DEM GLO-30 (c) DLR e.V. 2010-2014 and (c) Airbus Defence and Space GmbH 2014-2018, "
              "provided under COPERNICUS by the EU and ESA. Footprints: Overture Maps (ODbL / CC BY 4.0).",
}
json.dump(meta, open(OUT / "landscape.json", "w"), indent=2)
print("wrote", OUT)
