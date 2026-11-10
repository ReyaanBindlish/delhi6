# World pipeline: Gulbahar Lines and Hazrat Nizamuddin

First two playable locations, generated from real map data (Overture footprints + OSM + Copernicus GLO-30 DEM) and ready to import into UE 5.8. All scripts live in `world/story/scripts/`; the originals in `world/karachi/scripts/` and `world/delhi/scripts/` are untouched (copied and adapted).

Frame: local metres, X east, Y north, Z up, origin = slice centre = terrain centre, Z = metres above the DTM minimum. Unreal: `(X, -Y, Z) * 100` cm.

## Locations

| | Gulbahar Lines (Act 4, fictional) | Hazrat Nizamuddin (Act 2) |
|---|---|---|
| Real basis | Lyari core, Karachi | NZM station + basti, Delhi |
| Centre (lat, lon) | 24.8755, 66.9935 | 28.5889, 77.2540 |
| Playable square | 600 x 600 m | 800 x 800 m |
| Style | karachi (plaster, shutters, grills, party walls) | delhi (ochre/pink/whitewash, some exposed brick) |
| Buildings | 2,842 (4 religious, no shop shutters, tagged NoCombat) | 648 |
| Building faces | 2.06 M | 1.53 M |
| Station set | none | 5 platforms, 5 canopies, 42 track pieces, 6 footbridges, 9 stair flights, 9 signals, 6 elevated road decks |
| Terrain | 16 tiles 252 m @ 4 m (about 190k faces) + 1009 px heightmap | same |
| Total (all pieces, wires, roads, terrain) | about 2.5 M | about 2.0 M |
| Map | `/Game/Maps/Gulbahar_Lines` | `/Game/Maps/Nizamuddin` |
| Export size | 407 MB | 250 MB |

Why this square for Gulbahar: the research-08 sub-box is S24.8640 W66.9850 N24.8800 E67.0010. A sliding 600 m window found its densest position at 24.8755, 66.9935 (2,844 footprints, about 3x the median window of 1,791 and 3x the 942 around Kakri Ground). Kakri Ground (24.8563, 66.99929) lies about 0.9 km south of that sub-box's southern edge (2.1 km from the chosen centre), in open ground, so it is kept as the separate preset `gulbahar_kakri` for the later 11-minute-meeting tile (`prep_location.py gulbahar_kakri`, then terrain, then build). The chosen centre also matches the existing 300 m Lyari slice (24.8752, 66.9944).

Heights: Overture `height` / `num_floors` when present (Gulbahar has none, Nizamuddin 29 tagged), else a style estimate from footprint area (Lyari 2-4 storeys, mode 3; Delhi basti 2-4, colonies 3-5, large institutional footprints low; sheds and religious 1). Mean floors 2.96 / 3.04. Heights are estimates, not surveyed.

LOD tiers (build_location.py `--lod-radii`): LOD0 full detail (grill bars, shutter slats, window frames), LOD1 sparse grills and no slats, LOD2 glass and openings only. Keeps each location under 3 M faces.

## Station set (Nizamuddin)

Built from OSM `railway=platform`, `railway=rail`, `highway=footway|steps` with `bridge=yes`, `railway=signal`:

- Platforms: real polygons extruded to 0.84 m above rail level (Indian Railways high level), concrete sides, yellow safety strip 0.6 m in from every track-side edge, one UCX piece per convex part.
- Canopies: gabled sheet roof 4.6 m above the platform, steel columns about every 8 m (one row, or two on wide platforms), one UCX box per column. The roof has no collision.
- Tracks: 1,676 mm broad gauge, ballast bed, two rails, sleepers every 0.65 m, UCX per segment.
- Footbridges FOB 1, 2, 3 and two more footways near the rail: deck 6.8 m above ground, steel mesh railings (with collision), roof sheet if `covered=yes` (FOB 2), columns placed only where clear of the tracks, and 9 stair flights (stepped visual, one ramp wedge for collision) joining deck to platform or ground.
- Elevated roads: OSM `bridge=yes` trunk or primary ways become ramped viaduct decks with barriers and piers (the Baba Banda Singh Bahadur Setu viaduct).
- Not modelled yet: overhead electrification masts and catenary, rolling stock, the pedestrian subway (not in OSM), station hall interior. The station building is a generic Overture footprint.

`manifest.json` lists every piece with `kind`, `location_m`, collision piece count, platform `ref`/`name`, footbridge `deck_z_m`; `player_starts` (Platform 2-3 on the platform top, nearest street), `pois` (133 named OSM nodes and buildings in Nizamuddin, 13 in Gulbahar, with local metres and lat/lon, a `religious` flag), `surface_types` (material to surface name for the physics agent), `terrain`, `landscape_json`.

