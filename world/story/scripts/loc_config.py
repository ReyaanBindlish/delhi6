"""Shared location presets for the story-world pipeline (prep_location / make_terrain_story /
terrain_tiles_story / build_location). Plain python, no third-party imports.

Frame used everywhere: local metres, X east, Y north, Z up, origin = slice centre = terrain centre.
Unreal: (X, -Y, Z) in cm (see OldDelhi/Tools/ue/import_location.py).
"""
from pathlib import Path

GAME = Path(__file__).resolve().parents[3]            # ...\game
STORY = GAME / "world" / "story"
OUT_ROOT = STORY / "export"
TERRAIN_ROOT = STORY / "terrain"
TEX_DIR = GAME / "world" / "textures_polyhaven_cc0"
BLENDER = r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
PY = str(GAME / "characters" / ".venv" / "Scripts" / "python.exe")

# Landscape-friendly terrain: 1009 verts at 1 m/quad = 1008 m. Tiles are 4 x 4 of 252 m at 4 m mesh res.
TERRAIN_EXTENT_M = 1008.0
TERRAIN_N = 1009

LOCATIONS = {
    # Act 4 (fictional Gulbahar Lines, real Lyari fabric). Densest 600 m square of the research-08 sub-box
    # S24.8640 W66.9850 N24.8800 E67.0010: ~2.8k footprints vs 942 around Kakri Ground (open ground, south of
    # the sub-box). Kakri is exposed as preset "gulbahar_kakri" for the later 11-minute-meeting tile.
    "gulbahar_lines": dict(
        title="Gulbahar Lines", style="karachi", center=(24.8755, 66.9935), size=600.0,
        geojson=GAME / "world" / "karachi" / "source" / "lyari_overture_buildings.geojson",
        osm=GAME / "world" / "karachi" / "source" / "karachi_lyari.osm",
        dem=GAME / "world" / "karachi" / "source" / "Copernicus_DSM_COG_10_N24_00_E066_00_DEM.tif",
        dem_tile=(25, 66),   # (north edge lat, west edge lon) of the 1-degree tile
        map_name="Gulbahar_Lines", rail=False,
    ),
    "gulbahar_kakri": dict(
        title="Gulbahar Lines (Kakri Ground)", style="karachi", center=(24.8563, 66.99929), size=600.0,
        geojson=GAME / "world" / "karachi" / "source" / "lyari_overture_buildings.geojson",
        osm=GAME / "world" / "karachi" / "source" / "karachi_lyari.osm",
        dem=GAME / "world" / "karachi" / "source" / "Copernicus_DSM_COG_10_N24_00_E066_00_DEM.tif",
        dem_tile=(25, 66), map_name="Gulbahar_Kakri", rail=False,
    ),
    # Act 2. Station core ~0.8 x 0.8 km around the platforms (research 08 section 2.1).
    "nizamuddin": dict(
        title="Hazrat Nizamuddin", style="delhi", center=(28.5889, 77.2540), size=800.0,
        geojson=STORY / "source" / "nizamuddin_overture_buildings.geojson",
        osm=STORY / "source" / "nizamuddin.osm",
        dem=GAME / "world" / "delhi" / "source" / "Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif",
        dem_tile=(29, 77), map_name="Nizamuddin", rail=True,
    ),
}


def paths(name):
    out = OUT_ROOT / name
    return dict(out=out, work=out / "_work", slice=out / "_work" / "slice.json",
                terrain=TERRAIN_ROOT / name, tiles=out / "terrain")
