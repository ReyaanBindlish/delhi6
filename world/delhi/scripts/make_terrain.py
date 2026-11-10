"""Build an Unreal Landscape heightmap (bare-earth approximation) for Old Delhi from Copernicus DSM.

Copernicus GLO-30 is a *surface* model (includes rooftops). Old Delhi's ground is gently sloping,
so we approximate bare earth with a morphological opening (removes building-sized bumps) followed by
smoothing, then resample to a UE-friendly 2017x2017 grid.

Outputs (world/delhi/terrain/):
  old_delhi_heightmap_2017.png   16-bit grayscale, UE Landscape import
  old_delhi_dtm.npy              float32 metres (same grid) for snapping buildings to ground
  terrain.json                   centre lat/lon, extent, pixel size, Z scale, base elevation, UE import settings
"""
import json
from pathlib import Path

import numpy as np
import tifffile
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "source" / "Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif"
OUT = ROOT / "terrain"; OUT.mkdir(exist_ok=True)

LAT0, LON0 = 28.655, 77.230      # centre of the Phase 1 box
HALF_M = 1600.0                  # 3.2 km square, covers the 2.9 x 2.2 km box with margin
N = 2017                         # UE landscape: 2017 verts = 2016 quads (8x8 components of 252)
ZSCALE = 20.0                    # UE Z scale -> +-51.2 m range, 0.16 cm precision

R = 6378137.0
m_per_deg_lat = np.radians(1) * R
m_per_deg_lon = np.radians(1) * R * np.cos(np.radians(LAT0))

dsm = tifffile.imread(SRC).astype(np.float32)          # 3600x3600, 1 arc-second, tile N28 E077
# tile pixel centres: lat = 29 - (r+0.5)/3600, lon = 77 + (c+0.5)/3600
pad = 0.01
r0 = int((29 - (LAT0 + HALF_M / m_per_deg_lat + pad)) * 3600)
r1 = int((29 - (LAT0 - HALF_M / m_per_deg_lat - pad)) * 3600) + 1
c0 = int((LON0 - HALF_M / m_per_deg_lon - pad - 77) * 3600)
c1 = int((LON0 + HALF_M / m_per_deg_lon + pad - 77) * 3600) + 1
crop = dsm[r0:r1, c0:c1]

# bare-earth approximation: opening with ~150 m footprint removes buildings, then smooth
dtm = ndimage.grey_opening(crop, size=(5, 5))
dtm = ndimage.gaussian_filter(dtm, sigma=2.0)

# resample to N x N over the square (row 0 = north, like a top-down image)
ys = np.linspace(HALF_M, -HALF_M, N)                    # metres north of centre
xs = np.linspace(-HALF_M, HALF_M, N)                    # metres east of centre
lat = LAT0 + ys / m_per_deg_lat
lon = LON0 + xs / m_per_deg_lon
rr = (29 - lat) * 3600 - 0.5 - r0
cc = (lon - 77) * 3600 - 0.5 - c0
RR, CC = np.meshgrid(rr, cc, indexing="ij")
grid = ndimage.map_coordinates(dtm, [RR, CC], order=3, mode="nearest").astype(np.float32)

base = float(np.floor(grid.min()) - 1.0)
rel_cm = (grid - base) * 100.0
val = np.clip(np.round(32768 + rel_cm * 128.0 / ZSCALE), 0, 65535).astype(np.uint16)
Image.fromarray(val).save(OUT / "old_delhi_heightmap_2017.png")
np.save(OUT / "old_delhi_dtm.npy", grid)

px = 2 * HALF_M / (N - 1)
meta = {
    "centre_latlon": [LAT0, LON0], "extent_m": 2 * HALF_M, "size_px": N, "pixel_m": px,
    "elev_min_m": float(grid.min()), "elev_max_m": float(grid.max()), "base_elev_m": base,
    "ue_import": {
        "heightmap": "old_delhi_heightmap_2017.png",
        "scale_xyz_cm": [px * 100, px * 100, ZSCALE],
        "section_size": "63x63 quads", "sections_per_component": "2x2", "components": "16x16",
        "location_cm_note": "Place landscape centre at world origin; Z = 0 corresponds to base_elev_m + 0 m "
                            "(heightmap value 32768). World Z(cm) = (elev_m - base_elev_m) * 100.",
    },
    "source": "Copernicus DEM GLO-30 (DSM) - (c) DLR e.V. 2010-2014 and (c) Airbus Defence and Space GmbH "
              "2014-2018 provided under COPERNICUS by the European Union and ESA; bare-earth approximated "
              "by morphological opening.",
}
json.dump(meta, open(OUT / "terrain.json", "w"), indent=2)
print(f"terrain {N}x{N} @ {px:.3f} m/px, ground {grid.min():.1f}-{grid.max():.1f} m (base {base} m)")