## How to rebuild

```powershell
cd C:\Users\Rayaan\Desktop\game\world\story\scripts
$py = "C:\Users\Rayaan\Desktop\game\characters\.venv\Scripts\python.exe"
$bl = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
foreach ($l in "nizamuddin","gulbahar_lines") {
  & $py prep_location.py $l                 # footprints, roads, rail plan -> export/<l>/_work/slice.json, pois.json
  & $py make_terrain_story.py $l            # DTM, 1009 px heightmap -> terrain/<l>/
  & $bl --background --python terrain_tiles_story.py -- "..\terrain\$l" "..\export\$l\terrain"
  & $bl --background --python build_location.py -- $l      # buildings, station, roads, wires, FBX+UCX, manifest.json
}
# optional preview: blender --background export\<l>\_work\<l>.blend --python render_preview.py -- <abs out prefix> cx cy cz yaw pitch ortho
```

Measured here: prep about 20 s, terrain about 5 s, full build plus export 2 to 3 min (Nizamuddin) and about 3 min (Gulbahar). Presets (centre, size, sources, DEM tile) are in `loc_config.py`.

## Import into Unreal

```powershell
$ue = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
& $ue C:\Users\Rayaan\Desktop\game\OldDelhi\OldDelhi.uproject -run=pythonscript -script="C:\Users\Rayaan\Desktop\game\OldDelhi\Tools\ue\import_location.py gulbahar_lines" -unattended -nop4
& $ue ...same... -script="...\import_location.py nizamuddin" ...
```

`OLDDELHI_LIMIT=50` imports only 50 buildings (smoke test), `OLDDELHI_NO_LIGHTING=1` skips the placeholder sun/sky/fog (let the weather subsystem own it), `OLDDELHI_LOCATION` replaces the argument. The script reuses `/Game/Delhi/Materials` when `build_vertical_slice.py` has run, otherwise creates `/Game/World/Materials` (master with MPC_Weather wetness hookup, 20 material instances including the new platform, ballast, rail, canopy, safety line and sleeper ones). Meshes import with Nanite and UCX collision, terrain tiles use complex-as-simple. It places everything from the manifest, adds PlayerStarts, a NavMeshBoundsVolume, and saves the level. It has been syntax-checked only; it has not been run in the editor (the full editor was not launched).

Landscape alternative: `terrain/<loc>/landscape.json` holds import settings for `LS_<loc>_1009.png` (1009 verts, 1 m/quad, 8x8 components of 2x2 sections of 63 quads, scale 100/100/5, actor at the NW corner). Use either the tiles or the Landscape, not both.

## Cesium + Google Photorealistic 3D Tiles: editor reference only

Google imagery, tiles and 3D data must never be copied into or shipped with the game. Cesium for Unreal streaming Google Photorealistic 3D Tiles is allowed only as an editor-time overlay for visual comparison.

1. Enable the Cesium for Unreal plugin (not enabled by default) and set `CESIUM_ION_TOKEN`.
2. Run the import with `OLDDELHI_CESIUM_REF=1`. It spawns `REF_CesiumGeoreference_EditorOnly` (origin = slice centre lat/lon, height = terrain base ASL) and `REF_Google3DTiles_EditorOnly` (Cesium ion asset 2275207). Both are flagged editor-only (excluded from cook), hidden in game, tagged `EditorReferenceOnly`, and placed in the World Outliner folder `ReferenceOnly`.
3. Toggle the eye icon on that folder to flip between our generated geometry and the real photogrammetry. If the tiles sit too high or low, set `CESIUM_HEIGHT_OFFSET` (geoid versus ellipsoid differs by tens of metres) or edit the georeference origin height.
4. Before packaging, delete the folder. Nothing from the overlay is baked into any asset.

## Known limits and follow-ups

- Footprints come from Overture (Google Open Buildings in Karachi): roofs and walls are procedural, not scanned. Textures are the shared Poly Haven CC0 set.
- Gulbahar has one elevated deck (the real Lyari expressway) and no rail. Orangi (hills, stepped lanes) is a separate second tile.
- Terrain is 30 m DEM resampled and smoothed (about 2% slope at Nizamuddin); the platforms are level slabs placed on the mean ground, so cuttings or banks of a few tens of cm can appear at the station edges. Level the terrain under the rail corridor if it matters.
- Roads are flat ribbons on the terrain (no kerbs, no lane markings); the overhead cable tangle is procedural.
