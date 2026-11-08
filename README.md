# delhi6

A hyper-realistic open-world game set in Old Delhi ("Delhi-6"), built with an open-source asset pipeline and Unreal Engine 5.

![Old Delhi street-level preview](world/delhi/preview_old_delhi_street_up.png)

## What's here

| Folder | Contents |
|---|---|
| `game-design-docs/` | Design docs. Start with [implementation.md](game-design-docs/implementation.md) (Phase 1: Old Delhi) and [world-design-pipeline.md](game-design-docs/world-design-pipeline.md) |
| `world/delhi/` | Old Delhi modeled slice (Chandni Chowk), district blockouts, 557 real points of interest |
| `world/karachi/` | Lyari modeled slice, Saddar/Kharadar blockouts, and the city generator scripts |
| `world/mumbai/` | South Mumbai blockout and the OSM-to-Blender importer |
| `world/textures_polyhaven_cc0/` | CC0 PBR textures from Poly Haven |
| `characters/` | Two characters: concept sheet → Hunyuan3D-2 mesh → projected texture → FBX for UE5 |

Every `.blend` and `.fbx` is real-scale (1 unit = 1 m, Z up). City slices include `UCX_` collision meshes that Unreal imports as simple collision.

## Pipeline

**Cities** (`world/karachi/scripts/`):
1. `lyari_prep.py`: real building footprints (Overture Maps) + OSM roads → per-building floors, exposed and street-facing walls, convex collision pieces.
2. `lyari_build.py <slice.json> <out> <textures> [karachi|delhi]`: run in Blender. Models every building: openings with reveals, frames and glass, welded grills, rolling shutters, balconies, sunshades, parapets, water tanks, stair rooms, AC units, signboards, and overhead cable tangles.
3. `lyari_render.py`: finds a clear street spot by ray-casting and renders street and aerial previews.

`world/mumbai/scripts/osm_to_blender.py` builds quick massing blockouts straight from an `.osm` file.

**Characters** (`characters/scripts/`):
1. `01_crop_views.py`: crops the sheet and removes the background with BiRefNet (ONNX).
2. `02_generate_shape.py`: generates the mesh with Hunyuan3D-2.
3. `03_project_texture.py`: projects front, side and back views onto the mesh.
4. `04_blender_scene.py`: builds the Blender scene and exports FBX and GLB.

## Setup

- Blender 5.2+.
- Python 3.11 venv in `characters/.venv`: `torch` (CUDA 12.4), `transformers==4.49.0`, `diffusers==0.32.2`, `trimesh`, `pymeshlab`, `xatlas`, `opencv-python`, `onnxruntime-gpu`, `shapely`, `scipy`, `overturemaps`.
- Hunyuan3D-2: `git clone https://github.com/Tencent-Hunyuan/Hunyuan3D-2 characters/tools/Hunyuan3D-2`.
- BiRefNet: download `BiRefNet-general-epoch_244.onnx` from the rembg releases to `characters/tools/models/birefnet-general.onnx`.

## Data (not committed; re-download)

- **Buildings:** `overturemaps download --bbox=<w,s,e,n> -f geojson --type=building -o <file>.geojson`
- **OSM:** the Overpass API (`nwr(<s>,<w>,<n>,<e>); (._;>;); out meta;`) or Geofabrik extracts
- **Elevation:** Copernicus DEM 30 m tiles from `copernicus-dem-30m.s3.amazonaws.com`

Phase 1 bounding boxes are in [implementation.md](game-design-docs/implementation.md).

## Attribution and licences

- Map data © OpenStreetMap contributors, ODbL 1.0.
- Building footprints from Overture Maps Foundation (includes Google Open Buildings, CC BY 4.0, and OSM, ODbL).
- Elevation: Copernicus DEM © DLR / Airbus, provided under the Copernicus programme.
- Textures from Poly Haven, CC0.
- Shape model: Tencent Hunyuan3D-2 under the Tencent Hunyuan Community License. That licence excludes the EU, UK and South Korea; check it before commercial use.

All brands, shop names and people in the game are fictional (see the fiction policy in `implementation.md`). Real monuments and places are depicted respectfully.
